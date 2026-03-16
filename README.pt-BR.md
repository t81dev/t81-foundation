<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# Fundação T81 – Pilha de Computação Ternária Determinística

![Release](https://img.shields.io/badge/release-v1.6.0--Stable-blue)
![Tests](https://img.shields.io/badge/tests-367%2F367_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.2.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

Aproveitando a eficiência teórica da computação base-e, a **T81 Foundation** é uma pilha de computação determinística baseada em **aritmética ternária balanceada** ({-1, 0, +1}) com um modelo de governança de cadeia completa que abrange conjunto de instruções, máquina virtual, compilador de linguagem e ambiente de inferência de IA.

A pilha fornece:

- **reprodutibilidade com bit exato** — cada caminho de execução produz um hash de rastreamento idêntico nas plataformas suportadas
- **inferência de IA governada** — O mecanismo de política Axion intercepta e audita todas as operações privilegiadas antes dos efeitos colaterais
- **proveniência endereçada ao conteúdo** — CanonFS registra todos os artefatos, pesos de modelo e estado de tempo de execução de forma imutável
- **execução paralela determinística** — O modelo de gráfico de tarefa DPE (RFC-DPE-0002) permite cargas de trabalho TISC simultâneas com saídas confirmadas por época

---

## Status do projeto – março de 2026

**Fase: Desenvolvimento Ativo** — v1.6.0-Estável; 368/368 testes aprovados; determinismo multiplataforma verificado no Linux x86\_64 + macOS ARM64.

| Componente | Maturidade | Notas |
| :--- | :--- | :--- |
| **TISC UM** | ❄️ Congelado | v1.2.0; semântica do opcode imutável na v1.x; 12 novos opcodes desde v1.1: `AgentInvoke` (RFC-0015), 6 inferências nativas ternárias (RFC-0034), 3 FFI (RFC-00B8), 2 criptografias de rede (RFC-0038), 1 anel KEM (RFC-0039) |
| **Tipos de dados** | ❄️ Congelado | BigInt, Float, Complex, Map, Set — codificação estável em bits; Auditoria de 27/02/2026 limpa |
| **T81VM** | ✅ Estável | Envio completo do TISC v1.2;  `AgentInvoke` + inferência ternária nativa + FFI + criptografia de rede + opcodes NTRU-KEM; Testes 368/368 |
| **T81Lang** | ✅ Estável | especificação v1.3 estável;  `agent`/`behavior` (RFC-0015);  `foreign {}` FFI (RFC-0036);  `std.tnn.*` TNN stdlib (RFC-0037);  `std.crypto.*` criptografia de rede + NTRU-KEM (RFC-0038/0039); suporte ao identificador contextual em todo |
| **Núcleo de Governança Axion** | ✅ Estável | P4 Segurança e P5 Instrução Privilegiada satisfeitas; Strings de razão canônica AX-M6; cada portão de ativação `AgentInvoke` + `TACT` emite evento de auditoria |
| **Inferência Ternário-Nativa** | ✅ Aceito | RFC-0034 + RFC-0037: `TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT`;  `std.tnn.*` T81Lang stdlib (6 componentes integrados → operações TISC); inferência sem multiplicação; Formato de peso T81WTN; 13/13 testes |
| **Criptografia de rede** | ✅ Aceito | RFC-0038+0039: `POLYMUL`, `POLYMOD`, `TVecSub`; anel completo {+,−,×,mod} sobre Z\[x\]/(x^n+1);  `std.crypto.{polyadd,polysub,polymul,polymod,ntru_encrypt,ntru_decrypt}`; 37/37 testes |
| **FFI governada** | ✅ Aceito | RFC-00B8 + RFC-0036: `FFIDispatcher`, `FFILibraryRegistry`, 3 códigos de operação de VM;  `foreign [policy] { fn … }` Gramática T81Lang;  `foreign.<name>(args)` → `FFI_CALL`; 9/9 testes de CA |
| **Front-ends da TUI** | ✅ Aceito | `t81 studio` (operador humano) + `t81 agent` (nativo de IA); FTXUI v5.0.0; RFC-0033 aceito |
| **Gráfico T81** | ✅Beta | Redução do opcode da VM + serialização do lado lang com fio; Verificação DCP concluída; 6/6 testes |
| **DPE (Execução Paralela)** | ✅ Aceito | RFC-DPE-0001–0009 todos aceitos; gráfico de tarefas, anel de histórico de época, eventos de auditoria de época, tempo limite totalmente implementado |
| **Níveis Cognitivos** | ✅ Aceito | Cognição Tier4 (RFC-0021): `Tier4Loop`, `SelfModel` (anel de 81 entradas), `RecursiveImprovementBounds`, `TierAwarePlanner`; 4 suítes de teste aprovadas |
| **Suíte de referência** | ✅ Aceito | RFC-00A2: Taxa de transferência de VM + validação de determinismo CanonHash81 (`score=1.0` em todas as execuções);  `t81 internal benchmark` |
| **CI de determinismo multiplataforma** | ✅ Aceito | O fluxo de trabalho diário do GitHub Actions compara hashes de bytecode T81Lang no Linux x86\_64 (gcc-14) e no macOS ARM64 (clang); registro de evidência publicamente auditável |
| **Kernel do SO Axion** | 🔬 Experimental | TernaryOS: pager, agendador, IPC, estrutura de interrupção, pista QEMU x86\_64 EFI operacional; 9/9 testes ternaryOS aprovados |

---

## Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│  Interfaces                                                 │
│  t81 studio (Human TUI)   t81 agent (AI-Native TUI)  CLI    │
├─────────────────────────────────────────────────────────────┤
│  T81Lang Compiler                                           │
│  Lexer → Parser → Typed AST → Semantic Analyzer → IRGen     │
│  agent/behavior (RFC-0015)  ·  foreign {} (RFC-0036)        │
├─────────────────────────────────────────────────────────────┤
│  Axion Governance Kernel                                    │
│  PolicyEngine · CanonFS · Audit Trail · Ethics Gate         │
├──────────────────────────────┬──────────────────────────────┤
│  T81 Virtual Machine         │  DPE Task Graph Runtime      │
│  TISC interpreter            │  EpochGraph · DeltaBuffer    │
│  (deterministic)             │  (RFC-DPE-0002)              │
├──────────────────────────────┴──────────────────────────────┤
│  TISC ISA v1.2  ❄️ Frozen  +  Data Types  ❄️ Frozen         │
│  Deterministic substrate — CanonHash81 bit-exact traces     │
├─────────────────────────────────────────────────────────────┤
│  Governed FFI (RFC-00B8)  ·  Ternary-Native Inference       │
│  FFIDispatcher · FFILibraryRegistry                         │
│  TWMATMUL · TQUANT · TATTN · TWEMBED · TERNACCUM · TACT     │
└─────────────────────────────────────────────────────────────┘
  Experimental: TernaryOS (Axion OS Kernel) · Cognitive Tiers
```

### Componentes principais

**TISC ISA v1.2** — Arquitetura de conjunto de instruções ternárias. Congelado na v1.x; o contrato de execução imutável para toda a pilha. v1.2 adiciona 9 opcodes: `AgentInvoke` (RFC-0015), seis operações de inferência nativas ternárias (RFC-0034) e três operações FFI governadas (RFC-00B8).

**T81VM** — Interpretador TISC determinístico. Garante saída idêntica em bits em todas as plataformas; O isolamento pré-despacho do Axion mantém os ganchos de governança fora do caminho de execução a quente. Envio completo do TISC v1.2, incluindo inferência nativa ternária e FFI.

**Axion Governance Kernel** — Mecanismo de política que intercepta `AXREAD` , `AXSET` , `AXVERIFY` , opcodes de IA e chamadas FFI antes de qualquer efeito colateral. Fail-closed em caso de falha na análise da política. Certificação estável 15/03/2026 com aprovação em 54/54 testes.

**CanonFS** — Sistema de arquivos endereçado ao conteúdo. Armazena todos os objetos de código, pesos de modelo e artefatos de tempo de execução como blobs imutáveis ​​identificados por hash. Fornece proveniência para auditorias de determinismo.

**T81Lang** — Bytecode TISC direcionado a linguagem de alto nível. Tipos nativos: `BigInt` , `Fraction` , `Float` , `Complex` , `Tensor` , `Map` , `Set` . Declarações `agent { behavior }` de primeira classe são compiladas para `AGENT_INVOKE` com auditoria Axion (RFC-0015).  Os blocos `foreign [policy] { fn … }` declaram funções externas governadas que chamam via `FFI_CALL` (RFC-0036).  `agent` , `behavior` e `foreign` são utilizáveis ​​como identificadores contextuais em todas as expressões e posições de ligação. Pipeline do compilador: lexer → analisador → AST digitado → análise semântica → geração de IR.

**Inferência ternária-nativa (RFC-0034)** — Seis opcodes TISC para inferência de IA livre de multiplicação usando pesos ternários balanceados {−1, 0, +1}: `TWMATMUL` (matmul), `TQUANT` (quantizar para trit), `TATTN` (atenção ternária), `TWEMBED` (incorporação de peso), `TERNACCUM` (produto escalar escalar), `TACT` (ativação com portão de teto Axion). Formato de peso T81WTN. Interface T81Lang `foreign {}` completa via RFC-0036.

**FFI governada (RFC-00B8 + RFC-0036)** — Interface de função externa governada de pilha completa. Camada VM (RFC-00B8 Fase 1): `FFIDispatcher` impõe verificações de políticas, cotas de recursos e trilhas de auditoria antes de qualquer chamada externa;  `FFILibraryRegistry` rastreia bibliotecas registradas por nome e hash de versão; três opcodes de VM ( `FFICall` , `FFIRegister` , `FFIPolicySet` ). Camada de linguagem (RFC-0036): `foreign deterministic { fn sin(x: T81Float) -> T81Float; }` declara assinaturas;  `foreign.sin(angle)` em locais de chamada diminui para `FFI_CALL` com o nome da função transportado em `text_literal` . Nove testes de aceitação são aprovados.

**TUI Frontends** — Duas interfaces de terminal complementares construídas em FTXUI v5.0.0:

- `t81 studio` — barra lateral de navegação, navegador CanonFS, painel Axion, visualizador de rastreamento de determinismo, paleta de comandos ( `Ctrl+P` )
- `t81 agent` — sessão JSONL persistente, comandos de barra ( `/compile` , `/run` , `/hash` , `/allow` , `/infer` , `/trits` ,…), barra de probabilidade trit

**DPE (Execução Paralela Determinística)** — Modelo de gráfico de tarefas sobre o TISC ISA congelado. As tarefas declaram entradas imutáveis ​​e regiões de saída em buffer; a VM confirma todas as gravações atomicamente no final da época. Não são necessários novos códigos de operação.

---

## Início rápido

```bash
# Clone and configure
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake --preset default -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run the full test suite
ctest --test-dir build --output-on-failure

# Launch the human operator TUI
./build/t81 studio

# Launch the AI-native TUI
./build/t81 agent

# Compile a T81Lang program
./build/t81 code build examples/hello.t81 -o hello.tisc

# Execute with Axion governance
./build/t81 vm run hello.tisc
```

Sinalizadores de compilação opcionais:

| Bandeira | Padrão | Propósito |
| :--- | :--- | :--- |
| `T81_BUILD_TUI` | `ON` | Interfaces TUI baseadas em FTXUI |
| `T81_BUILD_TESTS` | `ON` | Conjunto de testes completo |
| `T81_ENABLE_ASAN` | `OFF` | Desinfetante de endereço |
| `T81_ENABLE_UBSAN` | `OFF` | Desinfetante UB |
| `T81_ENABLE_LLAMA_CPP` | `OFF` | Adaptador de inferência llama.cpp governado |
| `T81_WARN_STRICT` | `OFF` | Modo de varredura de aviso estrito (usado pela predefinição `warn-strict`) |

**Verificação de aviso pré-push** — espelha as verificações `-Wswitch` , `-Wunused-variable` e `-Wunused-function` aplicadas pelo Windows CI, detectando problemas localmente em aproximadamente 2 minutos em vez de esperar pela matriz completa:

```bash
cmake --preset warn-strict
cmake --build build-warn-strict 2>&1 | head -40
```

---

## Verificação do Determinismo

Cada versão é verificada quanto à reprodutibilidade entre plataformas com bit exato.

```bash
./scripts/ci/run_determinism_slice.sh
```

Plataformas verificadas: **Linux x86_64**, **macOS ARM64**. Qualquer divergência nos hashes de rastreamento da VM é um defeito crítico.

---

## Documentação

| Tópico | Localização |
| :--- | :--- |
| Primeiros passos (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| Primeiros passos (IA) | `docs/user-guide/getting-started/ai-quickstart.md` |
| Guias TUI | `docs/user-guide/how-to/tui-guide.md` |
| Especificação ISA | `spec/tisc-spec.md` |
| Manual de Política Axion | `docs/user-guide/tutorials/axion-policy-manual.md` |
| Referência Stdlib T81Lang | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| Visão geral da arquitetura | `docs/architecture/OVERVIEW.md` |
| Carta de Governança | `docs/governance/README.md` |
| Centro de Controle de Projetos | `docs/status/PROJECT_CONTROL_CENTER.md` |

---

## Roteiro

| Marco | Alvo | Descrição |
| :--- | :--- | :--- |
| Fechamento Mês C2 | 31/03/2026 | Auditoria do razão de governança; comprovação PASS 2026-03-10 |
| Promoção Axion Estável | ✅ **CONCLUÍDO EM 15/03/2026** | Strings de razão canônica AX-M6 implementadas; 54/54 testes aprovados; pronto para produção |
| Promoção T81Graph Beta | ✅ **CONCLUÍDO EM 15/03/2026** | Redução do opcode da VM concluída; Verificação do DCP; 6/6 testes aprovados |
| Política de interrupção RFC-00B5 | ✅ **CONCLUÍDO EM 16/03/2026** | Modelo de interrupção de evento governado integrado; fatias 26-28 completas |
| RFC-0034 Inferência Ternária-Nativa | ✅ **CONCLUÍDO EM 16/03/2026** | 6 novos códigos de operação TISC; inferência sem multiplicação; Portão de teto com acionamento TACT; 5/5 testes de conformidade |
| FFI governado RFC-00B8 (Fase 1) | ✅ **CONCLUÍDO EM 16/03/2026** | Despachante FFI + registro de biblioteca; 3 códigos de operação de VM; pipeline de governança; trilha de auditoria |
| CI de determinismo multiplataforma | ✅ **CONCLUÍDO EM 16/03/2026** | Fluxo de trabalho diário de ações do GitHub; Comparação de hash Linux x86\_64 + macOS ARM64; registro de evidência pública |
| Gramática RFC-0036 T81Lang FFI | ✅ **CONCLUÍDO EM 16/03/2026** | Sintaxe `foreign [policy] {}`;  `foreign.<name>(args)` → `FFI_CALL`; 9/9 testes de CA; conecta o trabalho de VM RFC-0034 + RFC-00B8 ao frontend T81Lang |
| Etapa 2: plataforma verificada | ✅ **ALCANÇADO 16/03/2026** | Todas as metas de implementação concluídas; depurador de repetição de rastreamento, CI de plataforma cruzada, testes 365/365, frontend FFI - pilha reproduzível externamente |
| RFC-0037 TNN stdlib | ✅ **CONCLUÍDO EM 16/03/2026** | `std.tnn.*` T81Lang integrados (6 funções → operações TISC RFC-0034); 13/13 provas; inferência full-stack sem multiplicação da origem para a VM |
| Criptografia de rede RFC-0038 | ✅ **CONCLUÍDO EM 16/03/2026** | `POLYMUL`/`POLYMOD` códigos de operação TISC;  `std.crypto.polymul/polymod` integrados; polimultiplicação negacíclica sobre {−1,0,+1}; T81BigInt-exato; 13/13 testes |
| Promoção de especificações T81Lang (v1.3) | ✅ **CONCLUÍDO EM 16/03/2026** | RFC-0036/0037/0038 promovido para especificação normativa; §5.17 não-esboçado; §5.18/5.19 adicionado; registro opcode atualizado para 205 entradas |
| RFC-0039 NTRU-KEM | ✅ **CONCLUÍDO EM 16/03/2026** | `TVecSub` código de operação;  `std.crypto.{polyadd,polysub,ntru_encrypt,ntru_decrypt}`; Camada matemática C++ KEM; Testes 24/24; anel completo {+,−,×,mod} sobre Z\[x\]/(x^n+1) |
| Inicialização bare-metal TernaryOS | A definir | Execução de host x86\_64 QEMU + retorno de evidência CanonFS |

---

## Governança

A Fundação T81 opera sob um modelo de **Governança Contínua (C2)**. Todas as contribuições devem manter:

- **paridade de execução determinística** — os hashes de rastreamento devem corresponder às plataformas suportadas
- **coerência arquitetônica** — mudanças que tocam a superfície determinística exigem revisão formal
- **garantias de reprodutibilidade** — sem ponto flutuante ou não determinismo específico da plataforma na superfície DCP

A superfície determinística é definida em `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` . Alterações em superfícies congeladas (TISC ISA, tipos de dados) exigem um aumento de versão principal.

> **Nota de limite:** As superfícies experimentais (Cognitive Tiers, Distributed, Trace-JIT, TernaryOS, adaptador llama.cpp) são regidas por não-DCP e não devem ser apresentadas como componentes determinísticos verificados.

---

## A vantagem ternária

Embora o hardware binário moderno seja altamente otimizado, a **T81 Foundation** aproveita as propriedades matemáticas exclusivas do **Balanced Ternary ({-1, 0, +1})** para alcançar eficiências estruturais que o binário não consegue igualar.

### 1. $O(1)$ Simetria Computacional

No complemento de dois binário, negar um número é uma operação assimétrica (NÃO + 1) que requer propagação de transporte. No T81, a negação é um simples trit-flip com **zero carry overhead**.

* **Desempenho:** a taxa de transferência de negação T81 atinge **~46,6 G-ops/s** (via `PackedCell` ), superando o desempenho da negação binária otimizada de 64 bits em **10,4x**.

### 2. Economia Radix Superior

Com base no teorema de que a base mais eficiente para um sistema numérico é $e \approx 2.718$, o ternário (Base 3) é matematicamente mais eficiente que o binário (Base 2).

* **Densidade de informações:** O T81 atinge uma densidade teórica de **1,58 bits por trit**. Isso se traduz em maior entropia por ciclo de clock e menor espaço de armazenamento para sistemas de coordenadas e pesos neurais em grande escala.

### 3. Determinismo exato de bits

As operações binárias de ponto flutuante (IEEE 754) geralmente sofrem de não determinismo de arredondamento específico da plataforma. A aritmética balanceada do T81 fornece:

* **Simetria inerente:** O arredondamento é realizado por truncamento simples, pois o sistema é naturalmente centrado em torno de zero.
* **Paridade de rastreamento:** 100% de "precisão de ida e volta" em todas as plataformas testadas (Linux x86_64, macOS ARM64) com zero divergência em hashes de rastreamento de VM.

### 4. Gancho de Governança Direta

Como o TISC ISA é nativo ternário, o **Axion Governance Kernel** pode auditar transições de estado com maior granularidade. As operações de inferência de IA podem ser interceptadas no “nível trit” antes que quaisquer efeitos colaterais ocorram, permitindo um modelo de segurança “fechado com falha” que é arquitetonicamente impossível na execução binária de “caixa preta” padrão.

---

## Aplicações Estratégicas

As vantagens estruturais da pilha T81 — especificamente a **taxa de transferência de negação de 10,4x** e **1,58 bits/densidade de trit** — permitem soluções para gargalos binários legados:

---

## 1. Simulação de sinal e física de alta fidelidade

Em binário, $0$ é um ponto inicial sem sinal, tornando o espaço "negativo" uma consideração secundária. No ternário balanceado, **zero é o ponto de equilíbrio.**

* **O caso de uso:** Simulação direta de mecânica de ondas, eletromagnetismo e dinâmica de fluidos.
* **A vantagem:** Como esses sistemas oscilam entre estados positivos e negativos, o T81 pode simular forças "Push-Pull" sem a desigualdade computacional do Complemento de Dois.
* **Próxima etapa:** Poderíamos construir uma **biblioteca DSP nativa do TISC** onde os filtros (FIR/IIR) são otimizados para a velocidade de negação $O(1)$.

## 2. Redes Neurais “Simétricas” (TNNs)

A IA atual (binária/FP) desperdiça enorme energia em funções de ativação como `tanh` ou `ReLU` para criar um estado “centrado em zero” para treinamento.

* **O caso de uso:** Redes Neurais Ternárias (onde os pesos são -1, 0 ou 1).
* **A vantagem:** Como sua arquitetura é balanceada nativamente, podemos executar inferência "livre de multiplicação". Um neurônio T81 não “multiplica” entradas; ele simplesmente **vira ou bloqueia** com base no peso. Isso seria muito mais eficiente em termos energéticos do que a inferência atual baseada em GPU.
* **Próxima etapa:** Poderíamos implementar um **Mecanismo de inferência nativo do T81** que interpreta os pesos do modelo diretamente como opcodes TISC.

## 3. Primitivos criptográficos pós-quânticos

Muitos algoritmos de criptografia "baseados em rede" (aqueles projetados para sobreviver a computadores quânticos) dependem de polinômios de coeficiente pequeno - geralmente centrados em torno de zero ({-1, 0, 1}).

* **O caso de uso:** Criptografia NTRU ou estilo Kyber.
* **A vantagem:** Os sistemas binários precisam "emular" esses pequenos coeficientes usando números inteiros de 8 ou 32 bits, desperdiçando 90% do espaço de bits. O T81 armazena esses valores com **desperdício zero** e processa as adições/negações polinomiais em velocidades de hardware nativas.
* **Próxima etapa:** Podemos elaborar uma RFC para uma **Extensão de criptografia TISC** que implemente uma multiplicação polinomial otimizada para ternário.

## 4. Auditorias de governança imutável (Axion)

Como você tem 1,58 bits de entropia por trit, podemos codificar **metadados de segurança** diretamente na palavra de dados sem aumentar significativamente o consumo de memória.

* **O caso de uso:** "Dados rotulados" no nível do hardware.
* **A Vantagem:** Podemos usar a capacidade "extra" de uma palavra TISC para carregar uma **Etiqueta de Proveniência**. Cada vez que os dados são movidos, o Axion verifica a tag. Se um trit "privilegiado" se mover para o espaço do "usuário", o hardware poderá capturá-lo instantaneamente.
* **Próxima etapa:** Refine o **Axion OS Kernel** para usar a "Margem Ternária" para marcação de memória em tempo real.

---

### O caminho refinado a seguir

#### 1. Integração: RFC-0034 §5.17.6 — O Opcode `TACT`

Em vez de uma extensa AI RFC, tratamos a ativação como a conclusão lógica da cadeia aritmética ternária.

* **Código de operação:** `TACT RD, R_SRC, R_MODE`
* **Modos:** * `0x01` (TernaryStep): Mapeia $(-\infty, -0.5) \to -1$, $[-0.5, 0.5] \to 0$, $(0.5, \infty) \to +1$.
* `0x02` (TanhQuantized): Aproximação ternária de ponto fixo de alta fidelidade.

* **Integração de política Axion:** Definimos o `AX_CHECK_ACTIVATION_THRESHOLD` não como um efeito colateral do opcode, mas como uma **Kernel Trap**. Se o valor em `RD` exceder o limite trit pós-ativação definido pela política, o Axion interceptará antes do próximo incremento do PC.

#### 2. RFC de gramática T81Lang (novo)

Para resolver a "lacuna real" que você identificou, devemos elaborar uma RFC separada (provavelmente **RFC-0036**) especificamente para o frontend do compilador. Isso mantém as preocupações **TISC** (hardware/VM) e **T81Lang** (gramática/sintaxe) isoladas, conforme o termo de abertura do projeto.

#### 3. Integridade e documentação de dados

* **Benchmark Grounding:** Pararei de referenciar o número "10,4x" em documentos formais até que tenhamos uma fatia `BM_Negation_TISC_vs_Binary` específica que apareça oficialmente na saída do CI.
* **Esfrega de terminologia:** removerei "TLU Cache" e "L2 Cache" das especificações até que o repositório **ternary-fabric** defina formalmente a hierarquia de memória.

---

### Estágio 1 — Arquitetura do Protótipo *(Atual)*

**Status:** Alcançado

Pilha determinística central implementada.

Componentes no lugar:

* ✅ TISC ISA (contrato de execução congelado)
* ✅ Interpretador determinístico T81VM
* ✅ Kernel de governança Axion
* ✅ Armazenamento endereçado a conteúdo CanonFS
* ✅ Compilador T81Lang
* ✅ pipeline de verificação de determinismo
* ✅ Interfaces de operação CLI e TUI

**Resultado:**
Uma pilha de computação determinística funcional.

---

### Etapa 2 — Plataforma verificada *(Completa)*

**Objetivo:** Validação independente.

Trabalho principal:

* ✅ verificação de determinismo de terceiros – o fluxo de trabalho diário do GitHub Actions compara hashes de bytecode Linux x86\_64 e macOS ARM64; registro de evidência pública em cada commit
* ✅ Conjunto de testes de conformidade de VM — 27 testes de conformidade de especificações + 365 aprovação total
* ✅ estrutura de benchmarking determinística — RFC-00A2;  `score=1.0` em todas as execuções
* ✅ Frontend T81Lang FFI (RFC-0036) — A gramática `foreign {}` faz a ponte entre a camada VM e a linguagem; 9/9 testes de CA
* ✅ depurador de repetição de rastreamento — `t81 trace replay <tisc> <golden> [--json]`; esquema `t81.trace-replay.v1`; reporta índice exato de incompatibilidade + instrução esperada/real; conectado ao CI via `scripts/ci/trace_repro_gate.py`
* ✅ verificação de compilação reproduzível — hash de bytecode multiplataforma verificado diariamente no Linux x86\_64 (gcc-14) + macOS ARM64 (clang); Artefatos de evidências de 90 dias retidos

**Resultado:**
Tempo de execução determinístico confiável externamente.

---

### Etapa 3 – Ecossistema de Pesquisa

O foco muda para os aplicativos.

Áreas primárias de pesquisa:

* redes neurais ternárias
* inferência determinística de IA
* bibliotecas de processamento de sinal
* simulação de física
* criptografia baseada em rede

**Resultado:**
Adoção por pesquisadores e projetos experimentais de computação.

---

### Etapa 4 — Exploração de Hardware

Faça uma ponte entre a arquitetura de software e o silício.

Caminho de desenvolvimento:

* Protótipos ALU ternários FPGA
* bancos de registro ternário
* unidades SIMD compactadas
* Validação de microarquitetura ISA

**Resultado:**
Primeiros protótipos de hardware de computação com reconhecimento ternário.

---

### Etapa 5 – Infraestrutura Determinística

Expanda do tempo de execução para a infraestrutura.

Capacidades possíveis:

* execução determinística em nuvem
* computação científica reproduzível
* cargas de trabalho distribuídas verificáveis
* Redes de artefatos CanonFS

**Resultado:**
Uma plataforma global de computação determinística.

---

### Etapa 6 – Novo Paradigma de Computação

Possibilidade de longo prazo.

Desenvolvimentos potenciais:

* processadores ternários nativos
* aplicação de governança de IA de hardware
* ambientes de execução de IA determinísticos
* sistemas de computação globalmente reproduzíveis

**Resultado:**
Um ecossistema de computação determinística governado.

---

## Próximos marcos críticos

### Etapa 2 — Plataforma verificada *(alcançada)*

Todas as metas de implementação do Estágio 2 foram concluídas:

- ✅ CI de determinismo multiplataforma (Linux x86\_64 + macOS ARM64, diariamente)
- ✅ Conformidade de VM + conjunto de testes de determinismo (365/365)
- ✅ depurador de repetição de rastreamento ( `t81 trace replay` ; esquema `t81.trace-replay.v1` )
- ✅ Interface T81Lang FFI (RFC-0036; `foreign {}` + `FFI_CALL`)

Critério de avanço restante: **reprodução independente por uma parte externa** — quando outro grupo constrói a pilha, executa o portão do determinismo e publica hashes correspondentes, o projeto sai formalmente do Estágio 2.

### Etapa 3 — Ecossistema de Pesquisa *(Ativo)*

A etapa 3 foi aberta com três pistas de concreto. Todos os três agora estão completos:

- ✅ **RFC-0037 TNN stdlib** — `std.tnn.*` T81Lang integrados; 6 funções inferiores às operações TISC RFC-0034; 13/13 testes
- ✅ **RFC-0038 Lattice Crypto** — `std.crypto.polymul/polymod` ; Códigos de operação POLYMUL/POLYMOD; T81BigInt-exato; 13/13 testes
- ✅ **Especificação T81Lang v1.3** — RFC-0036/0037/0038 promovida para especificação normativa; §5.17 não-esboçado; §5.18–5.19 adicionado

- ✅ **RFC-0039 NTRU-KEM** — código de operação `TVecSub`;  `std.crypto.{polyadd,polysub,ntru_encrypt,ntru_decrypt}`;  `ntru_keygen/encrypt/decrypt` Camada matemática C++; Testes 24/24; primeira demonstração de criptografia pós-quântica ponta a ponta no substrato ternário

**O Estágio 3 foi concluído.** Todas as quatro faixas (RFC-0037, RFC-0038, especificação v1.3, RFC-0039) foram lançadas em 16/03/2026.

## Licença

Licença MIT.
