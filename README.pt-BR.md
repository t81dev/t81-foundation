<p align="center">
  <img src="docs/assets/banner.png" alt="T81 — A Ternary Operating System for AI" width="100%">
  Visualização inicializável no QEMU · Inferência ternária governada · Bit exato entre plataformas
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation

![Lançamento](https://img.shields.io/badge/release-v1.9.2--Stable-blue)
![Testes](https://img.shields.io/badge/tests-404%2F404_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue)
![Execução](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![Licença](https://img.shields.io/badge/license-Apache_2.0-blue)

**T81 é um sistema operacional ternário para IA.**

Cada modelo carregado é executado dentro de um tempo de execução governado e determinístico. O kernel Axion intercepta todas as operações de IA antes que ocorra qualquer efeito colateral. O sistema de arquivos é endereçado ao conteúdo e imutável. O ISA substitui matmul de ponto flutuante por adição - nenhuma unidade de multiplicação é necessária. Qualquer IA expressável em pesos ternários é executada aqui: de forma verificável, reprodutível e sob controle político explícito.

```sh
curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
```

---

## Demonstração

Inicialize T81 no QEMU AArch64 (EDK2 slice6) em qualquer host Linux:

```sh
# Instalar dependências (Ubuntu 24.04)
sudo apt-get install -y qemu-system-arm qemu-efi-aarch64 mtools cmake ninja-build clang-18 lld-18

#Clone e execute
clone git https://github.com/t81dev/t81-foundation.git && cd t81-foundation
./drivers/qemu/scripts/boot_demo.sh
```

Saída terminal esperada:

```texto
Axion QEMU AArch64 EDK2 fatia6

[axion] entrada de kernel EL1 bare-metal
[axion] ExitBootServices concluído; transferindo para o kernel C++

T81 -- SO ternário para IA
  ===========================

Mecanismo de política [axion]: pronto
[axion] canonfs: montado (na memória)
[axion] thread do kernel tid = 1: em execução

t81> ajuda
  ajuda - esta mensagem
  versão -- T81 informações de compilação
  status – contadores de kernel e estado de governança
  política -- Axion resumo da política
t81>
```

Reproduza a sessão pré-gravada localmente com [asciinema](https://asciinema.org):

```sh
asciinema play drivers/qemu/t81-boot.cast
```

O log completo de inicialização trifásica está em [`drivers/qemu/sample-boot-log.txt`](drivers/qemu/sample-boot-log.txt). O fluxo de trabalho de CI [`qemu-boot`](.github/workflows/qemu-boot.yml) valida essa sequência em cada push.

---

## Índice

- [Demonstração](#demonstração)
- [O sistema operacional que faltava à IA](#o-sistema-operacional-que-faltava-à-ia)
- [Arquitetura](#arquitetura)
- [Qual é a aparência do T81Lang](#qual-é-a-aparência-do-t81lang)
- [Obtenha T81](#obtenha-t81)
- [Status](#status)
- [Progresso da inicialização](#progresso-da-inicialização)
- [Referência CLI](#referência-cli)
- [Verificação de determinismo](#verificação-de-determinismo)
- [Documentação](#documentação)
- [Governança](#governança)
- [A vantagem ternária](#a-vantagem-ternária)
- [Licença](#licença)

## O sistema operacional que faltava à IA

Os sistemas operacionais binários fornecem aos agentes de IA um slot de processo e um sistema de arquivos. É isso. Eles não podem dizer se uma inferência foi exata em termos de bits, qual política autorizou um carregamento de modelo ou se os pesos no disco são os pesos executados. T81 preenche essa lacuna – não colocando ferramentas em camadas sobre um sistema operacional existente, mas construindo o kernel, ISA, sistema de arquivos e modelo de processo que a computação nativa de IA exige.

### 1. Um kernel que governa todas as operações de IA antes dos efeitos colaterais

Quando um agente de IA realiza uma ação hoje, normalmente não existe nenhum mecanismo para verificar *após o fato* o que ele calculou, qual política aplicou ou se o resultado foi alterado. T81 corrige isso no nível de instrução.

O **Axion kernel** intercepta `AgentInvoke`, chamadas FFI e todos os códigos de operação de inferência no TISC ISA *antes que qualquer efeito colateral ocorra*. A política é escrita em Axion Policy Language (APL) e é fechada com falha – uma falha na análise da política interrompe a operação. Cada evento interceptado é gravado em uma trilha de auditoria ancorada em **CanonFS** que pode ser reproduzida deterministicamente.

```apl
# secure_model.apl — allow inference only for verified model hashes
allow infer if model.hash in approved_models;
deny  infer reason "unapproved-model";
```

```sh
t81 code run inference.t81 --policy secure_model.apl
# Axion: ALLOW  infer  model=sha3:a3f7c2b1…
# Axion: DENY   infer  model=sha3:deadbeef…  reason=unapproved-model
```

### 2. Reprodutibilidade como invariante do kernel, não como disciplina de ferramentas

O ponto flutuante IEEE 754 é inerentemente sensível à plataforma: os modos de arredondamento diferem, o tratamento denormal varia, a disponibilidade do FMA altera os resultados. As cargas de trabalho de IA baseadas nele não podem ser reproduzidas ou auditadas com certeza.

A aritmética ternária balanceada é simétrica em torno de zero. Arredondamento é truncamento – sem polarização direcional, sem desvio específico da plataforma. As superfícies determinísticas de T81 produzem **hashes de rastreamento CanonHash81 que são idênticos em bits** em todas as plataformas suportadas, verificados em cada execução de CI. Esta não é uma propriedade que possa ser aparafusada; é uma consequência do design do ISA.

```sh
t81 determinism verify-run program.tisc
#  Run 1: a3f7c2b1e94d8f20…
#  Run 2: a3f7c2b1e94d8f20…
#  ✓  bit-exact match confirmed
```

Plataformas verificadas: **Linux x86\_64**, **macOS ARM64**. Qualquer divergência numa superfície determinística governada é tratada como um defeito crítico.

### 3. Um ISA nativo para pesos ternários - nenhuma unidade de multiplicação necessária

Os pesos ternários {−1, 0, +1} não têm componente fracionário. Um produto escalar sobre eles é uma série de operações condicionais de adição/subtração - sem necessidade de multiplicação. T81 envia seis opcodes TISC que exploram isso diretamente:

| Código de operação | Operação |
| :--- | :--- |
| `TWMATMUL` | Matriz de peso ternário multiplicação |
| `TQUANT` | Quantize ativações para trit |
| `TATTN` | Atenção ternária (Q·Kᵀ sobre pesos trit) |
| `TWEMBED` | Pesquisa de incorporação de peso |
| `TERNACCUM` | Acumulação escalar de produto escalar trit |
| `TACT` | Ativação com portão de teto Axion |

Isso se alinha aos modelos da classe BitNet b1.58/xTern: **15–60× redução de energia**, **4–90× ganho de rendimento** versus linhas de base FP16/FP32 com precisão comparável. T81 O formato Ternary Weight (T81WTN) armazena modelos quantizados; `t81 weights import` converte de SafeTensors ou GGUF.

```sh
t81 weights import model.safetensors -o model.t81w
t81 code run inference.t81 --weights-model model.t81w --policy secure_model.apl
```

---

## Arquitetura

T81 é um sistema operacional. Cada componente tem um análogo no design de sistema operacional tradicional – criado do zero para semântica ternária e cargas de trabalho nativas de IA.

| Componente T81 | OS analogue | Papel |
| :--- | :--- | :--- |
| **TISC UM** | Conjunto de instruções (RISC-V, ARM) | Contrato de execução congelado; todo software é compilado nele |
| **T81VM** | Mecanismo de execução do kernel | Intérprete determinístico TISC; Axion dispara em cada opcode |
| **Axion** | Kernel de segurança | Política de falha fechada antes de qualquer efeito colateral; ancorado em auditoria |
| **CanonFS** | Sistema de arquivos | Endereçado ao conteúdo, imutável; pesos do modelo verificados por hash |
| **T81Lang** | Linguagem de programação do sistema | Compila para TISC; `agent`/`behavior` são o modelo de processo |
| **Agente / Comportamento** | Modelo de processo | Um agente é um processo; um comportamento é seu `main()` |
| **Níveis Cognitivos** | Hierarquia de anel de privilégio | Nível 1 (simbólico) → Nível 5 (distribuído); limitado pela governação |
| **DPE** | Agendador | Gráfico de tarefas determinísticas; atomicidade de confirmação de época |

```text
┌─────────────────────────────────────────────────────────────┐
│  Interfaces                                                 │
│  t81 studio (Human TUI)   t81 agent (AI-Native TUI)  CLI    │
├─────────────────────────────────────────────────────────────┤
│  T81Lang  — system language                                 │
│  Lexer → Parser → Typed AST → Semantic Analyzer → IRGen     │
│  agent/behavior (RFC-0015)  ·  foreign {} (RFC-0036)        │
├─────────────────────────────────────────────────────────────┤
│  Axion  — kernel                                            │
│  PolicyEngine · CanonFS · Audit Trail · Ethics Gate         │
├──────────────────────────────┬──────────────────────────────┤
│  T81VM  — execution engine   │  DPE  — scheduler            │
│  TISC interpreter            │  EpochGraph · DeltaBuffer    │
│  (deterministic)             │  (RFC-DPE-0002)              │
├──────────────────────────────┴──────────────────────────────┤
│  TISC ISA v1.9.0  ❄️ Frozen  +  Data Types  ❄️ Frozen       │
│  CanonHash81 bit-exact traces across all platforms          │
├─────────────────────────────────────────────────────────────┤
│  Governed FFI (RFC-00B8)  ·  Ternary-Native Inference       │
│  TWMATMUL · TQUANT · TATTN · TWEMBED · TERNACCUM · TACT     │
└─────────────────────────────────────────────────────────────┘
  Experimental: T81 Userland · Cognitive Tiers
```

**TISC ISA** — O conjunto de instruções congeladas. Cada software é compilado nele. A semântica do Opcode e os formatos de ligação são imutáveis ​​na v1.x; divergência é um defeito crítico.

**T81VM** — O mecanismo de execução. Axion intercepta o disparo no limite de despacho do opcode - antes de qualquer efeito colateral - mantendo o caminho de governança fora do loop de intérprete ativo.

**Axion** — O kernel. Intercepta `AgentInvoke`, `AXREAD`, `AXSET`, `AXVERIFY`, opcodes de inferência e chamadas FFI antes de qualquer efeito colateral. Fail-closed em caso de falha na análise de política; cada evento comprometido com CanonFS. Um agente não possui capacidades por padrão – cada ação requer autorização de política explícita.

**CanonFS** — O sistema de arquivos. Pesos de modelo, objetos de código e artefatos de tempo de execução são armazenados como blobs imutáveis ​​identificados por hash. O kernel Axion verifica se os pesos carregados por um modelo correspondem ao hash na política governante, eliminando ataques de troca de modelo no nível do sistema operacional.

**T81Lang** — A linguagem de programação do sistema. Tipos nativos: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`, `Option`, `Result`. As declarações `agent`/`behavior` são o modelo do processo - um agente é um processo de primeira classe; um comportamento é seu ponto de entrada. Eles diminuem para `AgentInvoke` em TISC. `foreign {}` bloqueia abaixo de `FFICall` (RFC-00B8).

**DPE** — O agendador. As tarefas declaram entradas imutáveis; a VM confirma todas as gravações atomicamente no final da época. Paralelismo determinístico sobre o ISA congelado — não são necessários novos códigos de operação.

---

## Qual é a aparência do T81Lang

T81Lang é a linguagem de programação do sistema de T81. Ele compila para bytecode TISC e fornece às declarações `agent`/`behavior` status de primeira classe - um agente é um processo; um comportamento é seu ponto de entrada.

**Tipos básicos e aritmética:**

```t81
fn main() -> i32 {
  let greeting: T81String = "Hello, T81!";
  let ratio:    T81Float  = 3.14159t81;
  let big:      T81BigInt = 123456789t81;
  print(greeting);
  print(ratio);
  print(big);
  return 0;
}
```

**Agente/Comportamento — o modelo de processo:**

```t81
// Um agente é um processo nomeado. Seus comportamentos são seus pontos de entrada.
// A política do kernel Axion bloqueia cada AgentInvoke antes da execução.
agente Calculadora {
  comportamento add(a: i32, b: i32) -> i32 {
    retornar a + b;
  }
}

fn principal() -> i32 {
  deixe o resultado: i32 = Calculator.add(38, 4);
  imprimir(resultado);   //42
  retornar 0;
}
```

**Executando e compilando:**

```sh
t81 code run program.t81                          # compile and execute
t81 code build program.t81 -o program.tisc        # compile to bytecode
t81 vm run program.tisc                           # execute bytecode directly
```

**Com a política Axion e um modelo de pesos:**

```sh
t81 code run inference.t81 \
  --policy        secure_model.apl \
  --weights-model model.t81w \
  --trace
```

**Experimente em seu navegador — não é necessária instalação:**

> **[Inicie o Playground T81Lang →](https://t81dev.github.io/t81-foundation/playground)**
>
> Escreva e execute programas T81Lang diretamente no navegador. O compilador completo + interpretador T81VM é executado como WebAssembly. Oito exemplos integrados: Hello World, aritmética BigInt, tensores, agente/comportamento e muito mais.

**Exploração interativa (local):**

```sh
t81 repl       # line-buffered REPL; empty line executes
t81 studio     # human operator TUI (7 views, Ctrl+P palette)
t81 agent      # AI-native TUI with /compile /run /hash /allow /infer
```

---

## Obtenha T81

###macOS/Linux

```sh
curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
```

Detecta a arquitetura do sistema operacional e da CPU, baixa o binário correto e instala em `~/.local/bin`. Defina `T81_INSTALL_DIR` para substituir.

### Windows (PowerShell)

```powershell
irm https://github.com/t81dev/t81-foundation/releases/latest/download/install.ps1 | iex
```

Instala em `%LOCALAPPDATA%\t81\bin`.

### Docker — 60 segundos, zero conjunto de ferramentas

```sh
docker run --rm -it ghcr.io/t81dev/t81-foundation demo
```

Extrai uma imagem de aproximadamente 100 MB, executa três programas (Hello World → tipos ternários → verificação de determinismo) e, em seguida, entra em um REPL interativo. Sem compilador, sem CMake, sem configuração.

```sh
docker run --rm -it ghcr.io/t81dev/t81-foundation          # REPL only
docker run --rm -it ghcr.io/t81dev/t81-foundation <cmd>    # any t81 subcommand
```

### Arquivos pré-construídos

Downloads diretos da [versão mais recente](https://github.com/t81dev/t81-foundation/releases/latest):

| Plataforma | Arquivo |
| :--- | :--- |
| Linux x86\_64 | `t81-<version>-linux-x86_64.tar.gz` |
| Linux ARM64 | `t81-<version>-linux-arm64.tar.gz` |
| macOS Apple Silício | `t81-<version>-macos-arm64.tar.gz` |
| MacOS Intel | `t81-<version>-macos-x86_64.tar.gz` |
| Janelas x86\_64 | `t81-<version>-windows-x86_64.zip` |

Cada arquivo usa um layout de instalação padrão: `bin/`, `lib/`, `include/`. Coloque `bin/t81` em seu `PATH`.

###Python (pip)

```sh
pip install t81
```

Instala o pacote `t81` Python para CPython 3.9–3.13 no Linux (x86\_64, ARM64), macOS (Apple Silicon, Intel) e Windows. Fornece `T81Int`, `BigInt`, `Float`, `Fraction`, `Tensor`, `HanoiVM`, `CanonFS` e a API `compile`/`compile_and_run` completa. As rodas são publicadas no PyPI em cada versão por meio do fluxo de trabalho [`python-wheels`](.github/workflows/python-wheels.yml).

```python
import t81
result = t81.compile_and_run("fn main() -> i32 { return 42; }")
```

### Construir a partir da fonte

```sh
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake --preset default -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure   # 404 tests
```

---

## Status

v1.9.2 · Testes 404/404 aprovados · Apache 2.0

Os tipos de dados TISC ISA e principais estão **congelados** na v1.x — a semântica do opcode e os formatos de ligação não serão alterados sem um grande aumento na versão.

| Componente | Maturidade | Notas |
| :--- | :--- | :--- |
| **TISC UM** | ❄️ Congelado | v1.9.0; `AgentInvoke`, 6 opcodes de inferência nativos ternários, 3 FFI, 2 criptografias de rede, 1 NTRU-KEM |
| **Tipos de dados** | ❄️ Congelado | BigInt, Float, Complex, Map, Set — codificação estável em bits |
| **T81VM** | ✅ Estável | Superfície determinística verificada; rastreamentos de bits idênticos no Linux x86\_64 + macOS ARM64 |
| **T81Lang** | ✅ Estável | Especificação v1.9.0; o determinismo do compilador controla ativo |
| **Axion** | ✅ Estável | Sequências de motivos canônicos, ganchos de auditoria, aplicação de políticas de falha fechada |
| **Inferência Ternário-Nativa** | ✅ Estável | RFC-0034+RFC-0037; todos os 6 opcodes implementados e evidenciados |
| **Criptografia de rede** | ✅ Estável | RFC-0038 (rede ternária) + RFC-0039 (NTRU-KEM) |
| **FFI governada** | ✅ Estável | RFC-00B8+RFC-0036; Sintaxe `FFIDispatcher`, `FFILibraryRegistry`, `foreign {}` |
| **DPE (Execução Paralela)** | ✅ Estável | RFC-DPE-0001–0009; semântica de época determinística |
| **Front-ends da TUI** | ✅Beta | `t81 studio` e `t81 agent` – utilizáveis ​​em produção |
| **Níveis Cognitivos** | ✅Beta | Cognição Tier4 (RFC-0021); limitado pela governação |
| **T81 País do usuário** | ✅Beta | Serviços de usuário HAL +; limitado por políticas |
| **Destino bare-metal nativo** | 🚧 Alfa | T81 atualmente é executado como uma camada de sistema operacional convidado no Linux e macOS; a execução bare-metal está em desenvolvimento ativo |
| **Sequência de inicialização do QEMU** | 🚧 Alfa | EFI → bare-metal → ponte C++ independente confirmada; `t81>` shell ativo em serial — [veja o progresso da inicialização](#inspeção-de-vm) |

As classificações de superfície seguem RFC-0048. Superfícies experimentais e não-DCP governadas não são apresentadas como componentes determinísticos verificados.

---

## Progresso da inicialização

Gravação ao vivo da sequência de inicialização atual do QEMU AArch64 (saída serial):

<p align="center">
  <img src="https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/assets/boot.gif"
       alt="T81 QEMU AArch64 boot sequence — live t81> demonstração de shell"
       width="95%" style="border:1px solid #ddd; raio da borda:8px; sombra da caixa: 0 4px 8px rgba(0,0,0,0.1);">
  <br><small>Progresso atual da inicialização: EFI → bare-metal EL1 → mecanismo de política → CanonFS montagem → t81 interativo> prompt</small>
</p>

<br><small>Replay interativo: <a href="https://github.com/t81dev/t81-foundation/blob/main/drivers/qemu/t81-boot.cast">t81-boot.cast (asciinema)</a></small>

T81 inicializa no QEMU AArch64 (EDK2/UEFI). A tabela abaixo rastreia a conclusão de uma inicialização limpa com um prompt de shell visível na saída serial — o pré-requisito para uma demonstração de inicialização gravada neste README.

| Estágio | O que cobre | Feito |
| :--- | :--- | :--- |
| **1. Inicialização EFI/UEFI** | Cargas binárias PE32 + EFI, conclusão de `ExitBootServices`, transferência para kernel bare-metal | 95% |
| **2. Entrada do kernel + inicialização HAL** | PL011 UART confirmado em EL1; ponte C++ independente inicializa antes do shell | 95% |
| **3. EFI ↔ ponte de kernel C++** | C++ independente (`-ffreestanding -fno-exceptions`) compilado em BOOTAA64.EFI; chama banner + shell do QEMU real | 90% |
| **4. CanonFS montagem** | Driver na memória sempre online na inicialização; driver persistente é ativado via `T81_CANONFS_ROOT` | 80% |
| **5. Shell/prompt interativo** | Shell `t81>` com buffer de linha em serial; Comandos `help` / `version` / `status` / `policy` | 95% |
| **6. Loop de eventos do kernel ** | Despacho prioritário (falhas → interrupções → pager → tick do agendador), WFI inativo | 100% |
| | **Geral** | **~93%** |

**Estado atual:** O binário BOOTAA64.EFI é uma imagem de três estágios. A Fase 1 (EFI) imprime o banner ConOut e chama `ExitBootServices`. A Fase 2 (bare metal C) confirma o acesso EL1 PL011 MMIO. A Fase 3 (ponte C++ independente) imprime o banner de governança e executa o shell interativo `t81>` — tudo compilado em um único binário PE32+ sem tempo de execução C++ hospedado. A sequência serial esperada em uma execução do Linux QEMU:

```texto
Axion QEMU AArch64 EDK2 fatia6

[axion] entrada de kernel EL1 bare-metal
[axion] ExitBootServices concluído; transferindo para o kernel C++

T81 -- SO ternário para IA
  ===========================

Mecanismo de política [axion]: pronto
[axion] canonfs: montado (na memória)
[axion] thread do kernel tid = 1: em execução

t81>
```

**Restante para inicialização limpa:** Driver Virtio-blk MMIO para CanonFS persistente em bare-metal (então `T81_CANONFS_ROOT` tem um dispositivo de bloco real por trás dele no QEMU) e conectando o loop de eventos `KernelRuntimeState` hospedado (agendador, pager, interrupções GICv3) no caminho da ponte independente para que `status` mostre contadores ativos.

Scripts de inicialização, imagem de disco e saída serial capturada estão em [`drivers/qemu/`](drivers/qemu/):

- [`drivers/qemu/scripts/launch_production.sh`](drivers/qemu/scripts/launch_production.sh) — inicializa a imagem no QEMU
- [`drivers/qemu/sample-boot-log.txt`](drivers/qemu/sample-boot-log.txt) — sequência serial confirmada de uma execução recente
- [`drivers/qemu/docs/QEMU_TESTING_RESULTS.md`](drivers/qemu/docs/QEMU_TESTING_RESULTS.md) — relatório completo do teste de inicialização

O fluxo de trabalho de CI [`qemu-boot`](.github/workflows/qemu-boot.yml) cria o binário EFI, monta uma imagem FAT32 GPT, inicializa-a no QEMU (TCG cortex-a57 + EDK2 AArch64) em cada push que toca `userland/experimental/` ou `drivers/qemu/`, valida todos os oito marcadores de inicialização em todas as três fases e confirma o log serial atualizado de volta para `drivers/qemu/sample-boot-log.txt`.

---

## Referência CLI

```sh
# Compilar e executar
compilação de código t81 <file.t81> -o <file.tisc>
código t81 executado <file.t81|file.tisc> [--policy <apl>] [--weights-model <t81w>] [--trace]
substituição de código t81
verificação de código t81 <file.t81>

# Inspeção de VM
t81 vm executado <file.tisc>
depuração t81 vm <file.tisc>
rastreamento de vm t81 <file.tisc>

# Axion governança
compilação de política t81 <file.apl>
Política t81 validada <file.apl>
status do áxion t81
auditoria axion t81

# Determinismo
t81 determinismo verify-run <file.tisc> # execute duas vezes, compare hashes
hash de determinismo t81 <file.tisc>
Certificação de determinismo t81 <file.tisc>

# Pesos do modelo
importação de pesos t81 <model.safetensors|model.gguf> -o model.t81w
informações de pesos t81 <model.t81w>
pesos t81 verificam <model.t81w>
pesos t81 quantizam <input> --to-gguf <out>

# TISC bytecódigo
t81 desasmo tisc <file.tisc>
validação de tisc t81 <file.tisc>
estatísticas de tisc t81 <file.tisc>

#Interfaces
t81 studio # operador humano TUI
agente t81 # TUI nativa de IA
```

---

## Verificação de determinismo

```sh
./scripts/ci/run_determinism_slice.sh
```

O portão de determinismo multiplataforma de CI é executado em cada push para `main` e em uma programação diária. Qualquer divergência de hash em uma superfície determinística verificada bloqueia a mesclagem.

---

## Documentação

| Tópico | Localização |
| :--- | :--- |
| Primeiros passos (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| Primeiros passos (IA) | `docs/user-guide/getting-started/ai-quickstart.md` |
| Guias TUI | `docs/user-guide/how-to/tui-guide.md` |
| Especificação ISA | `spec/tisc-spec.md` |
| Axion Manual de Políticas | `docs/user-guide/tutorials/axion-policy-manual.md` |
| Referência Stdlib T81Lang | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| Visão geral da arquitetura | `docs/architecture/OVERVIEW.md` |
| Carta de Governança | `docs/governance/README.md` |
| Centro de Controle de Projetos | `docs/status/PROJECT_CONTROL_CENTER.md` |
| Resultados de referência de inferência | [`benchmarks/results/inference_comparison.md`](benchmarks/results/inference_comparison.md) |

---

## Governança

T81 Foundation opera sob um modelo de **Governança Contínua (C2)**. Todas as contribuições devem manter:

- **paridade de execução determinística** — hashes de rastreamento correspondem entre plataformas suportadas
- **coerência arquitetônica** — alterações na superfície determinística requerem revisão formal
- **autoridade de especificação** — `spec/` > `docs/architecture/` > `docs/`; superfícies congeladas requerem um impacto de versão principal

O registro de superfície determinístico é definido em `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. As classificações de limites de superfície (DCP/não-DCP governado/experimental/fora do escopo) são definidas na RFC-0048.

---

## A vantagem ternária

Embora o hardware binário moderno seja altamente otimizado para computação de uso geral, o **ternário balanceado** ({−1, 0, +1}) tem propriedades estruturais que importam especificamente na execução determinística, inferência de IA governada e cargas de trabalho neurais de baixa complexidade.

### 1. Negação O(1) — propagação de transporte zero

A negação binária do complemento de dois é um NOT bit a bit seguido por +1, o que pode acionar longas cadeias de transporte. A negação ternária balanceada inverte +1 ↔ −1 e deixa 0 inalterado - **sem transporte, tempo constante**.

Medido: a negação de PackedCell atinge **~49,9 G-ops/s** em hardware x86\_64 recente, **~10,9× mais rápido** do que a negação de número inteiro de 64 bits otimizada (verificada em Linux x86\_64 e macOS ARM64).

### 2. Economia de raiz superior

A base ótima da teoria da informação é *e ≈ 2,718*. O ternário (base 3) está mais próximo do que o binário (base 2), fornecendo **~1,585 bits de informação por trit** (log₂3). Maior entropia por dígito, intervalos simétricos mais compactos — especialmente úteis para pesos, embeddings e tensores esparsos.

### 3. Determinismo bit-exato inerente

O IEEE 754 sofre de modos de arredondamento específicos da plataforma, diferenças de associatividade e manuseio denormal. O ternário balanceado é simétrico em torno de zero: o arredondamento é um truncamento sem polarização direcional. Cada caminho de execução produz **hashes de rastreamento CanonHash81 idênticos** em plataformas suportadas.

### 4. Inferência neural livre de multiplicação

Os pesos ternários {−1, 0, +1} reduzem os produtos escalares para adição/subtração condicional - nenhuma unidade de multiplicação necessária. Combinado com os seis opcodes de inferência TISC:

- Redução de energia de 15–60× em relação às linhas de base FP16/FP32
- Ganho de rendimento de 4–90× com precisão comparável
- Alinha-se com BitNet b1.58, xTern e pesquisa de transformadores ternários 2024–2026

O formato T81 Peso Ternário (T81WTN) e `t81 weights import` tornam-no pronto para produção na pilha hoje.

### 5. Ganchos de governança em nível Trit

Como o TISC ISA é nativo ternário, o kernel Axion pode interceptar e auditar transições de estado em **granularidade de nível trit** antes de qualquer efeito colateral. Isso permite a aplicação de políticas sem falhas, portas éticas refinadas e trilhas de auditoria determinísticas que são fundamentalmente mais inspecionáveis ​​do que a execução binária de caixa preta.

---

## Licença

Licença Apache 2.0.

---

<details>
<summary>Nota honesta de bootstrap (março de 2026)</summary>

T81 foi projetado como um sistema operacional independente com seu próprio ISA e kernel - mas ainda não existe hardware ternário nativo. A visualização atual é executada como uma camada convidada no Linux/macOS/Windows por meio de binários, Docker ou QEMU.

Este é um andaime temporário - da mesma forma que o Linux inicial rodava em simuladores antes do hardware real. A bota bare metal está em Alpha; o objetivo é eventualmente escapar completamente da dependência do sistema operacional host.

Obrigado por ler até aqui.

</details>
