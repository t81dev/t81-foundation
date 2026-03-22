<p align="center">
  <img src="docs/assets/banner.png" alt="T81 — A Ternary Operating System for AI" width="100%">
  Vista previa de arranque en QEMU · Inferencia ternaria gobernada · Bit exacto en todas las plataformas
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation

![Lanzamiento](https://img.shields.io/badge/release-v1.9.2--Stable-blue)
![Pruebas](https://img.shields.io/badge/tests-404%2F404_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue)
![Ejecución](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![Licencia](https://img.shields.io/badge/license-Apache_2.0-blue)

**T81 es un sistema operativo ternario para IA.**

Cada modelo que carga se ejecuta dentro de un tiempo de ejecución determinista y gobernado. El kernel Axion intercepta cada operación de IA antes de que ocurra cualquier efecto secundario. El sistema de archivos está dirigido al contenido y es inmutable. La ISA reemplaza el matmul de punto flotante con la suma, sin necesidad de multiplicar unidades. Cualquier IA expresable en pesos ternarios se ejecuta aquí: verificable, reproducible y bajo control político explícito.

```sh
curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
```

---

## Demostración

Inicie T81 en QEMU AArch64 (EDK2 segmento6) en cualquier host Linux:

```sh
# Instalar departamentos (Ubuntu 24.04)
sudo apt-get install -y qemu-system-arm qemu-efi-aarch64 mtools cmake ninja-build clang-18 lld-18

# Clonar y ejecutar
clon de git https://github.com/t81dev/t81-foundation.git && cd t81-foundation
./drivers/qemu/scripts/boot_demo.sh
```

Salida terminal esperada:

```texto
Axion QEMU AArch64 EDK2 segmento6

[axion] entrada del kernel EL1 bare-metal
[axion] ExitBootServices completo; entrega al kernel de C++

T81 -- SO ternario para IA
  ============================

[axion] motor de políticas: listo
[axion] canonfs: montado (en memoria)
[axion] hilo del kernel tid=1: ejecutándose

t81> ayuda
  ayuda - este mensaje
  versión -- T81 información de compilación
  estado: contadores de núcleo y estado de gobernanza
  política -- Axion resumen de política
t81>
```

Reproduzca la sesión pregrabada localmente con [asciinema](https://asciinema.org):

```sh
asciinema play drivers/qemu/t81-boot.cast
```

El registro de arranque trifásico completo se encuentra en [`drivers/qemu/sample-boot-log.txt`](drivers/qemu/sample-boot-log.txt). El flujo de trabajo de CI [`qemu-boot`](.github/workflows/qemu-boot.yml) valida esta secuencia en cada inserción.

---

## Tabla de Contenidos

- [Demostración](#demostración)
- [El sistema operativo que le faltaba a la IA](#el-sistema-operativo-que-le-faltaba-a-la-ia)
- [Arquitectura](#arquitectura)
- [Cómo se ve T81Lang](#cómo-se-ve-t81lang)
- [Obtener T81](#obtener-t81)
- [Estado](#estado)
- [Progreso de arranque](#progreso-de-arranque)
- [Referencia CLI](#referencia-cli)
- [Verificación del determinismo](#verificación-del-determinismo)
- [Documentación](#documentación)
- [Gobernanza](#gobernanza)
- [La ventaja ternaria](#la-ventaja-ternaria)
- [Licencia](#licencia)

## El sistema operativo que le faltaba a la IA

Los sistemas operativos binarios brindan a los agentes de IA una ranura de proceso y un sistema de archivos. Eso es todo. No pueden decirle si una inferencia fue exacta en bits, qué política autorizó la carga de un modelo o si los pesos en el disco son los pesos que se ejecutaron. T81 cierra esa brecha, no colocando herramientas sobre un sistema operativo existente, sino construyendo el kernel, ISA, sistema de archivos y modelo de proceso que requiere la computación nativa de IA.

### 1. Un kernel que gobierna cada operación de IA antes de los efectos secundarios

Cuando un agente de IA realiza una acción hoy en día, normalmente no existe ningún mecanismo para verificar *después del hecho* qué calculó, qué política aplicó o si el resultado fue alterado. T81 soluciona este problema en el nivel de instrucción.

El kernel **Axion** intercepta `AgentInvoke`, llamadas FFI y cada código de operación de inferencia en TISC ISA *antes de que ocurra cualquier efecto secundario*. La política está escrita en el lenguaje de políticas (APL) Axion y se cierra a prueba de fallos: un error en el análisis de la política detiene la operación. Cada evento interceptado se escribe en un registro de auditoría anclado **CanonFS** que se puede reproducir de forma determinista.

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

### 2. La reproducibilidad como invariante del núcleo, no como disciplina de herramientas

El punto flotante IEEE 754 es inherentemente sensible a la plataforma: los modos de redondeo difieren, el manejo anormal varía, la disponibilidad de FMA cambia los resultados. Las cargas de trabajo de IA basadas en él no se pueden reproducir ni auditar con certeza.

La aritmética ternaria equilibrada es simétrica alrededor del cero. El redondeo es truncamiento: sin sesgo direccional, sin deriva específica de la plataforma. Las superficies deterministas de T81 producen **hashes de seguimiento CanonHash81 que son idénticos en bits** en todas las plataformas compatibles, verificados en cada ejecución de CI. Esta no es una propiedad que pueda complementarse; es una consecuencia del diseño de ISA.

```sh
t81 determinism verify-run program.tisc
#  Run 1: a3f7c2b1e94d8f20…
#  Run 2: a3f7c2b1e94d8f20…
#  ✓  bit-exact match confirmed
```

Plataformas verificadas: **Linux x86\_64**, **macOS ARM64**. Cualquier divergencia en una superficie determinista gobernada se trata como un defecto crítico.

### 3. Un ISA nativo de pesos ternarios: no se requiere unidad multiplicadora

Los pesos ternarios {−1, 0, +1} no tienen componente fraccionario. Un producto escalar sobre ellos es una serie de operaciones condicionales de suma/resta, sin necesidad de multiplicar. T81 incluye seis códigos de operación TISC que explotan esto directamente:

| Código de operación | Operación |
| :--- | :--- |
| `TWMATMUL` | Matriz de peso ternaria multiplicada |
| `TQUANT` | Cuantizar activaciones para trit |
| `TATTN` | Atención ternaria (Q·Kᵀ sobre pesos trit) |
| `TWEMBED` | Búsqueda de incrustación de peso |
| `TERNACCUM` | Acumulación de producto escalar trit escalar |
| `TACT` | Activación con puerta de techo Axion |

Esto se alinea con los modelos de clase BitNet b1.58/xTern: **15–60× reducción de energía**, **4–90× ganancia de rendimiento** frente a las líneas base FP16/FP32 con una precisión comparable. El formato T81 Peso ternario (T81WTN) almacena modelos cuantificados; `t81 weights import` convierte desde SafeTensors o GGUF.

```sh
t81 weights import model.safetensors -o model.t81w
t81 code run inference.t81 --weights-model model.t81w --policy secure_model.apl
```

---

## Arquitectura

T81 es un sistema operativo. Cada componente tiene un análogo en el diseño del sistema operativo tradicional, creado desde cero para semántica ternaria y cargas de trabajo nativas de IA.

| T81 componente | Análogo del sistema operativo | Role |
| :--- | :--- | :--- |
| **TISC UNO** | Conjunto de instrucciones (RISC-V, ARM) | Contrato de ejecución congelado; todo el software se compila en él |
| **T81VM** | Motor de ejecución del kernel | Intérprete determinista TISC; Axion se activa en cada código de operación |
| **Axion** | Núcleo de seguridad | Política cerrada ante cada efecto secundario; anclado en auditoría |
| **CanonFS** | Sistema de archivos | Dirigido al contenido, inmutable; pesos del modelo verificados por hash |
| **T81Idioma** | lenguaje de programación del sistema | Compila en TISC; `agent`/`behavior` son el modelo de proceso |
| **Agente / Comportamiento** | modelo de proceso | Un agente es un proceso; un comportamiento es su `main()` |
| **Niveles cognitivos** | Jerarquía de anillos de privilegios | Nivel 1 (simbólico) → Nivel 5 (distribuido); limitada por la gobernanza |
| **DPE** | Programador | Gráfico de tareas determinista; atomicidad de compromiso de época |

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

**TISC ISA** — El conjunto de instrucciones congelado. Cada pieza de software se compila con él. La semántica del código de operación y los formatos de cable son inmutables en v1.x; la divergencia es un defecto crítico.

**T81VM** — El motor de ejecución. Axion intercepta el fuego en el límite de envío del código de operación, antes de cualquier efecto secundario, manteniendo la ruta de gobernanza fuera del bucle del intérprete activo.

**Axion** — El núcleo. Intercepta `AgentInvoke`, `AXREAD`, `AXSET`, `AXVERIFY`, códigos de operación de inferencia y llamadas FFI antes de cualquier efecto secundario. Fallo cerrado debido a un error en el análisis de políticas; cada evento comprometido con CanonFS. Un agente no posee capacidades de forma predeterminada: cada acción requiere una autorización de política explícita.

**CanonFS** — El sistema de archivos. Los pesos de los modelos, los objetos de código y los artefactos en tiempo de ejecución se almacenan como blobs inmutables identificados mediante hash. El kernel Axion verifica que los pesos que carga un modelo coincidan con el hash en la política gobernante, eliminando los ataques de intercambio de modelos a nivel del sistema operativo.

**T81Lang** — El lenguaje de programación del sistema. Tipos nativos: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`, `Option`, `Result`. Las declaraciones `agent`/`behavior` son el modelo de proceso: un agente es un proceso de primera clase; un comportamiento es su punto de entrada. Bajan a `AgentInvoke` en TISC. `foreign {}` bloques inferiores a `FFICall` (RFC-00B8).

**DPE** — El planificador. Las tareas declaran entradas inmutables; la VM confirma todas las escrituras de forma atómica al final de la época. Paralelismo determinista sobre la ISA congelada: no se requieren nuevos códigos de operación.

---

## Cómo se ve T81Lang

T81Lang es el lenguaje de programación del sistema de T81. Se compila en TISC código de bytes y otorga a las declaraciones `agent`/`behavior` un estado de primera clase: un agente es un proceso; un comportamiento es su punto de entrada.

**Tipos básicos y aritmética:**

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

**Agente/Comportamiento: el modelo de proceso:**

```t81
// Un agente es un proceso con nombre. Sus comportamientos son sus puntos de entrada.
// La política del kernel Axion controla cada AgentInvoke antes de su ejecución.
Calculadora de agente {
  comportamiento agregar (a: i32, b: i32) -> i32 {
    devolver a + b;
  }
}

fn principal() -> i32 {
  dejar resultado: i32 = Calculadora.add(38, 4);
  imprimir(resultado);   // 42
  devolver 0;
}
```

**Ejecutando y compilando:**

```sh
t81 code run program.t81                          # compile and execute
t81 code build program.t81 -o program.tisc        # compile to bytecode
t81 vm run program.tisc                           # execute bytecode directly
```

**Con política Axion y un modelo de ponderaciones:**

```sh
t81 code run inference.t81 \
  --policy        secure_model.apl \
  --weights-model model.t81w \
  --trace
```

**Pruébalo en tu navegador; no requiere instalación:**

> **[Inicie T81Lang Playground →](https://t81dev.github.io/t81-foundation/playground)**
>
> Escriba y ejecute programas T81Lang directamente en el navegador. El compilador completo + intérprete T81VM se ejecuta como WebAssembly. Ocho ejemplos integrados: Hello World, aritmética BigInt, tensores, agente/comportamiento y más.

**Exploración interactiva (local):**

```sh
t81 repl       # line-buffered REPL; empty line executes
t81 studio     # human operator TUI (7 views, Ctrl+P palette)
t81 agent      # AI-native TUI with /compile /run /hash /allow /infer
```

---

## Obtener T81

### MacOS/Linux

```sh
curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
```

Detecta la arquitectura del sistema operativo y de la CPU, descarga el binario correcto y lo instala en `~/.local/bin`. Establezca `T81_INSTALL_DIR` para anular.

### Windows (PowerShell)

```powershell
irm https://github.com/t81dev/t81-foundation/releases/latest/download/install.ps1 | iex
```

Se instala en `%LOCALAPPDATA%\t81\bin`.

### Docker: 60 segundos, cero cadena de herramientas

```sh
docker run --rm -it ghcr.io/t81dev/t81-foundation demo
```

Extrae una imagen de ~100 MB, ejecuta tres programas (Hello World → tipos ternarios → verificación de determinismo) y luego ingresa en un REPL interactivo. Sin compilador, sin CMake, sin configuración.

```sh
docker run --rm -it ghcr.io/t81dev/t81-foundation          # REPL only
docker run --rm -it ghcr.io/t81dev/t81-foundation <cmd>    # any t81 subcommand
```

### Archivos prediseñados

Descargas directas desde la [última versión](https://github.com/t81dev/t81-foundation/releases/latest):

| Plataforma | Archivo |
| :--- | :--- |
| Linuxx86\_64 | `t81-<version>-linux-x86_64.tar.gz` |
| ARM64 | `t81-<version>-linux-arm64.tar.gz` |
| macOS Apple Silicio | `t81-<version>-macos-arm64.tar.gz` |
| MacOS Intel | `t81-<version>-macos-x86_64.tar.gz` |
| Windows x86\_64 | `t81-<version>-windows-x86_64.zip` |

Cada archivo utiliza un diseño de instalación estándar: `bin/`, `lib/`, `include/`. Coloque `bin/t81` en su `PATH`.

### Pitón (pip)

```sh
pip install t81
```

Instala el paquete Python `t81` para CPython 3.9–3.13 en Linux (x86\_64, ARM64), macOS (Apple Silicon, Intel) y Windows. Proporciona `T81Int`, `BigInt`, `Float`, `Fraction`, `Tensor`, `HanoiVM`, `CanonFS` y la API `compile`/`compile_and_run` completa. Las ruedas se publican en PyPI en cada versión a través del flujo de trabajo [`python-wheels`](.github/workflows/python-wheels.yml).

```python
import t81
result = t81.compile_and_run("fn main() -> i32 { return 42; }")
```

### Construir desde la fuente

```sh
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake --preset default -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure   # 404 tests
```

---

## Estado

v1.9.2 · Pruebas 404/404 aprobadas · Apache 2.0

Los tipos de datos centrales y ISA TISC están **congelados** en v1.x: la semántica del código de operación y los formatos de cable no cambiarán sin un aumento importante de la versión.

| Componente | Madurez | Notas |
| :--- | :--- | :--- |
| **TISC UNO** | ❄️ Congelado | v1.9.0; `AgentInvoke`, 6 códigos de operación de inferencia nativos ternarios, 3 FFI, 2 criptografía de celosía, 1 NTRU-KEM |
| **Tipos de datos** | ❄️ Congelado | BigInt, Float, Complex, Map, Set: codificación de bits estable |
| **T81VM** | ✅ Estable | Superficie determinista verificada; rastros de bits idénticos en Linux x86\_64 + macOS ARM64 |
| **T81Idioma** | ✅ Estable | Especificación v1.9.0; controles de determinismo del compilador activos |
| **Axion** | ✅ Estable | Cadenas de motivos canónicos, ganchos de auditoría, aplicación de políticas cerradas ante fallos |
| **Inferencia ternaria-nativa** | ✅ Estable | RFC-0034 + RFC-0037; los 6 códigos de operación implementados y evidenciados |
| **Criptografía reticular** | ✅ Estable | RFC-0038 (red ternaria) + RFC-0039 (NTRU-KEM) |
| **IFF gobernada** | ✅ Estable | RFC-00B8 + RFC-0036; Sintaxis `FFIDispatcher`, `FFILibraryRegistry`, `foreign {}` |
| **DPE (ejecución paralela)** | ✅ Estable | RFC-DPE-0001–0009; semántica de época determinista |
| **Interfaces TUI** | ✅Beta | `t81 studio` y `t81 agent`: utilizables en producción |
| **Niveles cognitivos** | ✅Beta | Cognición de nivel 4 (RFC-0021); limitada por la gobernanza |
| **T81 País de usuario** | ✅Beta | HAL + servicios de usuario; limitado por políticas |
| **Objetivo nativo bare-metal** | 🚧 Alfa | T81 actualmente se ejecuta como una capa de sistema operativo invitado en Linux y macOS; La ejecución bare-metal está en desarrollo activo. |
| **Secuencia de arranque QEMU** | 🚧 Alfa | EFI → bare-metal → puente C++ independiente confirmado; `t81>` shell activo en serie: [ver progreso de arranque](#progreso-de-arranque) |

Las clasificaciones de superficies siguen el RFC-0048. Las superficies experimentales y no DCP gobernadas no se presentan como componentes deterministas verificados.

---

## Progreso de arranque

Grabación en vivo de la secuencia de arranque actual de QEMU AArch64 (salida en serie):

<p align="center">
  <img src="https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/assets/boot.gif"
       alt="T81 QEMU AArch64 boot sequence — live t81> demostración de shell"
       ancho="95%" estilo="borde: 1px sólido #ddd; radio de borde: 8px; sombra de cuadro: 0 4px 8px rgba(0,0,0,0.1);">
  <br><small>Progreso de arranque actual: EFI → EL1 básico → motor de políticas → CanonFS montaje → t81 interactivo> indicador</small>
</p>

<br><small>Repetición interactiva: <a href="https://github.com/t81dev/t81-foundation/blob/main/drivers/qemu/t81-boot.cast">t81-boot.cast (asciinema)</a></small>

T81 arranca en QEMU AArch64 (EDK2/UEFI). La siguiente tabla realiza un seguimiento de la finalización hacia un inicio limpio con un indicador de shell visible en la salida en serie: el requisito previo para una demostración de inicio grabada en este README.

| Escenario | lo que cubre | Hecho |
| :--- | :--- | :--- |
| **1. Arranque EFI/UEFI** | Cargas binarias PE32+ EFI, `ExitBootServices` completado, transferencia al kernel bare-metal | 95% |
| **2. Entrada del kernel + inicio HAL** | PL011 UART confirmado en EL1; El puente C++ independiente se inicializa antes que el shell | 95% |
| **3. Puente del núcleo EFI ↔ C++** | C++ independiente (`-ffreestanding -fno-exceptions`) compilado en BOOTAA64.EFI; llama a banner + shell desde QEMU real | 90% |
| **4. CanonFS montaje** | Controlador en memoria siempre en línea al arrancar; El controlador persistente se activa a través de `T81_CANONFS_ROOT` | 80% |
| **5. Shell/mensaje interactivo** | Shell `t81>` con buffer de línea en serie; Comandos `help` / `version` / `status` / `policy` | 95% |
| **6. Bucle de eventos del kernel** | Despacho prioritario (fallos → interrupciones → buscapersonas → tic del programador), WFI inactivo | 100% |
| | **En general** | **~93%** |

**Estado actual:** El binario BOOTAA64.EFI es una imagen de tres etapas. La fase 1 (EFI) imprime el banner ConOut y llama a `ExitBootServices`. La fase 2 (metal desnudo C) confirma el acceso a EL1 PL011 MMIO. La fase 3 (puente C++ independiente) imprime el banner de gobernanza y ejecuta el shell interactivo `t81>`, todo compilado en un único binario PE32+ sin tiempo de ejecución alojado de C++. La secuencia serial esperada en una ejecución QEMU de Linux:

```texto
Axion QEMU AArch64 EDK2 segmento6

[axion] entrada del kernel EL1 bare-metal
[axion] ExitBootServices completo; entrega al kernel de C++

T81 -- SO ternario para IA
  ============================

[axion] motor de políticas: listo
[axion] canonfs: montado (en memoria)
[axion] hilo del kernel tid=1: ejecutándose

t81>
```

**Queda para limpiar el arranque:** Controlador MMIO Virtio-blk para CanonFS persistente en bare-metal (por lo que `T81_CANONFS_ROOT` tiene un dispositivo de bloque real detrás en QEMU) y cableado del bucle de eventos alojado `KernelRuntimeState` (programador, buscapersonas, interrupciones GICv3) en la ruta del puente independiente para que `status` muestre contadores en vivo.

Los scripts de arranque, la imagen del disco y la salida en serie capturada se encuentran en [`drivers/qemu/`](drivers/qemu/):

- [`drivers/qemu/scripts/launch_production.sh`](drivers/qemu/scripts/launch_production.sh) — inicia la imagen en QEMU
- [`drivers/qemu/sample-boot-log.txt`](drivers/qemu/sample-boot-log.txt) — secuencia en serie confirmada de una ejecución reciente
- [`drivers/qemu/docs/QEMU_TESTING_RESULTS.md`](drivers/qemu/docs/QEMU_TESTING_RESULTS.md) — informe de prueba de arranque completo

El flujo de trabajo de CI [`qemu-boot`](.github/workflows/qemu-boot.yml) crea el binario EFI, ensambla una imagen FAT32 GPT, lo inicia en QEMU (TCG cortex-a57 + EDK2 AArch64) en cada pulsación que toca `userland/experimental/` o `drivers/qemu/`, valida los ocho marcadores de inicio en las tres fases y confirma el registro en serie actualizado nuevamente en `drivers/qemu/sample-boot-log.txt`.

---

## Referencia CLI

```sh
# Compilar y ejecutar
compilación de código t81 <file.t81> -o <file.tisc>
ejecución de código t81 <file.t81|file.tisc> [--policy <apl>] [--weights-model <t81w>] [--trace]
respuesta del código t81
verificación de código t81 <file.t81>

# Inspección de máquinas virtuales
t81 máquina virtual ejecuta <file.tisc>
depuración de máquina virtual t81 <file.tisc>
t81 seguimiento de máquina virtual <file.tisc>

# Axion gobernanza
compilación de políticas t81 <file.apl>
Validación de política t81 <file.apl>
estado del axión t81
auditoría de axión t81

# Determinismo
t81 determinismo verificar-ejecutar <file.tisc> # ejecutar dos veces, comparar hashes
hash de determinismo t81 <file.tisc>
t81 determinismo certifica <file.tisc>

# Pesos del modelo
t81 pesos importar <model.safetensors|model.gguf> -o model.t81w
información de pesos t81 <model.t81w>
pesos t81 verificar <model.t81w>
t81 pesos cuantizan <input> --to-gguf <out>

# TISC código de bytes
t81 desastre tisc <file.tisc>
t81 tisc validar <file.tisc>
estadísticas de tisc t81 <file.tisc>

# Interfaces
estudio t81 # operador humano TUI
Agente t81 # TUI nativo de IA
```

---

## Verificación del determinismo

```sh
./scripts/ci/run_determinism_slice.sh
```

La puerta de determinismo multiplataforma de CI se ejecuta en cada envío a `main` y según una programación diaria. Cualquier divergencia de hash en una superficie determinista verificada bloquea la fusión.

---

## Documentación

| Tema | Ubicación |
| :--- | :--- |
| Primeros pasos (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| Primeros pasos (IA) | `docs/user-guide/getting-started/ai-quickstart.md` |
| Guías TUI | `docs/user-guide/how-to/tui-guide.md` |
| Especificación ISA | `spec/tisc-spec.md` |
| Axion Manual de Políticas | `docs/user-guide/tutorials/axion-policy-manual.md` |
| Referencia T81Lang Stdlib | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| Descripción general de la arquitectura | `docs/architecture/OVERVIEW.md` |
| Carta de Gobernanza | `docs/governance/README.md` |
| Centro de control de proyectos | `docs/status/PROJECT_CONTROL_CENTER.md` |
| Resultados del punto de referencia de inferencia | [`benchmarks/results/inference_comparison.md`](benchmarks/results/inference_comparison.md) |

---

## Gobernanza

T81 Foundation opera bajo un modelo de **Gobernanza Continua (C2)**. Todas las contribuciones deben mantener:

- **paridad de ejecución determinista**: los hashes de seguimiento coinciden en todas las plataformas compatibles
- **coherencia arquitectónica**: los cambios en la superficie determinista requieren una revisión formal
- **autoridad de especificación** — `spec/` > `docs/architecture/` > `docs/`; Las superficies congeladas requieren un mayor golpe de versión.

El registro de superficie determinista se define en `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. Las clasificaciones de límites de superficie (DCP/no-DCP gobernado/experimental/fuera de alcance) se definen en RFC-0048.

---

## La ventaja ternaria

Si bien el hardware binario moderno está altamente optimizado para la computación de propósito general, el **ternario balanceado** ({−1, 0, +1}) tiene propiedades estructurales que importan específicamente en la ejecución determinista, la inferencia de IA gobernada y las cargas de trabajo neuronales de baja complejidad.

### 1. Negación O(1): propagación de acarreo cero

La negación binaria en complemento a dos es un NO bit a bit seguido de +1, lo que puede desencadenar largas cadenas de acarreo. La negación ternaria equilibrada invierte +1 ↔ −1 y deja 0 sin cambios - **sin acarreo, tiempo constante**.

Medido: la negación de PackedCell alcanza **~49,9 G-ops/s** en hardware x86\_64 reciente, **~10,9 veces más rápido** que la negación entera optimizada de 64 bits (verificada en Linux x86\_64 y macOS ARM64).

### 2. Economía de base superior

La base óptima de la teoría de la información es *e ≈ 2,718*. El ternario (base 3) está más cerca que el binario (base 2), y ofrece **~1,585 bits de información por trit** (log₂3). Mayor entropía por dígito, rangos simétricos más compactos, especialmente útiles para pesos, incrustaciones y tensores dispersos.

### 3. Determinismo inherente de bits exactos

IEEE 754 sufre modos de redondeo específicos de la plataforma, diferencias de asociatividad y manejo anormal. El ternario equilibrado es simétrico alrededor de cero: el redondeo es un truncamiento sin sesgo direccional. Cada ruta de ejecución produce **hashes de seguimiento CanonHash81 idénticos** en las plataformas compatibles.

### 4. Inferencia neuronal sin multiplicación

Los pesos ternarios {−1, 0, +1} reducen los productos escalares a suma/resta condicional; no se requiere unidad multiplicadora. Combinado con los seis códigos de operación de inferencia TISC:

- Reducción de energía entre 15 y 60 veces frente a las líneas de base del FP16/FP32
- Ganancia de rendimiento de 4 a 90 veces con una precisión comparable
- Se alinea con la investigación de transformadores ternarios BitNet b1.58, xTern y 2024-2026

El formato T81 Peso ternario (T81WTN) y `t81 weights import` hacen que esto esté listo para producción en la pila hoy.

### 5. Ganchos de gobernanza a nivel trit

Debido a que TISC ISA es nativo ternario, el kernel Axion puede interceptar y auditar transiciones de estado en **granularidad de nivel trit** antes de cualquier efecto secundario. Esto permite la aplicación de políticas cerradas ante fallos, puertas éticas detalladas y pistas de auditoría deterministas que son fundamentalmente más inspeccionables que la ejecución binaria de caja negra.

---

## Licencia

Licencia Apache 2.0.

---

<details>
<summary>Nota de arranque honesta (marzo de 2026)</summary>

T81 está diseñado como un sistema operativo independiente con su propio ISA y kernel, pero todavía no existe ningún hardware ternario nativo. La vista previa actual se ejecuta como una capa invitada en Linux/macOS/Windows mediante archivos binarios, Docker o QEMU.

Se trata de un andamiaje temporal, de la misma manera que los primeros Linux se ejecutaban en simuladores antes que en el hardware real. El arranque de metal desnudo está en Alpha; el objetivo es eventualmente escapar por completo de la dependencia del sistema operativo host.

Gracias por leer hasta aquí.

</details>
