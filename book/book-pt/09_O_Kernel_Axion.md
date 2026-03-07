# Capítulo 9: O Kernel de Segurança Axion

## 9.1 Definição Formal

**Status: Implementado e Testado**

O **Kernel Axion** é o guardião do runtime T81. Diferente de sistemas operacionais tradicionais onde a segurança é imposta no limite da chamada de sistema (troca de modo Usuário/Kernel), o Axion impõe segurança no **nível da instrução**.

Formalmente, o Kernel Axion é uma função $\alpha$ que mapeia o estado atual da máquina $S$ e a operação proposta $Op$ para um veredito $V$:
$$
\alpha: (S, Op) \to \{\text{Allow}, \text{Deny}, \text{Warn}, \text{Defer}\}
$$

Esta avaliação acontece **antes** da transição de estado $S \xrightarrow{Op} S'$ ocorrer. Se $\alpha(S, Op) = \text{Deny}$, a transição é abortada, e a máquina intercepta com uma `SecurityFault`.

## 9.2 O Modelo de Política

**Status: Implementado**

Políticas são conjuntos de regras declarativas que definem as restrições para um contexto de execução específico. Uma política não diz *o que* computar, mas *como* é permitido computar.

### 9.2.1 Linguagem de Política (S-Expressions)
Políticas Axion são definidas usando uma sintaxe S-expression estilo Lisp, garantindo fácil análise e canonicalização.

**Exemplo: Uma Política Estrita de Nível 1**
```lisp
(policy
  (tier 1)                  ; Restringir ao Nível Simbólico (Sem recursão, sem reflexão)
  (max-instructions 10000)  ; Limite rígido de gás
  (max-stack 256)           ; Limite de profundidade de pilha
  (max-tensors 0)           ; Alocações de tensor não permitidas
  (allowed-tensor-hashes []) ; Pesos externos não permitidos
)
```

**Exemplo: Uma Política de Inferência de IA de Nível 3**
```lisp
(policy
  (tier 3)
  (max-recursion 1024)
  (max-tensors 50)
  (max-tensor-elements 1000000)
  (allowed-tensor-hashes [
    "canon:sha3:a7f..." ; Pesos de modelo permitidos específicos
  ])
)
```

### 9.2.2 Capacidades
Capacidades são permissões granulares concedidas a um processo.
*   **NetAccess**: Capacidade de usar handles `IoNet` (Nível 4).
*   **MetaWrite**: Capacidade de modificar o segmento Meta (Reflexão).
*   **InfExpand**: Capacidade de instanciar formas infinitas (Nível 5).

## 9.3 Interceptação de Instrução

**Status: Implementado e Testado**

O Kernel Axion é integrado diretamente no loop de busca-decodificação-execução da VM.

### 9.3.1 O Gancho Interceptor
Em `src/vm/vm.cpp`, o loop principal invoca o motor de política:

```cpp
// Pseudocódigo do Loop do Intérprete
while (!halted) {
    Opcode op = fetch();

    // 1. Verificação Axion
    Verdict v = axion->evaluate(ctx);
    if (v == Verdict::Deny) {
        throw SecurityFault(v.reason);
    }

    // 2. Execução
    execute(op);

    // 3. Log de Auditoria
    if (v == Verdict::Warn || policy.audit_all) {
        trace.log(op, v, state_hash);
    }
}
```

### 9.3.2 Abstrações de Custo Zero?
Não. O T81 rejeita explicitamente "Abstrações de Custo Zero" se elas comprometerem a segurança. A verificação do Axion impõe uma sobrecarga de desempenho. Esta é uma escolha de design deliberada: **Correção > Desempenho**. No entanto, para traces compilados por JIT, as verificações de política são realizadas uma vez durante a gravação do trace e embutidas no trace otimizado como asserções protegidas, reduzindo significativamente a sobrecarga em tempo de execução.

## 9.4 O Log de Auditoria (Trace)

**Status: Implementado e Testado**

O **Trace** é a prova criptográfica do que aconteceu. Não é apenas um log de depuração; é uma cadeia Merkle de eventos.

### 9.4.1 Estrutura do Trace
Cada entrada no log contém:
1.  **Tick**: O tempo do relógio lógico.
2.  **Opcode**: A instrução executada.
3.  **Verdict**: A decisão do Axion.
4.  **StateHash**: Um hash SHA3-256 do estado relevante da máquina *após* a operação.

$$
H_{t} = \text{Hash}(H_{t-1} || \text{Op}_t || \text{Verdict}_t || \text{StateDiff}_t)
$$

O hash final $H_n$ é a **Prova de Execução**. Se duas partes executam o mesmo código e obtêm o mesmo $H_n$, elas são garantidas criptograficamente de terem alcançado exatamente o mesmo estado através exatamente do mesmo caminho.

## 9.5 Promoção Cognitiva

**Status: Implementado**

Um programa começa em um Nível Cognitivo específico (geralmente Nível 1). Ele pode solicitar **Promoção** para um nível superior para realizar operações mais complexas.

*   **Solicitação**: O programa executa um opcode `Promote` com um token de capacidade assinado.
*   **Avaliação**: O Axion valida o token contra a política.
*   **Resultado**: Se permitido, o `tier_status` da VM é atualizado, desbloqueando novos opcodes (ex: `Recurse` ou `Gossip`).

**Caminho de Escalonamento de Nível**:
1.  **Nível 1**: Seguro, limitado, tempo polinomial.
2.  **Nível 2**: Dinâmico, reflexivo.
3.  **Nível 3**: Recursivo, potencial de tempo exponencial (requer limites de gás).
4.  **Nível 4**: Não-local, dependente de rede (requer limites de consenso).
5.  **Nível 5**: Infinito (requer contenção estrita).

> **Verificação**: `tests/cpp/test_ethics.cpp` verifica que tentativas de usar opcodes de Nível 3 em uma política de Nível 1 resultam em uma `SecurityFault`.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
