<p align="center">
  <img src="assets/banner.png" alt="T81 — Uma Arquitetura de Computação Ternária" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 — Uma Arquitetura de Computação Ternária

![Release](https://img.shields.io/badge/release-v1.9.0--Stable-blue)
![Tests](https://img.shields.io/badge/tests-369%2F369_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-Apache_2.0-blue)

Aproveitando a eficiência teórica da computação em base-e, **T81** é uma arquitetura de computação determinística construída sobre **aritmética ternária balanceada** ({-1, 0, +1}) com um modelo de governança de cadeia completa cobrindo o conjunto de instruções, a máquina virtual, o compilador da linguagem e o ambiente de inferência de IA.

A arquitetura oferece:

- **reprodutibilidade exata de bits em superfícies verificadas** — superfícies determinísticas governadas produzem hashes de rastreamento idênticos nas plataformas suportadas
- **inferência de IA governada** — o mecanismo de política Axion intercepta e audita cada operação privilegiada antes de quaisquer efeitos colaterais
- **proveniência endereçada por conteúdo** — CanonFS registra todos os artefatos, pesos de modelo e estado de tempo de execução imutavelmente
- **execução paralela determinística para semântica de época governada** — o modelo de grafo de tarefas DPE (RFC-DPE-0002) permite cargas de trabalho TISC concorrentes com saídas comprometidas por época sob a atual superfície de determinismo

---

## Índice
- [Status do Projeto](#status-do-projeto)
- [Visão Geral da Arquitetura e Ecossistema](#visão-geral-da-arquitetura-e-ecossistema)
- [Componentes Principais](#componentes-principais)
- [Início Rápido](#início-rápido)
- [Verificação de Determinismo](#verificação-de-determinismo)
- [Documentação](#documentação)
- [Governança](#governança)
- [A Vantagem Ternária](#a-vantagem-ternária)
- [Licença](#licença)

---

## Status do Projeto

**Fase: Desenvolvimento Ativo** — v1.9.0-Stable; 369/369 testes aprovados; determinismo multiplataforma verificado em Linux x86\_64 + macOS ARM64.

A classificação da superfície de determinismo segue o modelo de governança introduzido pelo Registro de Superfície de Determinismo e RFC-0048:

- **DCP / superfície determinística verificada**: semântica, limite e prova de repetição são governados e aplicados por CI
- **Não DCP governado**: limitado por política e arquitetonicamente importante, mas ainda não tem direito a alegações determinísticas completas
- **Experimental**: superfície ativa de design ou validação; sem reivindicação de garantia determinística

| Componente | Maturidade | Notas |
| :--- | :--- | :--- |
| **TISC ISA** | ❄️ Frozen | v1.9.0; semântica de opcode imutável sob v1.x; `AgentInvoke` (RFC-0015), 6 inferência nativa ternária (RFC-0034), 3 FFI (RFC-00B8), 2 criptografia de reticulado (RFC-0038), 1 anel KEM (RFC-0039) |
| **Data Types** | ❄️ Frozen | BigInt, Float, Complex, Map, Set — codificação estável de bits; auditoria limpa |
| **T81VM** | ✅ Stable | DCP / superfície determinística verificada para o interpretador e a paridade atual de rastreamento da plataforma suportada; despacho completo de TISC v1.9.0 com `AgentInvoke`, inferência nativa ternária, FFI, criptografia de reticulado e opcodes NTRU-KEM; 369/369 testes |
| **T81Lang** | ✅ Stable | Geral não DCP governado: especificação v1.9.0 Stable com controles ativos de determinismo do compilador, mas a emissão do compilador permanece parcialmente verificada em vez de totalmente promovida como uma superfície determinística verificada |
| **Axion Governance Kernel** | ✅ Stable | Geral não DCP governado: cadeias de razão canônicas e ganchos de auditoria estão ativos, mas a superfície completa do kernel/governança é mais ampla que o atual registro determinístico verificado |
| **Ternary-Native Inference** | ✅ Stable | Não DCP governado: a superfície opcode/runtime/stdlib da RFC-0034 + RFC-0037 está implementada e evidenciada, mas nem todos os caminhos de execução adjacentes à inferência ainda são promovidos como superfícies determinísticas verificadas |
| **Lattice Cryptography** | ✅ Stable | Não DCP governado: a superfície RFC-0038+0039 é implementada e limitada por política; a promoção determinística permanece específica da superfície em vez de implícita para toda a vertical de criptografia |
| **Governed FFI** | ✅ Stable | Não DCP governado: a ponte linguagem/VM RFC-00B8 + RFC-0036 é implementada de ponta a ponta, mas a sandbox e a promoção de esquema mais ampla permanecem abertas antes de alegações determinísticas mais fortes |
| **TUI Frontends** | ✅ Beta | Não DCP governado: as TUIs de operador e agente são interfaces utilizáveis ​​em produção, mas a integração UI/runtime não é em si uma superfície determinística verificada |
| **DPE (Parallel Execution)** | ✅ Stable | Modelo de execução determinístico governado com RFC-DPE-0001–0009 aceitos; a semântica de época determinística está em vigor, enquanto a promoção mais ampla da superfície permanece governada pelo registro e pela cadeia RFC complementar |
| **Cognitive Tiers** | ✅ Beta | Experimental / não DCP: a Cognição da Camada 4 permanece limitada à governança, mas não é uma superfície determinística verificada |
| **TernaryOS User Environment** | ✅ Beta | Não DCP governado / beta: implementado e limitado por política, mas não apresentado atualmente como uma superfície determinística verificada |
| **Axion OS** | ✅ Alpha | Não DCP governado / alpha: arquitetura de governança ativa, mas ainda não é uma superfície determinística verificada totalmente promovida |

---

## Visão Geral da Arquitetura e Ecossistema

```text
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
│  TISC ISA v1.9.0  ❄️ Frozen  +  Data Types  ❄️ Frozen       │
│  Deterministic substrate — CanonHash81 bit-exact traces     │
├─────────────────────────────────────────────────────────────┤
│  Governed FFI (RFC-00B8)  ·  Ternary-Native Inference       │
│  FFIDispatcher · FFILibraryRegistry                         │
│  TWMATMUL · TQUANT · TATTN · TWEMBED · TERNACCUM · TACT     │
└─────────────────────────────────────────────────────────────┘
  Experimental: TernaryOS · Cognitive Tiers
```

### Componentes Principais

**TISC ISA v1.9.0** — Arquitetura de Conjunto de Instruções Ternárias. Congelado em v1.x; o contrato de execução imutável para toda a pilha.

**T81VM** — Interpretador determinístico TISC. Garante saída idêntica a bits entre plataformas; o isolamento pré-despacho do Axion mantém os ganchos de governança fora do caminho de execução ativo.

**Axion Governance Kernel** — Mecanismo de política que intercepta `AXREAD`, `AXSET`, `AXVERIFY`, opcodes de IA e chamadas FFI antes de qualquer efeito colateral. Falha com segurança (fail-closed) na falha de análise da política.

**CanonFS** — Sistema de arquivos endereçado por conteúdo. Armazena todos os objetos de código, pesos de modelo e artefatos de tempo de execução como blobs imutáveis e identificados por hash. Fornece proveniência para auditorias de determinismo.

**T81Lang** — Linguagem de alto nível visando o bytecode TISC. Tipos nativos: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`. Pipeline do compilador: lexer → parser → AST tipado → análise semântica → geração de IR.

**Ternary-Native Inference (RFC-0034)** — Seis opcodes TISC para inferência de IA livre de multiplicação usando pesos ternários balanceados {−1, 0, +1}: `TWMATMUL` (matmul), `TQUANT` (quantificar para trit), `TATTN` (atenção ternária), `TWEMBED` (incorporação de peso), `TERNACCUM` (produto escalar), `TACT` (ativação com porta de teto Axion). Formato de peso T81WTN. Frontend `foreign {}` de T81Lang completo via RFC-0036.

**Governed FFI (RFC-00B8 + RFC-0036)** — Interface de função estrangeira governada de pilha completa. Camada VM (RFC-00B8 Fase 1): `FFIDispatcher` impõe verificações de política, cotas de recursos e trilhas de auditoria antes de qualquer chamada estrangeira; `FFILibraryRegistry` rastreia bibliotecas registradas por nome e hash de versão; três opcodes de VM (`FFICall`, `FFIRegister`, `FFIPolicySet`). Camada de linguagem (RFC-0036): `foreign deterministic { fn sin(x: T81Float) -> T81Float; }` declara assinaturas; `foreign.sin(angle)` em locais de chamada diminui para `FFI_CALL` com o nome da função carregado em `text_literal`.

**TUI Frontends** — Duas interfaces de terminal complementares criadas no FTXUI v5.0.0:
- `t81 studio` — barra lateral de navegação, navegador CanonFS, painel Axion, visualizador de rastreamento de determinismo, paleta de comandos (`Ctrl+P`)
- `t81 agent` — sessão JSONL persistente, comandos de barra (`/compile`, `/run`, `/hash`, `/allow`, `/infer`, `/trits`, …), barra de probabilidade de trit

**DPE (Deterministic Parallel Execution)** — Modelo de grafo de tarefas sobre o TISC ISA congelado. As tarefas declaram entradas imutáveis e regiões de saída em buffer; a VM confirma todas as gravações atomicamente no final da época. Não são necessários novos opcodes.

---

## Início Rápido

### Construir a partir da fonte

```bash
# Clone the repository
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# Configure and build (Release mode)
cmake --preset default -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run the test suite (369 tests)
ctest --test-dir build --output-on-failure
```

### CLI e Interfaces

```bash
# Compile a T81Lang program
./build/t81 code build examples/hello.t81 -o hello.tisc

# Execute with Axion governance
./build/t81 vm run hello.tisc

# Launch the human operator TUI
./build/t81 studio

# Launch the AI-native TUI
./build/t81 agent
```

---

## Verificação de Determinismo

As superfícies determinísticas verificadas são testadas quanto à reprodutibilidade de plataforma cruzada exata de bits.

```bash
./scripts/ci/run_determinism_slice.sh
```

Plataformas verificadas para a superfície central atual: **Linux x86_64**, **macOS ARM64**. Qualquer divergência em uma superfície determinística verificada é um defeito crítico.

---

## Documentação

| Tópico | Local |
| :--- | :--- |
| Primeiros Passos (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| Primeiros Passos (IA) | `docs/user-guide/getting-started/ai-quickstart.md` |
| Guia TUI | `docs/user-guide/how-to/tui-guide.md` |
| Especificação ISA | `spec/tisc-spec.md` |
| Manual de Política Axion | `docs/user-guide/tutorials/axion-policy-manual.md` |
| Referência Stdlib T81Lang | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| Visão Geral da Arquitetura | `docs/architecture/OVERVIEW.md` |
| Carta de Governança | `docs/governance/README.md` |
| Centro de Controle do Projeto | `docs/status/PROJECT_CONTROL_CENTER.md` |

---

## Governança

A T81 Foundation opera sob um modelo de **Governança Contínua (C2)**. Todas as contribuições devem manter:

- **paridade de execução determinística** — hashes de rastreamento devem corresponder nas plataformas suportadas
- **coerência arquitetônica** — as alterações que afetam a superfície determinística exigem revisão formal
- **garantias de reprodutibilidade** — não há não-determinismo de ponto flutuante ou específico de plataforma na superfície do DCP

A superfície determinística é definida em `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. As alterações em superfícies congeladas (TISC ISA, Data Types) exigem um aumento de versão principal.

> **Nota de limite:** as classificações DCP, governadas não DCP, experimentais e fora do escopo são definidas constitucionalmente no RFC-0048. Os documentos públicos não devem apresentar superfícies governadas não DCP ou experimentais como componentes determinísticos verificados.

---

## A Vantagem Ternária

Embora o hardware binário moderno seja altamente otimizado para computação de uso geral, a **T81 Foundation** explora as propriedades matemáticas e estruturais exclusivas do **ternário balanceado** ({−1, 0, +1}) para fornecer vantagens que são difíceis ou impossíveis de alcançar em sistemas binários convencionais — especialmente em execução determinística, inferência de IA governada e cargas de trabalho neurais de baixa complexidade.

### 1. Simetria Computacional O(1) — Negação Sem Vai Um

No binário de complemento de dois, a negação requer um NOT bit a bit seguido de +1, o que pode desencadear longas cadeias de vai-um (carry). No ternário balanceado, a negação consiste simplesmente em inverter o sinal de cada trit diferente de zero (+1 ↔ −1, 0 permanece 0) — **propagação de carry zero**, tempo constante.

- **Desempenho medido**: a negação do PackedCell atinge **~49.9 G-ops/s** em hardware x86_64 recente, superando a negação de números inteiros de 64 bits otimizada em **~10.9×** (benchmarks verificados no Linux x86_64 e no macOS ARM64).
- Essa simetria se estende naturalmente a muitos padrões aritméticos, reduzindo a latência e a potência em operações pesadas em sinais.

### 2. Economia Superior de Base e Densidade Teórica

A base ideal da teoria da informação para sistemas de números posicionais está próxima de *e ≈ 2.718*. O ternário (base 3) está matematicamente mais próximo que o binário (base 2), fornecendo **~1.585 bits de informação por trit** (log₂(3)).

- Na prática, isso permite maior entropia por dígito e representação mais compacta de intervalos simétricos — especialmente valioso para pesos de redes neurais, incorporações, sistemas de coordenadas e tensores esparsos de grande escala.

### 3. Determinismo Inerente de Bits Exatos e Arredondamento Independente de Plataforma

O ponto flutuante IEEE 754 sofre de modos de arredondamento específicos de plataforma, diferenças de associatividade e tratamento de números subnormais que quebram a reprodutibilidade. A aritmética ternária balanceada é naturalmente simétrica em torno de zero:

- O arredondamento usa truncamento simples — sem viés dependente da direção.
- Cada caminho de execução produz **hashes de rastreamento idênticos** em plataformas suportadas (atualmente Linux x86_64 + macOS ARM64 verificado em 100% de paridade).
- Isso fornece reprodutibilidade sólida para computação científica, auditoria de inferência de IA e execuções de agentes governados.

### 4. Inferência Neural Sem Multiplicação

Pesos ternários balanceados {−1, 0, +1} permitem **produtos escalares sem multiplicação** — substitua MUL por ADD/SUB condicional (ou acúmulo puro ao pular zeros). Combinado com opcodes TISC personalizados (`TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT`):

- Permite uma potência drasticamente menor e um rendimento maior para inferência de IA.
- Alinha-se às tendências de 2024–2026 em modelos extremos de baixo bit (BitNet b1.58, xTern, transformadores ternários), oferecendo redução de energia de 15 a 60× e ganhos de taxa de transferência de 4 a 90× em comparação com as linhas de base de FP16/FP32 com perda mínima de precisão.
- Formato de peso T81WTN + operações nativas ternárias tornam essa vantagem pronta para produção na pilha.

### 5. Governança Arquitetônica e Ganchos de Segurança

Como todo o TISC ISA é ternário nativo, o **Axion Governance Kernel** pode interceptar e auditar transições de estado na **granularidade em nível de trit** antes que ocorra qualquer efeito colateral. Isso permite:

- Imposição de política de fail-closed em operações privilegiadas (invocações de IA, chamadas de FFI, comportamentos de agente).
- Portas éticas refinadas, rastreamento de proveniência por meio do CanonFS e trilhas de auditoria determinísticas.
- Um modelo de segurança que é fundamentalmente mais inspecionável do que a execução binária de caixa preta.

Essas vantagens se combinam em domínios em que **reprodutibilidade**, **inferência de baixa complexidade**, **execução governada** e **simetria matemática** são mais importantes — exatamente os casos de uso de destino da arquitetura T81.

---

## Licença

Licença Apache 2.0.
