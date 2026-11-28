# Capítulo 2: Princípios Centrais e Invariantes

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Capítulo 2: Princípios Centrais e Invariantes](#capítulo-2-princípios-centrais-e-invariantes)
  - [2.1 O Invariante de Determinismo](#21-o-invariante-de-determinismo)
    - [2.1.1 Superfícies de Determinismo e Vetores de Ataque](#211-superfícies-de-determinismo-e-vetores-de-ataque)
    - [2.1.2 A "Lacuna Libm" e `dmath`](#212-a-"lacuna-libm"-e-`dmath`)
  - [2.2 Lógica Ternária (Base-3)](#22-lógica-ternária-base-3)
    - [2.2.1 Por que Ternário?](#221-por-que-ternário?)
    - [2.2.2 Implementação](#222-implementação)
  - [2.3 Auditabilidade e o Trace Axion](#23-auditabilidade-e-o-trace-axion)
    - [2.3.1 A Estrutura do Trace](#231-a-estrutura-do-trace)
  - [2.4 Os Nove Princípios (Aplicação de Ética)](#24-os-nove-princípios-aplicação-de-ética)
  - [2.5 Checklist de Verificação](#25-checklist-de-verificação)
  - [2.6 Matriz de Auditoria Formal](#26-matriz-de-auditoria-formal)

<!-- T81-TOC:END -->


## 2.1 O Invariante de Determinismo

**Status: Implementado e Testado**

O axioma central da arquitetura T81 é o **Determinismo Estrito**. Neste sistema, um programa $P$ não é uma sugestão ao hardware; é uma definição matemática de uma função de transição de estado $f$.

Formalmente, dado um estado inicial $S$ e um vetor de entrada $I$, a função deve satisfazer:
$$
\forall \text{hardware } H: \text{Exec}_H(S, I) \to S' \implies S' \text{ é invariante}
$$

Alcançar isso requer a eliminação de todas as fontes de não-determinismo comuns na computação moderna. O T81 trata o ambiente hospedeiro (SO, CPU, FPU) como uma "fonte de entropia adversária" que deve ser restringida.

### 2.1.1 Superfícies de Determinismo e Vetores de Ataque

A "Superfície de Determinismo" é o limite onde a máquina abstrata interage com a realidade física. Qualquer vazamento de realidade física (tempo, ruído aleatório, peculiaridades de hardware) para o estado lógico constitui uma **Violação de Determinismo**.

| Camada   | Risco de Determinismo        | Mitigação                 | Evidência de Implementação |
| :--- | :--- | :--- | :--- |
| **Compilador** | Ordenação de tokens, iteração de mapas | Emissão canônica de AST | `src/frontend/ast.cpp` (Sorted Maps) |
| **Memória VM** | Vazamento de endereço de ponteiro | Manipuladores Opacos (Índices) | `src/vm/vm.cpp` (Segmentos de Memória) |
| **Coletor de Lixo** | Ciclos de coleta não-determinísticos | Gatilhos de contagem de alocação | `src/vm/gc.cpp` (GC baseado em instrução) |
| **Concorrência** | Condições de corrida, agendamento | Corrotinas cooperativas | `src/vm/scheduler.cpp` (Ticks Determinísticos) |
| **Ponto Flutuante** | Deriva de FPU do hospedeiro (IEEE-754) | `dmath` float de software | `include/t81/core/T81Float.hpp` |
| **Transcendental** | Variância de implementação Libm | Séries de Taylor (Iter fixas) | `include/t81/core/detail/dmath.hpp` |
| **JIT** | Divergência de otimização    | Verificações de Equivalência de Traço | `src/vm/jit_compiler.cpp` |

### 2.1.2 A "Lacuna Libm" e `dmath`
Uma vulnerabilidade crítica no determinismo multiplataforma é a "Lacuna Libm". O padrão IEEE-754 define formatos de ponto flutuante, mas deixa as funções transcendentais (sin, cos, pow) vagamente especificadas. Como resultado, `std::sin(x)` em x86_64/GLIBC pode diferir em 1 ULP (Unidade no Último Lugar) de `std::sin(x)` em ARM64/MUSL.

O T81 resolve isso com **`dmath`** (Matemática Determinística), uma biblioteca personalizada que implementa:
*   **Aritmética Soft-Float**: `Add`, `Sub`, `Mul` são bit-exact.
*   **Transcendentes Personalizadas**: `Sin`, `Cos`, `Exp` são implementadas via séries de Taylor/Maclaurin com um número fixo de iterações e constantes fixas, ignorando a `libm` do hospedeiro.
*   **Modo de Arredondamento**: Ties-to-even é imposto via software.

> **Invariante**: $\text{dmath::sin}(x)$ produz exatamente o mesmo padrão de bits em um Intel i9, um Apple M3 e uma placa de desenvolvimento RISC-V.

## 2.2 Lógica Ternária (Base-3)

**Status: Implementado e Testado**

O T81 é um sistema **ternário balanceado**. A unidade fundamental é o **trit**, com valores $\{-1, 0, 1\}$ (frequentemente denotados como $-, 0, +$ ou $T, 0, 1$).

### 2.2.1 Por que Ternário?
1.  **Aritmética Simétrica**: A faixa de valores é simétrica em torno de zero. Em binário (Complemento de Dois), a faixa é assimétrica (ex: -128 a +127). Em ternário balanceado, um inteiro de $N$-trits cobre $-\frac{3^N-1}{2} \dots +\frac{3^N-1}{2}$.
2.  **Eficiência de Arredondamento**: O arredondamento para o inteiro mais próximo é equivalente ao truncamento. $0.5$ não é exatamente representável, evitando o "problema de arredondamento de 0.5".
3.  **Economia de Base**: A economia de base $E(r, N) = r \lfloor \log_r N \rfloor$ é minimizada quando $r = e \approx 2.718$. O inteiro $3$ está mais próximo de $e$ do que o $2$, tornando o ternário teoricamente mais eficiente para densidade de armazenamento de informação.
4.  **Representação com Sinal**: Números negativos não requerem um bit de sinal separado. O sinal é carregado pelo trit não-zero mais significativo.

### 2.2.2 Implementação
Na base de código C++, trits são simulados em hardware binário para eficiência.
*   **Armazenamento Empacotado**: `T81Int` usa um esquema de codificação de 2 bits por trit (00=0, 01=1, 11=-1/T). Isso permite que 4 trits caibam em um byte (um Tryte).
*   **Aritmética**: Operações são implementadas usando matemática inteira que simula cadeias de transporte (carry) ternárias balanceadas.
    *   Exemplo: $1 + 1 = 1T$ (que é $3 - 1 = 2$).
    *   Exemplo: $T + T = T1$ (que é $-3 + 1 = -2$).

## 2.3 Auditabilidade e o Trace Axion

**Status: Implementado e Testado**

O determinismo por si só é insuficiente; a execução deve ser **auditável**. O Kernel Axion produz um log criptográfico chamado **Trace**.

### 2.3.1 A Estrutura do Trace
Um trace $\mathcal{T}$ é uma sequência ordenada de eventos $E_0, E_1, \dots, E_k$. Cada evento captura uma transição de estado significativa ou verificação de política.

```cpp
struct AxionEvent {
    uint64_t tick;          // Carimbo de data/hora lógico
    Opcode op;              // O código de operação tentado
    Verdict verdict;        // A decisão do kernel (Permitir/Negar)
    CanonHash81 state_hash; // Raiz Merkle do estado da VM
    std::string metadata;   // Informações de depuração contextuais
};
```

Este trace serve como uma **Prova de Execução**. Ao reproduzir o trace contra o estado inicial, um auditor pode provar matematicamente que a computação produziu o resultado reivindicado sem confiar no hardware que o produziu.

## 2.4 Os Nove Princípios (Aplicação de Ética)

**Status: Implementado e Testado**

O T81 incorpora um conjunto de "Princípios Constitucionais" imutáveis ($\Theta_1 \dots \Theta_9$) diretamente no motor de política da VM. Estes não são meramente diretrizes; são restrições de tempo de execução impostas pelo Kernel Axion.

| Símbolo | Princípio | Descrição | Imposto Por |
| :--- | :--- | :--- | :--- |
| $\Theta_1$ | **Não-Dano** | Camada de segurança fundamental; previne corrupção de memória e segfaults. | Verificações de Limites de Memória |
| $\Theta_2$ | **Não-Coerção** | Previne transições de estado forçadas sem autorização criptográfica. | Verificação de Assinatura |
| $\Theta_3$ | **Verdade** | A informação deve ser canônica; dois hashes diferentes não podem mapear para o mesmo objeto. | Verificações de Colisão CanonFS |
| $\Theta_4$ | **Interpretabilidade** | Execução opaca "caixa preta" é alertada contra; geração de trace é obrigatória para o Nível 3+. | Registrador de Trace |
| $\Theta_5$ | **Integridade de Identidade** | Nós distribuídos devem manter chaves de identidade consistentes. | Handshake de Nível 4 |
| $\Theta_6$ | **Prioridade Ética** | Políticas de segurança sobrepõem otimizações de desempenho. | Preempção de Política |
| $\Theta_7$ | **Contenção de Entropia** | Previne expansão de recursos ilimitada (ex: loops infinitos, vazamentos de memória). | Limites de Recursão / Gas |
| $\Theta_8$ | **Consistência Canônica** | Todos os dados devem ser normalizados antes do hash. | Serializador |
| $\Theta_9$ | **Execução Transparente** | O sistema não deve ocultar efeitos colaterais; `MetaWrite` requer permissão explícita de política. | Interceptor Axion |

> **Exemplo**: Se um programa tenta recursão infinita, ele viola $\Theta_7$ (Contenção de Entropia). O Kernel Axion detecta que `recursion_depth > policy.max_depth` e emite um veredito `Deny`, convertendo a operação em um `Trap::SecurityFault`.

## 2.5 Checklist de Verificação

*   [ ] **Consistência de Float**: O `T81Float` produz padrões de bits idênticos para funções transcendentais (`sin`, `exp`) em todas as plataformas? (Execute `tests/cpp/test_T81Float.cpp` e `tests/cpp/test_property_float.cpp`)
*   [ ] **Determinismo do GC**: O Coletor de Lixo roda em contagens exatas de instrução (alocações), não tempo de parede? (Verifique `kGcInterval` em `src/vm/vm.cpp`)
*   [ ] **Integridade do Trace**: O log Axion é imutável durante a execução? (Verificado por `tests/cpp/axion_log_determinism_test.cpp`)
*   [ ] **Aplicação de Ética**: As verificações $\Theta$ são acionadas corretamente quando os limites são excedidos? (Verificado por `tests/cpp/test_ethics.cpp`)

## 2.6 Matriz de Auditoria Formal

| Princípio | Seção da Spec | Implementação | Cobertura de Teste |
| :--- | :--- | :--- | :--- |
| Determinismo Estrito | `spec/determinism-profile.md` | `src/vm/vm.cpp` | `tests/cpp/test_property_invariants.cpp` |
| Lógica Ternária | `spec/t81-data-types.md` | `include/t81/ternary.hpp` | `tests/cpp/ternary_arith_test.cpp` |
| Auditabilidade | `spec/axion-kernel.md` | `include/t81/axion/api.hpp` | `tests/cpp/test_ethics.cpp` |
| Armazenamento Canônico | `spec/supplemental/canonfs-spec.md` | `src/canonfs/` | `tests/cpp/canonfs_driver_test.cpp` |

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
