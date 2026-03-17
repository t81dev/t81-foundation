<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# Fundação T81 — Pilha de Computação Ternária Determinista

![Release](https://img.shields.io/badge/release-v1.9.0--Stable-blue)
![Tests](https://img.shields.io/badge/tests-369%2F369_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.2.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

Aproveitando a eficiência teórica da computação em base-e, a **Fundação T81** é uma pilha de computação determinista construída sobre **aritmética ternária balanceada** ({-1, 0, +1}) com um modelo de governança de cadeia completa que cobre o conjunto de instruções, a máquina virtual, o compilador da linguagem e o ambiente de inferência de IA.

A pilha oferece:

- **reprodutibilidade bit a bit** — cada caminho de execução produz um hash de rastreamento idêntico nas plataformas suportadas
- **inferência de IA governada** — o mecanismo de políticas Axion intercepta e audita cada operação privilegiada antes que haja efeitos colaterais
- **proveniência endereçada por conteúdo** — CanonFS registra todos os artefatos, os pesos do modelo e o estado em tempo de execução de forma imutável
- **execução paralela determinista** — o modelo de gráfico de tarefas DPE (RFC-DPE-0002) permite cargas de trabalho TISC concorrentes com saídas comprometidas por época

---

## Status do Projeto — Março 2026

**Fase: Desenvolvimento Ativo** — v1.9.0-Stable; 369/369 testes passados; determinismo multiplataforma verificado em Linux x86\_64 + macOS ARM64.

| Componente | Maturidade | Notas |
| :--- | :--- | :--- |
| **TISC ISA** | ❄️ Frozen | v1.2.0; a semântica dos opcodes é imutável sob a v1.x; 12 novos opcodes desde a v1.1: `AgentInvoke` (RFC-0015), 6 de inferência nativa ternária (RFC-0034), 3 FFI (RFC-00B8), 2 de criptografia de rede (RFC-0038), 1 de anel KEM (RFC-0039) |
| **Data Types** | ❄️ Frozen | BigInt, Float, Complex, Map, Set — codificação estável de bits; auditoria limpa em 2026-02-27 |
| **T81VM** | ✅ Stable | Despacho completo do TISC v1.2; `AgentInvoke` + inferência nativa ternária + FFI + criptografia de rede + opcodes NTRU-KEM; 369/369 testes |
| **T81Lang** | ✅ Stable | especificação v1.3 Stable; `agent`/`behavior` (RFC-0015); `foreign {}` FFI (RFC-0036); `std.tnn.*` TNN stdlib (RFC-0037); `std.crypto.*` criptografia de rede + NTRU-KEM (RFC-0038/0039); suporte para identificadores contextuais em tudo |
| **Axion Governance Kernel** | ✅ Stable | Satisfação de Segurança P4 e P5 de Instruções Privilegiadas; cadeias de razões canônicas AX-M6; cada porta de ativação de `AgentInvoke` + `TACT` emite um evento de auditoria |
| **Ternary-Native Inference** | ✅ Stable | RFC-0034 + RFC-0037: `TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT`; `std.tnn.*` T81Lang stdlib (6 funções embutidas → opcodes TISC); inferência livre de multiplicação; formato de peso T81WTN; 13/13 testes; operações de inferência ternária prontas para produção |
| **Lattice Cryptography** | ✅ Stable | RFC-0038+0039: `POLYMUL`, `POLYMOD`, `TVecSub`; anel completo {+,−,×,mod} sobre Z\[x\]/(x^n+1); `std.crypto.{polyadd,polysub,polymul,polymod,ntru_encrypt,ntru_decrypt}`; 37/37 testes; criptografia de rede pronta para produção |
| **Governed FFI** | ✅ Stable | RFC-00B8 + RFC-0036: `FFIDispatcher`, `FFILibraryRegistry`, 3 opcodes de VM; `foreign [policy] { fn … }` gramática da T81Lang; `foreign.<name>(args)` → `FFI_CALL`; 9/9 testes AC; interface de funções estrangeiras governada pronta para produção |
| **TUI Frontends** | ✅ Beta | `t81 studio` (operador humano) + `t81 agent` (nativo em IA); FTXUI v5.0.0; RFC-0033 aceito; interfaces de terminal prontas para produção |
| **T81Graph** | ✅ Beta | Redução de opcodes da VM + serialização do lado da linguagem interligada; verificação DCP completa; 6/6 testes |
| **DPE (Parallel Execution)** | ✅ Stable | RFC-DPE-0001–0009 todos aceitos; gráfico de tarefas, anel de histórico de época, eventos de auditoria de época, tempo limite totalmente implementado; execução paralela determinista pronta para produção |
| **Cognitive Tiers** | ✅ Beta | Cognição de Nível 4 (RFC-0021): `Tier4Loop`, `SelfModel` (anel de 81 entradas), `RecursiveImprovementBounds`, `TierAwarePlanner`; 4 suítes de teste passadas; arquitetura cognitiva experimental pronta para testes beta |
| **Benchmark Suite** | ✅ Stable | RFC-00A2: desempenho da VM + validação de determinismo de CanonHash81 (`score=1.0` em todas as execuções); `t81 internal benchmark`; validação de desempenho pronta para produção |
| **TernaryOS User Environment** | ✅ Beta | RFC-00B9: t81-init, administrador de sessões, shell t81sh; 15/15 critérios de aceitação implementados; sequência de boot, ciclo de vida de sessão e arquitetura de shell operacionais |
| **Cross-Platform Determinism CI** | ✅ Stable | Fluxo de trabalho diário de GitHub Actions que compara hashes de bytecode do T81Lang em Linux x86\_64 (gcc-14) e macOS ARM64 (clang); registro de evidência auditável publicamente; validação de determinismo multiplataforma pronta para produção |
| **Hanoi VM** | ✅ Alpha | RFC-0000 §4 boot centrado em ética; escalonador determinista de 81 slots; gerenciamento de snapshots; RFC-0000 §7 superfície de comandos (status, optimize, simulate, snapshot, rollback); suite de testes completa; microkernel pronto para Alpha |
| **Axion OS Kernel** | ✅ Alpha | Sistema completo de governança com 100% de cobertura de testes (28/28 testes); motor de políticas pronto para produção e avaliação de ética; Princípios Θ₁-Θ₉ totalmente implementados; documentação completa da API e exemplos de integração; kernel pronto para Alpha com tomada de decisões determinista e total integração com a pilha T81 |

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

### Componentes chave

**TISC ISA v1.2** — Arquitetura de Conjunto de Instruções Ternárias. Congelado sob v1.x; o contrato de execução imutável para toda a pilha. v1.2 adiciona 9 opcodes: `AgentInvoke` (RFC-0015), seis operações de inferência nativa ternária (RFC-0034), e três operações FFI governadas (RFC-00B8).

**T81VM** — Interpretador TISC determinista. Garante saídas bit-idênticas entre plataformas; O isolamento de pré-despacho de Axion mantém os ganchos de governança fora do caminho de execução quente. Despacho completo do TISC v1.2 incluindo a inferência nativa ternária e FFI.

**Axion Governance Kernel** — Motor de políticas que intercepta `AXREAD`, `AXSET`, `AXVERIFY`, opcodes de IA, e chamadas FFI antes de qualquer efeito colateral. Fechado diante de falha na análise das políticas. Certificado Stable em 2026-03-15 com 54/54 testes passados.

**CanonFS** — Sistema de arquivos endereçado a conteúdo. Armazena todos os objetos de código, pesos do modelo e artefatos de tempo de execução como objetos binários grandes (blobs) imutáveis e identificados por hash. Fornece proveniência para auditorias de determinismo.

**T81Lang** — Linguagem de alto nível direcionada ao bytecode TISC. Tipos nativos: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`. Declarações de primeira classe `agent { behavior }` compila para `AGENT_INVOKE` com auditoria de Axion (RFC-0015). Os blocos `foreign [policy] { fn … }` declaram funções estrangeiras governadas que chamam por meio de `FFI_CALL` (RFC-0036). `agent`, `behavior` e `foreign` são usáveis como identificadores contextuais em todas as expressões e posições de vínculo. Pipeline do compilador: lexer → parser → AST tipado → análise semântica → geração de IR.

**Ternary-Native Inference (RFC-0034)** — Seis opcodes TISC para inferência de IA sem multiplicação utilizando pesos ternários balanceados {−1, 0, +1}: `TWMATMUL` (matmul), `TQUANT` (quantizar para trit), `TATTN` (atenção ternária), `TWEMBED` (inserção de peso), `TERNACCUM` (produto escalar), `TACT` (ativação com o portão teto de Axion). Formato de peso T81WTN. Frontend de T81Lang `foreign {}` completo através de RFC-0036.

**Governed FFI (RFC-00B8 + RFC-0036)** — Interface de funções estrangeiras governada de pilha completa. Camada da VM (RFC-00B8 Phase 1): `FFIDispatcher` aplica as verificações das políticas, cotas de recursos e trilhas de auditoria antes de qualquer chamada estrangeira; `FFILibraryRegistry` rastreia as bibliotecas registradas por nome e hash de versão; três opcodes da VM (`FFICall`, `FFIRegister`, `FFIPolicySet`). Camada da Linguagem (RFC-0036): `foreign deterministic { fn sin(x: T81Float) -> T81Float; }` declara as assinaturas; `foreign.sin(angle)` nos locais de chamada reduz para `FFI_CALL` com o nome da função contido em `text_literal`. Passam os nove testes de aceitação.

**TUI Frontends** — Duas interfaces de terminal complementares construídas em FTXUI v5.0.0:

- `t81 studio` — barra lateral de navegação, explorador CanonFS, painel do Axion, visualizador de rastreamento de determinismo, paleta de comandos (`Ctrl+P`)
- `t81 agent` — sessão JSONL persistente, comandos de barra (`/compile`, `/run`, `/hash`, `/allow`, `/infer`, `/trits`, …), barra de probabilidade trit

**DPE (Deterministic Parallel Execution)** — Modelo de gráfico de tarefas sobre a ISA de TISC congelada. As tarefas declaram entradas imutáveis e regiões de saída em buffer; a VM compromete todas as escritas atomicamente ao fim da época. Não são necessários opcodes novos.

---

## Início Rápido

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

Opções de compilação opcionais:

| Opção | Por Defeito | Propósito |
| :--- | :--- | :--- |
| `T81_BUILD_TUI` | `ON` | Interfaces frontais TUI baseadas em FTXUI |
| `T81_BUILD_TESTS` | `ON` | Suite de testes completa |
| `T81_ENABLE_ASAN` | `OFF` | Address sanitizer |
| `T81_ENABLE_UBSAN` | `OFF` | UB sanitizer |
| `T81_ENABLE_LLAMA_CPP` | `OFF` | Adaptador de inferência governado llama.cpp |
| `T81_WARN_STRICT` | `OFF` | Modo restrito de escaneamento de avisos (usado pelo preset `warn-strict`) |

**Escaneamento de aviso prévio ao push** — reflete os testes `-Wswitch`, `-Wunused-variable`, e `-Wunused-function` aplicados por Windows CI, encontrando problemas localmente em aproximadamente 2 minutos em vez de esperar a matriz inteira:

```bash
cmake --preset warn-strict
cmake --build build-warn-strict 2>&1 | head -40
```

---

## Verificação de Determinismo

Cada lançamento é verificado quanto à reprodutibilidade de determinismo bit a bit por múltiplas plataformas.

```bash
./scripts/ci/run_determinism_slice.sh
```

Plataformas verificadas: **Linux x86_64**, **macOS ARM64**. Qualquer divergência nos hashes do rastreador da VM é um defeito crítico.

---

## Documentação

| Tópico | Localização |
| :--- | :--- |
| Primeiros passos (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| Primeiros passos (AI) | `docs/user-guide/getting-started/ai-quickstart.md` |
| Guia da TUI | `docs/user-guide/how-to/tui-guide.md` |
| Especificação da ISA | `spec/tisc-spec.md` |
| Manual da Política do Axion | `docs/user-guide/tutorials/axion-policy-manual.md` |
| Referência da Stdlib da T81Lang | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| Resumo da Arquitetura | `docs/architecture/OVERVIEW.md` |
| Estatutos de Governança | `docs/governance/README.md` |
| Centro de Controle do Projeto | `docs/status/PROJECT_CONTROL_CENTER.md` |

---

## Roadmap

| Marco | Alvo | Descrição |
| :--- | :--- | :--- |
| Fecho do Mês C2 | 2026-03-31 | Auditoria do livro-razão de governança; PASS pré-vôo 2026-03-10 |
| Promoção a Axion Stable | ✅ **COMPLETADO 2026-03-15** | Cadeias de razão canônicas de AX-M6 implementadas; 54/54 testes passando; pronto para produção |
| Promoção a T81Graph Beta | ✅ **COMPLETADO 2026-03-15** | Redução de opcode da VM completada; Verificação do DCP; 6/6 testes passando |
| Política de interrupção RFC-00B5 | ✅ **COMPLETADO 2026-03-16** | Modelo de interrupção de eventos governados integrado; fatias 26-28 completas |
| RFC-0034 Inferência Ternária-Nativa | ✅ **COMPLETADO 2026-03-16** | 6 novos opcodes TISC; inferência sem multiplicação; porta teto de ativação TACT; 5/5 testes de conformidade |
| RFC-00B8 FFI Governado (Fase 1) | ✅ **COMPLETADO 2026-03-16** | Despachante FFI + registro de bibliotecas; 3 opcodes da VM; pipeline de governança; trilhas de auditoria |
| CI de Determinismo Multiplataforma | ✅ **COMPLETADO 2026-03-16** | Fluxo de trabalho diário de GitHub Actions; comparação de hash entre Linux x86\_64 + macOS ARM64; registro público de evidências |
| RFC-0036 Gramática FFI de T81Lang | ✅ **COMPLETADO 2026-03-16** | Sintaxe `foreign [policy] {}`; `foreign.<name>(args)` → `FFI_CALL`; 9/9 testes de critérios de aceitação; vincula o trabalho da VM do RFC-0034 + RFC-00B8 para o frontend da T81Lang |
| Etapa 2: Plataforma Verificada | ✅ **ALCANÇADO 2026-03-16** | Todos os objetivos de implementação completos; depurador de rastreamento de replay, CI multiplataforma, 365/365 testes, frontend FFI — pilha externamente reprodutível |
| RFC-0037 TNN stdlib | ✅ **COMPLETADO 2026-03-16** | Funções embutidas da T81Lang `std.tnn.*` (6 funções → opcodes TISC do RFC-0034); 13/13 testes; inferência sem multiplicação de ponta a ponta até a VM |
| RFC-0038 Criptografia de Rede | ✅ **COMPLETADO 2026-03-16** | opcodes TISC `POLYMUL`/`POLYMOD`; funções embutidas `std.crypto.polymul/polymod`; multiplicação polinomial negacíclica sobre {−1,0,+1}; precisão T81BigInt-exact; 13/13 testes |
| Promoção da especificação T81Lang (v1.3) | ✅ **COMPLETADO 2026-03-16** | Promoção da RFC-0036/0037/0038 para a especificação normativa; §5.17 removido do esboço; agregados §5.18/5.19; o registro de opcodes atualizado para 205 entradas |
| RFC-0039 NTRU-KEM | ✅ **COMPLETADO 2026-03-16** | opcode `TVecSub`; `std.crypto.{polyadd,polysub,ntru_encrypt,ntru_decrypt}`; camada matemática C++ para KEM; 24/24 testes; anel completo {+,−,×,mod} sobre Z\[x\]/(x^n+1) |
| TernaryOS bare-metal boot | TBD | Execução de host x86\_64 via QEMU + retorno de evidências pela CanonFS |

---

## Governança

A Fundação T81 opera sob um modelo de **Governança Contínua (C2)**. Todas as contribuições devem manter:

- **paridade de execução determinista** — os hashes de rastreio devem corresponder nas plataformas suportadas
- **coerência da arquitetura** — alterações que tocam a superfície determinista exigem uma revisão formal
- **garantias de reprodutibilidade** — não pode haver ponto flutuante nem não-determinismos específicos da plataforma na superfície do DCP

A superfície determinista é definida em `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. As alterações em superfícies congeladas (TISC ISA, Tipos de Dados) requerem um aumento na versão principal.

> **Nota de limites:** As superfícies experimentais (Camadas Cognitivas, Distribuídas, Trace-JIT, TernaryOS, adaptador de llama.cpp) não são governadas pelo DCP e não devem ser apresentadas como componentes deterministas verificados.

---

## A Vantagem Ternária

Enquanto o hardware binário moderno é altamente otimizado, a **Fundação T81** aproveita as propriedades matemáticas exclusivas do **Ternário Equilibrado ({-1, 0, +1})** para obter eficiências estruturais que o sistema binário não consegue igualar.

### 1. Simetria Computacional $O(1)$

No complemento de dois binário, negar um número é uma operação assimétrica (NÃO + 1) que requer propagação de carry. No T81, a negação é um simples trit-flip com **zero sobrecarga de carry**.

* **Desempenho:** O rendimento de negação do T81 atinge **~46.6 G-ops/s** (via `PackedCell`), superando a negação binária otimizada de 64 bits por **10.4x**.

### 2. Maior Economia Radix

Baseado no teorema de que a base mais eficiente para um sistema numérico é $e \approx 2.718$, o ternário (Base 3) é matematicamente mais eficiente que o binário (Base 2).

* **Densidade de Informação:** O T81 atinge uma densidade teórica de **1.58 bits por trit**. Isso se traduz em maior entropia por ciclo de clock e tamanhos reduzidos de armazenamento para sistemas de coordenadas e pesos de redes neurais em larga escala.

### 3. Determinismo Bit a Bit

As operações de ponto flutuante binário (IEEE 754) frequentemente sofrem de não determinismos de arredondamento específicos de plataforma. A aritmética equilibrada do T81 proporciona:

* **Simetria Inerente:** O arredondamento é executado por simples truncamento, já que o sistema é naturalmente centralizado em zero.
* **Paridade de Rastreio:** 100% de "Exatidão Roundtrip" através de todas as plataformas testadas (Linux x86_64, macOS ARM64) com divergência nula de hashes de rastreamento de VM.

### 4. Direct Governance Hook

Como o TISC ISA é nativamente ternário, o **Kernel de Governança Axion** pode auditar transições de estado com granularidade mais fina. As operações de inferência de IA podem ser interceptadas no "nível do trit" antes que ocorram quaisquer efeitos colaterais, permitindo um modelo de segurança "fail-closed" que é arquiteturalmente impossível na execução de "caixa preta" binária padrão.

---

# T81: Um Relatório Completo de Sistemas e Subsistemas

## 1. Resumo Executivo

A T81 Foundation não é um único programa. É um repositório multicamadas que combina um modelo de dados ternário, uma ISA, uma VM, uma superfície de linguagem/compilador, uma camada de governança, armazenamento endereçado por conteúdo, infraestrutura de CI e benchmarks, um esforço experimental de OS/kernel e um grande aparato de governança/documentação. Apenas a raiz do repositório mostra uma grande pegada do sistema: `.github`, `benchmarks`, `book`, `contracts`, `core`, `docs`, `experimental`, `kernel`, `lang`, `runtime`, `spec`, `src`, `tests`, `tooling`, `tools`, além de `internal`, `legacy`, `notebooks`, `pdf` e ativos multilíngues/voltados ao público. O projeto possui 3.636 commits e é principalmente construído em C++ com componentes menores de Python/CMake/Shell/C. ([GitHub][1])

O núcleo técnico mais forte é a pilha de execução centrada em determinismo: tipos de dados, TISC, o caminho do interpretador da T81VM sem JIT e o registro de determinismo / limite DCP associado. Essas áreas são explicitamente nomeadas como o "Perfil de Núcleo Determinista" (Deterministic Core Profile - DCP), vinculadas a artefatos de verificação e apoiadas por verificações de CI e caminhos de teste concretos. A visão geral da arquitetura também separa claramente o núcleo congelado/estável da periferia experimental. ([GitHub][2])

A maior fraqueza estrutural é a incoerência de status entre as superfícies de autoridade. O repositório apresenta várias narrativas de maturidade/versão conflitantes ao mesmo tempo: o README principal diz "v1.9.0-Stable; 369/369 tests", o Centro de Controle do Projeto diz "v1.4.1-Stable; 363/363 tests", o `CMakeLists.txt` indica a versão `1.3.6`, a especificação do TISC é "Version 1.1 — Stable", enquanto o README descreve "TISC ISA v1.2", e o kernel Axion aparece tanto como "Stable" na matriz de implementação quanto "Alpha" na especificação normativa e no log de progresso do SO experimental. Isso não invalida a implementação, mas reduz materialmente a auditabilidade e a credibilidade externa. ([GitHub][1])


## 2. Metodologia de Avaliação

Esta avaliação baseia-se em seis fluxos de evidência: a estrutura e o histórico da raiz do repositório; especificações normativas em `spec/`; documentos de arquitetura/governança/estado em `docs/`; superfícies de compilação e CI, como `CMakeLists.txt` e `.github/workflows/ci.yml`; painéis de maturidade, como `PROJECT_CONTROL_CENTER.md` e `IMPLEMENTATION_MATRIX.md`; e o registro de progresso do SO experimental em `experimental/ternaryos/docs/PROGRESS.md`. A própria ordem de autoridade do repositório é explícita: `/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`. ([GitHub][1])

Esta é, portanto, uma auditoria de superfície de estrutura/especificação/implementação, não uma verificação completa do código-fonte linha por linha de cada arquivo C++. Quando o próprio repositório declara o status da implementação, testes ou maturidade, eu trato isso como afirmações, a menos que sejam apoiadas por evidências adjacentes, como verificações de CI, caminhos de teste explícitos ou consistência entre documentos. Quando o próprio repositório contém contradições, eu trato isso como evidência de desvio de governança. ([GitHub][3])

## 3. Mapa de Sistema-de-Sistemas

### Fundações arquitetônicas

Objetivo: definir a arquitetura delimitada, o modelo de autoridade e o limite do núcleo determinista. Locais principais: `docs/architecture/OVERVIEW.md`, `docs/product/DETERMINISTIC_CORE_PROFILE.md`, `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`, `spec/`. Maturidade: relativamente alta como uma superfície de controle de documentação. Acoplamento: tudo depende disso. Risco principal: desvio de status entre esses arquivos e as mensagens de alto nível voltadas para o público. ([GitHub][2])

### Modelo de dados / modelo numérico

Objetivo: tipos de dados ternários nativos canônicos e codificação determinista. Locais principais: `core/types/`, `spec/t81-data-types.md`, testes relacionados listados em DCP/registro. Maturidade: entre os domínios mais confiáveis; tratados como Frozen/Verified (Congelados/Verificados) na matriz de implementação e no DCP. Risco principal: o escopo de ponto flutuante é cuidadosamente limitado, mas as mensagens públicas podem obscurecer esses limites. ([GitHub][4])

### ISA / VM / runtime

Objetivo: semântica de instrução congelada e ambiente de execução determinista. Locais principais: `core/isa/`, `core/vm/`, `runtime/`, `spec/tisc-spec.md`, `spec/t81vm-spec.md`. Maturidade: o caminho do interpretador é o substrato operacional principal; a especificação da VM permanece como Beta, mesmo que os painéis a promovam para Stable (Estável). Risco principal: desalinhamento de especificações/painéis e equivalência JIT inacabada. ([GitHub][2])

### Superfícies de linguagem / compilador

Objetivo: linguagem de alto nível determinista que compila exclusivamente para TISC. Locais principais: `lang/`, `spec/t81lang-spec.md`, dispositivos (fixtures) e portas de reprodução nomeadas nos documentos. Maturidade: mais forte do que em muitos repositórios de linguagem experimentais porque tem uma gramática normativa, descrição do pipeline do compilador e ressalvas de determinismo explícitas. Risco principal: as alegações de maturidade da linguagem são mais fortes do que as evidências de implementação inspecionadas diretamente aqui. ([GitHub][5])

### Governança / política / Axion

Objetivo: supervisionar instruções privilegiadas, vereditos de políticas e visibilidade de execução. Locais principais: `kernel/axion/`, `spec/axion-kernel.md`, documentos de governança, documentos relacionados a rastreamento. Maturidade: mista. A arquitetura é real o suficiente para moldar a VM e o discurso de políticas, mas várias alegações centrais de administração são explicitamente apenas parciais na especificação. Risco principal: a retórica de governança excede a aplicação atual em algumas áreas. ([GitHub][6])

### Armazenamento / CanonFS / modelo de objeto

Objetivo: persistência e proveniência endereçadas a conteúdo. Locais principais: `src/canonfs/`, `include/t81/canonfs/`, notas de progresso de SO suplementar e do SO Axion. Maturidade: estável com limites nos documentos de arquitetura; cenários mais ambiciosos de hóspede/boot/persistência permanecem experimentais na trilha do SO. Risco principal: alegações de proveniência de armazenamento de núcleo estável são misturadas com alegações de armazenamento/boot de protótipo mais amplas. ([GitHub][2])

### Superfícies de Kernel / SO / Axion OS

Objetivo: substrato do sistema operacional, MMU, agendador, IPC, junções de dispositivo/HAL, runtime de serviço, vias de boot. Locais principais: `experimental/ternaryos/`, `kernel/`, documentos de progresso do SO. Maturidade: protótipo hospedado avançado, mas ainda não é um sistema operacional completo. Risco principal: confusão de nomes entre o Axion-como-kernel-de-governança e o Axion-como-sistema-operacional. ([GitHub][7])

### IA / cognição / superfícies de inferência

Objetivo: inferência ternária nativa, camadas cognitivas, superfícies de runtime/agentes governadas, FFI, alguns limites de experimento de IA. Locais principais: `experiments/ ai`, `experimental/`, recursos referenciados na RFC no README/especificações. Maturidade: mista; o trabalho do opcode de inferência parece substancialmente implementado, enquanto a camada cognitiva e ambições mais amplas de AGI/governança permanecem parcialmente especulativas ou explicitamente não-DCP. Risco principal: sobreextensão. ([GitHub][1])

### Testes / benchmarks / aplicação de CI

Objetivo: aplicar a integridade estrutural, as alegações de determinismo, o alinhamento de especificações/documentos e as portas de benchmark. Locais principais: `.github/workflows`, `benchmarks/`, `tests/`, `scripts/ci/`, `scripts/governance/`. Maturidade: forte em relação à etapa do projeto. Risco principal: a amplitude da CI é substancial, mas alguns trabalhos são informativos e os números públicos são inconsistentes entre os documentos. ([GitHub][1])

### Documentação / comunicação pública

Objetivo: especificações, auditorias, painéis, livro, superfícies do README multilíngues. Locais principais: `docs/`, `book/`, traduções do README. Maturidade: muito alta em quantidade, mista em integridade de sincronização. Risco principal: dispersão de documentação e camadas de autoridade concorrentes. ([GitHub][1])

### Legado / interno / notebooks / pdf / arquivo

Objetivo: histórico, artefatos de suporte, material exploratório, material gerado. Locais principais: `legacy`, `internal`, `notebooks`, `pdf`, `artifacts/archive`. Maturidade: incerta por design. Risco principal: superfícies mortas, status de suporte incerto, confusão do colaborador. ([GitHub][1])

## 4. Inventário Detalhado de Subsistemas

### Tipos de Dados T81

Objetivo: semântica de coleção e numérica canônica. Locais: `core/types/`, `spec/t81-data-types.md`, testes nomeados em DCP/registro. Status: implementado, verificado e tratado como congelado. Evidência de verificação: `v1_canonical_numeric_contract_test.cpp`, `tisc_binary_io_determinism_test.cpp`, mais notas de auditoria na matriz de implementação. Relevância do determinismo: fundamental. Relevância da governança: alta porque a canonização e a repetição da política dependem de representações estáveis. Preocupação principal: as mensagens de ponto flutuante devem permanecer limitadas. ([GitHub][4])

### TISC ISA

Objetivo: contrato de execução imutável. Locais: `core/isa/`, decodificação/despacho da VM, `spec/tisc-spec.md`. Status: núcleo operacional, mas os relatórios de status/versão são inconsistentes: a matriz de implementação o trata como congelado/verificado, o README fala sobre a v1.2, enquanto a página de especificação indica “Versão 1.1 — Stable” e o CI ainda faz referência a “Verificar a integridade do congelamento do TISC v1.1.0”. Relevância do determinismo: máxima. Relevância da governança: a visibilidade do Axion é normativa na especificação. Preocupação principal: a integridade do congelamento é fortemente governada, mas a contabilidade da versão não é apresentada de forma coerente. ([GitHub][4])

### Interpretador T81VM

Objetivo: ambiente de execução determinístico para TISC. Locais: `core/vm/`, `spec/t81vm-spec.md`. Status: quase central e tratado como estável pelos painéis, mas ainda em fase Beta na especificação normativa. Evidências de verificação: testes de rastreamento de máquina virtual e propriedade de determinismo, inclusão em DCP, menções de conformidade. Relevância do determinismo: máxima. Relevância da governança: integração explícita com o Axion. Principais preocupações: a função pública `get_execution_mode()` não está exposta; os eventos de agendamento ainda não são entradas de rastreamento de primeira classe; as garantias arquiteturais do ponto flutuante estão limitadas de forma explícita. ([GitHub][8])

### T81Lang

Objetivo: linguagem de alto nível determinista que compila exclusivamente para TISC. Locais: `lang/`, `spec/t81lang-spec.md`, scripts de testes/reproducibilidade nomeados no registro. Status: um dos subsistemas mais consistentes pela documentação e estado declarado de promoção. Evidência de verificação: configurações deterministas, testes de reprodução, gramática completa, descrição do pipeline de compilação. Relevância do determinismo: alta, mas com advertências explícitas sobre o comportamento transcendental/de ponto flutuante dependente do host. Relevância da governança: consciência de camada, pureza/efeitos, visibilidade do Axion. Principais preocupações: as afirmações sobre a maturidade da linguagem são mais fortes do que as evidências de implementação diretamente inspecionadas neste caso. ([GitHub][5])

### Kernel de Governança Axion

Objetivo: política e camada de supervisão de execução. Locais: `kernel/axion/`, `spec/axion-kernel.md`. Status: parcialmente real e parcialmente ideal. A matriz de implementação a promove para a versão "Stable" com aprovação em 54/54 testes, mas a especificação normativa ainda a categoriza como "Alpha" e admite explicitamente implementação parcial da governança determinista e métricas de complexidade incompletas. Relevância do determinismo: alta. Relevância da governança: é o centro da governança. Preocupação principal: a arquitetura é significativa, mas o idioma "juiz final" está adiante da capacidade de detecção implementada. ([GitHub][4])

### CanonFS

Objetivo: armazenamento de conteúdo imutável com proveniência de histórico. Locais: `src/canonfs/`, `include/t81/canonfs/`, documentação suplementar de especificações. Status: com limites estáveis na visão geral da arquitetura e em documentos associados com DCP; reinvindicações de persistência e boots mais avançadas ainda estão no escopo do SO Experimental. Evidência de verificação: caminhos da fonte citados e testes de fase do sistema operacional. Preocupação principal: o repositório confunde recursos de procedência estáveis de armazenamento e inicialização e armazenamento hospedado, que são obviamente protótipos de longo prazo. ([GitHub][2])

### Perfil de Núcleo Determinista / Registro

Objetivo: delimitar o critério de certificação de garantia e superfícies determinísticas verificadas. Locais: `docs/product/DETERMINISTIC_CORE_PROFILE.md`, `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. Status: uma máquina de governança real, além da pura retórica de marca. Evidências de verificação: testes e aprovação de pipeline de mapeamento na configuração no CI correspondente a essas superfícies e os mapeamentos nomeados da aplicação contínua. Principais preocupações: um limiar excelente sob a perspectiva da engenharia e uma documentação de orientação geral difusa na publicidade principal do repositório, com os termos não especificados. ([GitHub][9])

### Ferramentas de CI / Governança

Objetivo: garantir as asserções de estabilidade e estrutura, aderência à prova ao DCP/determinismo e congelamentos para referências em aprovações automáticas. Locais: `.github/workflows/ci.yml`, `scripts/ci/`, `scripts/governance/`. Status: impressionante, atipicamente potente. Principais preocupações: apesar de amplamente notável o rigor desse rastreamento sobre seu ambiente de aprovações informacionais de produção, ainda é preciso um cuidado para observar de modo analítico esses caminhos para separar apenas do CI em execução de metadados como comprovações conclusivas exclusivas deste contexto da aplicação de CI. ([GitHub][3])

### Benchmarks

Objetivo: marcos para fluxo de execução/trabalho e desempenho. Locais: `benchmarks/`, bloqueios de benchmark em CI e as afirmações base no README. Status: funcional, serve a seus bloqueios configurados neste estágio de forma operacional com o limite do benchmark que os relata como uma compilação de resultados integrados na avaliação final da liberação/estável; Preocupação principal: as indicações sobre as propostas no formato do "tipo marketing", nas frentes de performance mais destacadas pelo documento e publicadas pelas afirmações documentadas superam ou abrangem e extrapolam os achados empíricos sobre a evidência analisada. ([GitHub][1])

### Axion OS / TernaryOS

Objetivo: kernel base com OS em formato Host, suporte e arquitetura MMU/IPC, linhas de ambiente baseadas e voltadas aos agendadores, runtimes operacionais baseados em sistemas. Locais: `experimental/ternaryos/`, `kernel/` no repositório geral e as documentações no formato "log" de progressão. Status: este componente conta com avaliações técnicas bem definidas a passos consistentes da implementação que pode ser descrita e demonstrada pelo repositório como algo prático real na arquitetura e hospedagem como um framework em simulação, mas ainda experimental. Principais preocupações: O termo "Axion-como-kernel-operacional" em paralelo com o "Axion-como-kernel-governança". São entidades com caminhos que podem confundir as diretrizes se não se definirem como independentes do projeto principal em uso. ([GitHub][7])

### TUI / CLI / Ferramentas

Objetivo: frontend e comandos do agente, tutoriais de uso, CLI geral a implementações na linha de interface com operador em desenvolvedor. Locais: `tools/`, `tooling/`, `examples/`, a documentação do TUI em parâmetros nas partes no formato `T81_BUILD_TUI` no script do build da documentação. Status: um campo bem operado capaz de ser publicado, embora existam algumas características de usabilidade que não representam algo inerente da parte estrutural da arquitetura do repositório/documentos, servindo de acessório para testes e comandos na forma operada do projeto. Principais preocupações: uma documentação extensiva que em termos de prioridades não é primária ao nível e tamanho que pode dificultar na sincronia da implementação geral arquitetural mais importante e seus desenvolvimentos da estrutura central determinista de governança no sistema T81. ([GitHub][10])

### Experimental / Distributed / Cognitive Tiers / IA

Objetivo: caminhos operacionais baseados fora dos padrões rigorosos em DCP e na linha referenciada experimental com recursos e inferência operacionais experimentais. Locais: diretórios `experimental/`, `experiments/ ai`, e outros focos aos limites de testes referenciados nestas pautas distribuídas e de camada (Tiers) nas linhas principais e DCPs e registros. Status: referenciadas e baseadas explicitamente para documentação externa do projeto determinista de fato e tratadas sob esta premissa de que essas informações documentam as etapas e funções externas das frentes na fase não verificadas como componentes determinísticas, mantidas sob esta métrica. Principais preocupações: essas linhas parecem cobrir níveis maiores nos requisitos e promessas de implementação além do que hoje já se estabelece como a fundação de código na evidência concreta documentada com sucesso e implementada nesta camada inicial atual de operação na governança. ([GitHub][1])

## 5. Avaliação da Coerência da Arquitetura

Existe um modelo estruturado e hierárquico claro. O panorama estrutural destaca um nível escalonado em arquitetura, com os modelos deterministas desde a linguagem inicial (T81Lang), passando a sua interpretação nativa (TISC e T81VM) baseada sob controle (Axion e CanonFS). De forma consistente são destacados o perfil DCP, que explicita com registros a restrição de garantia na sua superfície para assegurar a autenticidade certificada e o status da verificação experimental em partes do núcleo não testados em conformidade fora deste nível (Não DCP). Uma coerência interna admirável nestas pautas base. ([GitHub][2])

Porém as áreas adjacentes na sua própria fundação mostram os limites não tão concretamente implementados com as definições aplicadas entre esses subsistemas. Encontramos o termo Axion para determinar partes nucleares base e em instâncias operacionais autônomas; A versão Beta e documentação do VM estão rotuladas com as credenciais em painéis com o grau Stable nas fases documentadas e nas especificações e implementações Alpha ou no uso do nome `ternaryos` de progressões na linguagem nas avaliações. Estas variações na documentação comprometem e afetam a confiabilidade autônoma no grau autodescritivo. ([GitHub][6])

O ponto frágil entre esses aspectos na arquitetura, além do nome/modelo base na estrutura, se mostra de um caráter que ainda apresenta rastros ou dados diretos (como os "eventos na execução VM" indiretamente avaliados ou nas restrições "Axion" referenciadas como partes sem instrumentação de código, apesar da presença ou exigência nas implementações referenciadas de rastreios). ([GitHub][8])

As ligações mais efetivas nestas características são a demarcação base no DCP na estrutura e do núcleo, mas as interfaces internas ou os recursos mais ambiciosos na implementação em hardware ou cognitivo e suas implementações reais divergem na proporção documental das estruturas e métricas e versões operacionais de controle da documentação de progresso atual no projeto. ([GitHub][1])

## 6. Avaliação de Determinismo e Reprodutibilidade

O conceito determinista aplicado não é puramente marketing documentado neste contexto; existem fortes frentes verificadas e codificadas nesta estrutura com registros e delimitações bem desenhados e avaliados em sua abrangência para exclusões específicas listadas no seu registro do nível do DCP. ([GitHub][1])

O uso desta frente no escopo geral com testes implementados no código e avaliações aplicadas na semântica operacional no ISA TISC, os interpretadores na integração do nível na avaliação nativa e funções de formatações do flutuante com uso seguro demonstrado validando as alegações baseadas nestas configurações nos workflows ou na integração em processos CI nas bases multiplataforma das execuções em verificações de portas documentadas com provas reais nos processos descritos neste formato. ([GitHub][11])

E do mesmo modo há limites bem demarcados nos testes nos JIT e em execuções dependentes ao uso em FPU, com exclusões assumidas nas redes no processador com limitações, mantidas no padrão verificado que determinam com clareza na transparência destas exclusões aplicadas documentais ao projeto T81Lang e TISC em testes em implementações. ([GitHub][11])

A fragilidade ou desequilíbrio na avaliação recai nas mensagens da promessa comercial frente à veracidade das bases com os números desencontrados nos documentos para estabelecer a integridade em métricas, versões com dados ou os processos documentados publicamente nestas alegações da reprodutibilidade determinística e nas estatísticas com referências a bases exatas das publicações nos diferentes quadros comparados com sua avaliação de lançamento. ([GitHub][9])

## 7. Governança, Segurança e Avaliação da Camada de Política

Existem regras operacionais fortes. Estes não são processos meramente orientados para o discurso do documento base no modelo README. Axion como a presença desta avaliação e controle do núcleo a testes de auditoria na infraestrutura da interface de máquina ou operações está descrita como uma documentação das bases no limite DCP em testes CI reais implementados. ([GitHub][12])

Embora seja documentado nas políticas, há lacunas nas restrições operacionais completas listadas e nas implementações a serem cumpridas pelas políticas determinísticas descritas (detecção implementada parcialmente nas estruturas deterministas, avaliação e administração do determinismo "parciais"). Ainda não se justifica por completas implementações operativas práticas e integradas no modelo "juízo autônomo" das descrições do Axion no nível de testes como propostas reais. ([GitHub][6])

Sua principal avaliação crível no projeto não reside na proposta moral desta governança (éticas do modelo), mas nas operações a nível restritivo de comandos ou nas ações interceptadas implementadas, que no âmbito superior (como os testes da implementação na camada de AIs ou do processo cognitivo e inferencial) ainda operam nas etapas a serem fundamentadas empiricamente nestas detecções avaliadas nos mecanismos incompletos. ([GitHub][6])

## 8. Verificação da Realidade da Implementação

A integração real aplicável documentada de forma sólida está ancorada nos componentes essenciais na estrutura base e do limite do DCP: nos testes nas operações T81VM, no formato ISA em uso restrito de suas estruturas de testes integrados e dados de reproduções aplicáveis e as especificações documentais nas regras Axion de segurança do pipeline na implementação CI operacional em fluxo validado na estabilidade no núcleo congelado. ([GitHub][9])

Áreas limitadas na operação avaliada com desenvolvimentos em fases integradas estão as aplicações do serviço SO, as partes mais abrangentes na infraestrutura com documentações em especificações parciais do Axion e referências ou painéis desatualizados entre a comunicação referenciada da integração implementada comparadas com a documentação do processo referenciado de versão estável nos comandos aplicados e nos controles implementados na rotina prática destas avaliações em status no painel avaliativo no projeto. ([GitHub][6])

Em um horizonte com protótipos listados mas documentados como referências nas propostas com os limites das ambições em projetos na fase de JIT em replicação em formato de teste em hardware ou do sustrato independente (SO em execuções de testes amplos com simulações e nas propostas base em um ambiente a operação da computação na escala "AIs ou hardware independente/nativa no mundo exterior"). Elas não estão avaliadas de modo focado no limite documentado em sua estrutura de forma operacional de base em uso com sua restrição explícita (e confirmadas na restrição no DCP avaliado) nestes painéis. ([GitHub][9])

Pode ser confirmado: a fundação documentada, em especial o código em ISA TISC, tipos operacionais, implementações do interpretador e partes sólidas da governança CI/teste. Uma fase em estruturação promissora para desenvolvimentos com testes, e as ambições não aplicáveis de base neste nível de estruturação com os módulos de operações (OS generalista completo e propostas com Hardware nativo). ([GitHub][4])

## 9. Testes, Verificação e Avaliação de CI

Este campo tem rigor comprovado na documentação de pipeline testado do repositório em sua versão avaliada no GitHub e os scripts na governança. Há referências de validações, na coerência de estruturas nas restrições no formato determinístico e de avaliações em fluxos com a limitação DCP no benchmark e a validação do formato ISA nas aprovações de integridade congelada testada nos processos. Estes parâmetros ultrapassam níveis convencionais experimentais. ([GitHub][3])

Com base nos registros verificados e mapeados nestas estruturas referenciadas em suas aplicações de testes específicos nomeados e validados no fluxo DCP/Registros, que se destacam de avaliações empíricas generalizadas nas provas avaliadas documentais. ([GitHub][11])

Pontos críticos com lacunas: a omissão dos dados de avaliações para resultados na reprodução (em testes em modo do compilador referenciados com falha parcial descritos e o JIT nas propostas excluídas); testes com eventos operacionais com ausências na visibilidade com descrições das lacunas dos recursos no nível avaliado de implementações na própria especificação e os painéis referenciais nas avaliações descritos nestes processos não coerentes documentados da aplicação e controle. ([GitHub][3])

## 10. Documentação e Avaliação de Integridade da Especificação

Este é o limite da clareza e de riscos aplicados do nível abrangente na estrutura de arquivos de documentos do T81. Há uma hierarquia referenciada nas informações, com os centros descritos para centralização da implementação na governança nos mapeamentos nestes quadros propostos como as ordens estabelecidas (Ex: os graus nas autoridades do documento overview na arquitetura base e na matriz). Um forte sistema arquitetônico nestas prioridades propostas. ([GitHub][2])

O descompasso dos alinhamentos nesta base cria confusão. O README referenciado na versão/testes nos totais, com conflitos de versão do `CMake`, de especificações de versões das notas no painel "Stable/Alpha", do status nos especificadores TISC ou T81VM beta, contra o status no Axion como estável e com dados referenciados ao OS nestes alinhamentos que confundem os níveis nas métricas documentadas ou com base aos usuários em avaliação da credibilidade destas atualizações em documentação nos dados aplicáveis. ([GitHub][1])

Este processo exige cuidado nos alinhamentos por um novo operador ou parceiro em pesquisa. As diversas referências multilingues nos `docs`, `spec`, os `notebooks` e o amplo registro e quadros avaliativos nestas páginas dificultam esta avaliação com sua falta de coesão nos dados aplicados que se mostra complexo de mapear num fluxo simples e conciso nos recursos propostos neste ecossistema de avaliação documental. ([GitHub][1])

## 11. Avaliação do Kernel / OS / Axion

A aplicação nas etapas aplicadas ao protótipo operacional referenciado nesta matriz está confirmada na descrição em MMU, IPC com propostas, gerenciamentos nas vias de processo do pager, de interrupções de governanças e boot no modo de rotina nas provas dos testes e de marcos com especificações em código implementadas para esta estrutura. A abordagem desta documentação de engenharias nos painéis apresenta o SO avaliado como uma implementação séria com os logs da avaliação aplicada neste nível de andamento. ([GitHub][7])

Contudo ele é referenciado nesta aplicação sob bases em hospedeiros no nível QEMU, os fluxos com avaliações no ambiente simulado na hospedagem, e processos em referências na virtualização da estrutura aplicável das evidências e processos de validação de artefatos com adaptadores do formato aplicável referenciados em teste avaliativo com processos base. O estado operável independente e da aplicação final completa não é a sua métrica validada para a base operacional final. ([GitHub][7])

Os recursos em colisão de nomes entre o Axion na avaliação do ambiente OS nestas páginas na integração versus o "Axion Kernel", definido como governança no nível no ambiente da supervisão, prejudicam a referência nestas especificações ou abordagens propostas que necessitariam uma divisão nestas abordagens nominais com maior clareza de contexto e uso com a coerência das especificações abordadas. ([GitHub][7])

Este projeto apresenta-se assim no desenvolvimento experimental de nível prototipado na arquitetura, hospedado com uso em sustratos nestas integrações testadas como avaliadas de modo focado em uso OS. ([GitHub][7])

## 12. Avaliação de Linguagem / VM / Pilha ISA

O TISC está confirmado na especificação referenciada e operacional neste projeto na avaliação de execução determinista e a visibilidade de restrições descritas a Axion com compatibilidades especificadas e aplicadas nos tipos e bases no nível do T81VM e de testes no compilador do ecossistema e em registros na limitação e congelamentos nos controles do ambiente e comportamento das linguagens nestas interações documentais. ([GitHub][12])

A máquina virtual, a base nas avaliações do núcleo interpretador T81VM da execução e do nível no DCP e avaliações dos testes em simulações aplicadas do controle nas restrições no formato determinístico excluindo das métricas nas compilações aplicáveis nas especificações da reprodução referenciada e assumindo em sua integridade com honestidade referenciada os processos faltantes para as provas documentadas nas descrições de sua integridade testável. ([GitHub][8])

A estrutura descrita no compilador com a base T81Lang e seu nível normativo na infraestrutura com as restrições explícitas de sua gramática nas especificações aplicáveis nas declarações dos controles das matrizes. Neste processo, o avanço e as propostas dos ecossistemas em nível cognitivo referenciados das visões das estruturas implementadas ou "estável" nos agentes das comunicações externas devem ser compreendidos como níveis ainda não certificados no padrão documentado de avaliação final aplicada destas bases em sua aplicabilidade generalizada no uso destas provas sem a restrição aplicável em ambientes fechados com este rigor no uso destas instâncias externas da arquitetura. ([GitHub][5])

O modelo integrado em dados, formato TISC e não JIT em operações com a T81VM constitui a estabilidade de processos em uso destas premissas no aval de aplicação crível neste contexto operacional do código experimental fundamentado no repositório. ([GitHub][9])

## 13. Pesquisa vs Avaliação de Produtização

Este estágio com níveis no repositório com propostas entre os processos documentais implementados referenciados neste experimento na infraestrutura técnica desenvolvida não a caracteriza ainda em aplicações e avaliações definitivas como uma "infraestrutura", tendo as validações e testes DCP de estabilizações como comprovação do esforço nas disciplinas implementáveis e das ferramentas com base em sua avaliação operacional. ([GitHub][3])

Com as etapas na estabilidade nas implementações documentadas desta base para o formato referenciado no ecossistema e no estágio da fundação dos dados propostos e das certificações testadas independentes e limites aplicáveis das bases exploratórias, este processo exigirá atualizações nestas documentações aplicadas nas restrições documentadas na base verificável nas avaliações destas estruturas a fim de um aprimoramento no estágio definitivo para aplicações de plataformas de produção em ambientes testáveis em formato reprodutível externo nas avaliações do projeto referenciado no projeto avaliativo aplicável. ([GitHub][9])

## 14. Análise SWOT

**Pontos Fortes**
Um modelo real em integração no ambiente determinístico e controle da infraestrutura de CI avaliada neste contexto operacional das informações testáveis com os perfis de implementações de uso e com uma abordagem de documentação do nível DCP delineado em uso das informações documentadas referenciadas nestes logs das integrações base nas informações documentadas nestas propostas. ([GitHub][2])

**Pontos Fracos**
Incoerências nestes relatórios e status em painéis documentados de níveis e especificações ou controles nos arquivos. Uma proposta na escala referenciada experimental com a divergência de escopo em informações destas atualizações base nos níveis operacionais e dos testes em OS cognitivo ou do hardware na base da governança nestes recursos em comparação de atualizações no ambiente de testes destas propostas. ([GitHub][13])

**Oportunidades**
O perfil operacional validado em certificação destas propostas base da governança pode integrar-se em aplicações restritas do determinismo operacional como validação dos projetos experimentais no projeto referenciados para uso em pesquisa em linguagens no nível testado e de suas simulações aplicáveis destas propostas validadas nestes ecossistemas avaliativos e com as exclusões no limite referenciado nestes painéis de pesquisa. ([GitHub][9])

**Ameaças**
A credibilidade nestes registros devido aos conflitos nos relatórios do repositório pode prejudicar as informações da reprodutibilidade base nas estatísticas das informações publicadas de níveis do uso ou testes das métricas. O escopo referenciado destas propostas (Hardware, AI, Livros, painéis múltiplos, etc.) torna difícil na avaliação da manutenção e no rastreio integrado dos níveis base na versão aplicável destas provas de processos com documentações na estrutura de documentações. ([GitHub][1])

## 15. Registro de Risco

| Risco | Sistemas afetados | Gravidade | Evidência | Mitigação |
| ---------------------------- | ------------------------------------------------------------------------------ | -------: | ----------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- |
| Desvio de status/versão | Todo o repositório | Crítica | README `v1.9.0` / 369 testes vs Control Center `v1.4.1` / 363 testes vs CMake `1.3.6` | Crie uma única fonte de verdade para versão, estado de liberação e totais de testes; gere documentos downstream a partir dela. ([GitHub][1]) |
| Incompatibilidade de especificação/implementação | ISA, VM, Axion | Crítica | Especificação TISC 1.1 vs README TISC 1.2; Especificação da VM Beta vs painel Stable; Especificação do Axion Alpha vs matriz Stable | Adicione verificações automáticas de sincronização de status para versão/estado de especificação vs painéis vs README. ([GitHub][12]) |
| Risco de teatro de governança | Axion, níveis cognitivos | Alta | A especificação do Axion diz que a administração do determinismo é parcial; métricas de complexidade são parciais | Restrinja as alegações públicas a pontos de imposição e ganchos implementados e verificados apenas. ([GitHub][6]) |
| Sobre-extensão | Experimental, IA, SO, hardware | Alta | DCP exclui muitas superfícies externas, enquanto README/roadmap abrange hardware, nuvem determinística, cognição | Congele o marketing de superfície externa; publique um mapa de "núcleo vs pesquisa" nos documentos principais. ([GitHub][9]) |
| Confusão de terminologia/nomenclatura | Axion, TernaryOS, kernel | Alta | Log de progresso diz SO Axion com `ternaryos` interno; especificação usa Axion Kernel como camada de supervisão | Renomeie ou coloque prefixos consistentes no kernel de governança vs kernel do sistema operacional. ([GitHub][7]) |
| Dispersão experimental | `experimental`, `experiments/ ai`, `legacy`, `internal`, notebooks/pdf/archive | Alta | Grande superfície de repositório com muitos diretórios de status de suporte incerto/inativo | Publique a taxonomia do status de suporte por diretório de nível superior: núcleo mantido, suporte mantido, experimental, legado, arquivado, apenas interno. ([GitHub][1]) |
| Pontos cegos de teste | Agendamento de VM, reprodução do compilador, JIT | Média | Eventos de rastreamento de agendamento não são de primeira classe; emissão do compilador é parcial; JIT é excluído do DCP | Promova superfícies de rastreamento ausentes para testes de primeira classe antes de prosseguir com a expansão de recursos. ([GitHub][8]) |
| Risco de manutenção | Documentos + código + painéis | Média | Pesada rede de documentação com vários painéis de status e camadas de autoridade | Gere matrizes/painéis a partir de metadados legíveis por máquina. ([GitHub][2]) |
| Risco de integração | Novos colaboradores | Média | Amplitude da árvore principal e camadas concorrentes de documentos/especificações/livros/centros de controle | Adicione o mapa do caminho do colaborador: “onde confiar primeiro, onde ainda não confiar”. ([GitHub][1]) |
| Risco de credibilidade | Parceiros/financiadores externos | Crítica | Marca determinista/auditável prejudicada por inconsistência interna | Trate os defeitos de sincronização como defeitos de bloqueio de lançamento. ([GitHub][1]) |

## 16. Painel de Maturidade

As pontuações são minha síntese das especificações inspecionadas, painéis, CI, DCP/registro e documentos de progresso do sistema operacional. Elas são julgamentos de avaliação, não números fornecidos pelo repositório. ([GitHub][2])

| Domínio | Clareza conceitual | Profundidade de implementação | Evidência de teste | Estabilidade da interface | Clareza da governança | Prontidão operacional | Integridade da documentação |
| ---------------------------- | -----------------: | -------------------: | ------------: | ------------------: | -----------------: | --------------------: | ----------------------: |
| Tipos de Dados | 5 | 4 | 4 | 5 | 4 | 4 | 4 |
| TISC ISA | 5 | 4 | 4 | 5 | 4 | 4 | 3 |
| T81VM | 4 | 4 | 4 | 3 | 4 | 4 | 3 |
| T81Lang | 4 | 3 | 3 | 3 | 4 | 3 | 4 |
| Governança Axion | 4 | 3 | 3 | 3 | 4 | 3 | 3 |
| CanonFS | 4 | 3 | 3 | 3 | 3 | 3 | 3 |
| CI / ferramentas de governança | 4 | 4 | 4 | 4 | 5 | 4 | 4 |
| Axion OS / TernaryOS | 4 | 3 | 4 | 2 | 3 | 2 | 3 |
| IA / cognitiva / distribuída | 3 | 2 | 2 | 2 | 3 | 1 | 3 |
| Documentos / comunicações | 4 | 4 | 3 | 2 | 4 | 3 | 2 |

Perfil de maturidade ponderado geral: **3.4 / 5**. Isso corresponde a uma plataforma experimental séria com um núcleo determinístico confiável, mas com ambiguidade suficiente na camada externa e desvio de governança/documentação para bloquear uma classificação mais forte de "pronto para infraestrutura". ([GitHub][9])

## 17. Recomendações Estratégicas

### Prioridades imediatas (0–30 dias)

Unifique a autoridade de versão/status. Escolha uma fonte canônica para o número de lançamento, maturidade da especificação, maturidade do subsistema e totais de teste; gere o README, o Control Center, a Implementation Matrix e os metadados de compilação a partir dela. Até que isso seja feito, toda declaração “estável” deve ser tratada como provisória. ([GitHub][1])

Renomeie os dois conceitos de Axion. Um é um kernel de governança/mecanismo de políticas; o outro é um sistema operacional experimental. Eles precisam de nomes diferentes voltados para o usuário ou prefixos rigorosos. ([GitHub][6])

Publique um índice de status de suporte do repositório por diretório de nível superior: núcleo mantido, suporte mantido, experimental, legado, arquivado, apenas interno. A árvore é muito grande para deixar implícito. ([GitHub][1])

### Prioridades de curto prazo (1–3 meses)

Aperte as regras de promoção de especificações para implementação. Um subsistema não deve ser chamado de Estável nos painéis enquanto sua especificação normativa permanecer como Beta/Alfa, a menos que o repositório distinga explicitamente "implementação estável, especificação pendente". No momento, ele não faz isso de maneira limpa. ([GitHub][8])

Termine as superfícies de observabilidade ausentes no limite de VM/Axion: consulta direta no modo de execução, eventos de rastreamento de agendamento de primeira classe e o que restar da cadeia de razão canônica e trabalho de detecção de não determinismo. Esses são pontos de alavancagem porque convertem a governança de filosofia em instrumentação. ([GitHub][8])

Separe as notas de versão do DCP das notas de versão do ecossistema. O núcleo merece uma disciplina de lançamento enxuta e certificável. O ecossistema mais amplo merece um log de progresso de pesquisa. Misturá-los embota ambos. ([GitHub][9])

### Prioridades de médio prazo (3–12 meses)

Busque a reprodução externa independente do núcleo determinístico, porque o próprio repositório cita isso como o critério de avanço restante para a verificação do estilo do Estágio 2. Isso faria mais pela credibilidade do que outra camada de painéis. ([GitHub][1])

Para o esforço do sistema operacional, escolha o próximo teste prático: ou "protótipo hospedado com simulação rigorosa de dispositivo/inicialização" ou "substrato de kernel real em um alvo restrito". No momento, ele está progredindo de forma responsável, mas ainda por meio de muitas vias em estágios. Uma meta de aceitação mais restrita aguçaria o programa. ([GitHub][7])

Para o perímetro de IA/governança, pare de promover visões cognitivas/distribuídas/de hardware como quase pares do núcleo. Mantenha-os em canais de pesquisa explicitamente não-DCP até que eles tenham a mesma densidade de evidência que a pilha de VM/ISA/tipo de dados. ([GitHub][9])

## 18. Veredito Final

Atualmente, a T81 Foundation é, de forma mais crível, **uma rigorosa plataforma de computação determinística experimental com um núcleo de modelo de dados/ISA/VM real e uma superestrutura de documentação/governança incomumente elaborada**. ([GitHub][2])

Seu ativo técnico mais forte é **o perfil de núcleo determinístico delimitado e o fato de que o repositório realmente tenta vincular declarações a testes nomeados, verificações de CI e exclusões explícitas de escopo**. ([GitHub][9])

Sua maior fraqueza estrutural é a **incoerência de status em especificações, painéis, README e metadados de construção**. Em um repositório normal, isso é irritante. Em um repositório construído em torno de determinismo e auditabilidade, é corrosivo. ([GitHub][1])

A única mudança que mais melhoraria sua trajetória é **transformar a própria sincronização em uma superfície determinística de primeira classe**: um gráfico de autoridade, um registro de maturidade gerado, uma verdade de lançamento e uma separação rígida entre o núcleo certificável e a fronteira exploratória.

[1]: https://github.com/t81dev/t81-foundation/ "GitHub - t81dev/t81-foundation: O T81 é uma arquitetura computacional unificada, determinística e nativa em ternário projetada para superar as limitações da computação binária. · GitHub"
[2]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/architecture/OVERVIEW.md "raw.githubusercontent.com"
[3]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/.github/workflows/ci.yml "raw.githubusercontent.com"
[4]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/status/IMPLEMENTATION_MATRIX.md "raw.githubusercontent.com"
[5]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/t81lang-spec.md "raw.githubusercontent.com"
[6]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/axion-kernel.md "raw.githubusercontent.com"
[7]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/experimental/ternaryos/docs/PROGRESS.md "raw.githubusercontent.com"
[8]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/t81vm-spec.md "raw.githubusercontent.com"
[9]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/product/DETERMINISTIC_CORE_PROFILE.md "raw.githubusercontent.com"
[10]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/CMakeLists.txt "raw.githubusercontent.com"
[11]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/governance/DETERMINISM_SURFACE_REGISTRY.md "raw.githubusercontent.com"
[12]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/tisc-spec.md "raw.githubusercontent.com"
[13]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/status/PROJECT_CONTROL_CENTER.md "raw.githubusercontent.com"

---

## Licença

MIT License.
