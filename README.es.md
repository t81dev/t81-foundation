<p align="center">
  <img src="assets/banner.png" alt="T81 — Una Arquitectura de Computación Ternaria" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 — Una Arquitectura de Computación Ternaria

![Release](https://img.shields.io/badge/release-v1.9.0--Stable-blue)
![Tests](https://img.shields.io/badge/tests-369%2F369_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

Aprovechando la eficiencia teórica de la computación en base-e, **T81** es una arquitectura de computación determinista construida sobre **aritmética ternaria equilibrada** ({-1, 0, +1}) con un modelo de gobernanza de cadena completa que cubre el conjunto de instrucciones, la máquina virtual, el compilador del lenguaje y el entorno de inferencia de IA.

La arquitectura ofrece:

- **reproducibilidad exacta de bits en superficies verificadas** — las superficies deterministas gobernadas producen hashes de rastreo idénticos en las plataformas compatibles
- **inferencia de IA gobernada** — el motor de políticas Axion intercepta y audita cada operación privilegiada antes de que tenga efectos secundarios
- **procedencia dirigida por contenido** — CanonFS registra todos los artefactos, pesos del modelo y estado en tiempo de ejecución de forma inmutable
- **ejecución paralela determinista para semántica de época gobernada** — el modelo de gráfico de tareas DPE (RFC-DPE-0002) permite cargas de trabajo TISC concurrentes con salidas comprometidas por época bajo la superficie determinista actual

---

## Tabla de Contenidos
- [Estado del Proyecto](#estado-del-proyecto)
- [Descripción General de Arquitectura y Ecosistema](#descripción-general-de-arquitectura-y-ecosistema)
- [Componentes Clave](#componentes-clave)
- [Inicio Rápido](#inicio-rápido)
- [Verificación de Determinismo](#verificación-de-determinismo)
- [Documentación](#documentación)
- [Gobernanza](#gobernanza)
- [La Ventaja Ternaria](#la-ventaja-ternaria)
- [Licencia](#licencia)

---

## Estado del Proyecto

**Fase: Desarrollo Activo** — v1.9.0-Stable; 369/369 pruebas pasadas; determinismo multiplataforma verificado en Linux x86\_64 + macOS ARM64.

La clasificación de la superficie determinista sigue el modelo de gobernanza introducido por el Registro de Superficie de Determinismo y RFC-0048:

- **DCP / superficie determinista verificada**: la semántica, el límite y la prueba de repetición están gobernados y aplicados por CI
- **Gobernado no DCP**: limitado por políticas y arquitectónicamente importante, pero aún no tiene derecho a afirmaciones deterministas completas
- **Experimental**: superficie de diseño o validación activa; ninguna afirmación de garantía determinista

| Componente | Madurez | Notas |
| :--- | :--- | :--- |
| **TISC ISA** | ❄️ Frozen | v1.9.0; la semántica de los opcodes es inmutable bajo v1.x; `AgentInvoke` (RFC-0015), 6 de inferencia nativa ternaria (RFC-0034), 3 FFI (RFC-00B8), 2 de criptografía de red (RFC-0038), 1 de anillo KEM (RFC-0039) |
| **Data Types** | ❄️ Frozen | BigInt, Float, Complex, Map, Set — codificación estable de bits; auditoría limpia |
| **T81VM** | ✅ Stable | DCP / superficie determinista verificada para el intérprete y la paridad de rastreo actual de la plataforma compatible; despacho completo de TISC v1.9.0 con `AgentInvoke`, inferencia nativa ternaria, FFI, criptografía de red y opcodes NTRU-KEM; 369/369 pruebas |
| **T81Lang** | ✅ Stable | Gobernado no DCP en general: especificación v1.9.0 Stable con controles de determinismo del compilador activos, pero la emisión del compilador sigue estando parcialmente verificada en lugar de ser promovida completamente como una superficie determinista verificada |
| **Axion Governance Kernel** | ✅ Stable | Gobernado no DCP en general: las cadenas de razones canónicas y los ganchos de auditoría están activos, pero la superficie completa del kernel/gobernanza es más amplia que el registro determinista verificado actualmente |
| **Ternary-Native Inference** | ✅ Stable | Gobernado no DCP: la superficie opcode/runtime/stdlib de RFC-0034 + RFC-0037 está implementada y evidenciada, pero no todas las rutas de ejecución adyacentes a la inferencia se promueven aún como superficies deterministas verificadas |
| **Lattice Cryptography** | ✅ Stable | Gobernado no DCP: la superficie RFC-0038+0039 está implementada y limitada por políticas; la promoción determinista sigue siendo específica de la superficie en lugar de implícita para todo el vertical criptográfico |
| **Governed FFI** | ✅ Stable | Gobernado no DCP: el puente de lenguaje/VM de RFC-00B8 + RFC-0036 está implementado de extremo a extremo, pero la caja de arena y la promoción de esquema más amplia permanecen abiertas antes de afirmaciones deterministas más sólides |
| **TUI Frontends** | ✅ Beta | Gobernado no DCP: las TUI de operador y agente son interfaces utilizables en producción, pero la integración UI/runtime no es en sí misma una superficie determinista verificada |
| **DPE (Parallel Execution)** | ✅ Stable | Modelo de ejecución determinista gobernado con RFC-DPE-0001–0009 aceptados; la semántica de época determinista está en su lugar, mientras que la promoción de la superficie más amplia sigue gobernada por el registro y la cadena RFC complementaria |
| **Cognitive Tiers** | ✅ Beta | Experimental / no DCP: la cognición de Nivel 4 (Tier4 Cognition) permanece limitada por la gobernanza pero no es una superficie determinista verificada |
| **TernaryOS User Environment** | ✅ Beta | Gobernado no DCP / beta: implementado y limitado por políticas, pero no se presenta actualmente como una superficie determinista verificada |
| **Axion OS** | ✅ Alpha | Gobernado no DCP / alpha: arquitectura de gobernanza activa, pero aún no es una superficie determinista verificada promovida en su totalidad |

---

## Descripción General de Arquitectura y Ecosistema

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

### Componentes Clave

**TISC ISA v1.9.0** — Arquitectura de Conjunto de Instrucciones Ternarias. Congelado bajo v1.x; el contrato de ejecución inmutable para toda la pila.

**T81VM** — Intérprete TISC determinista. Garantiza salidas idénticas en bits entre plataformas; el aislamiento previo al despacho de Axion mantiene los ganchos de gobernanza fuera de la ruta de ejecución principal.

**Axion Governance Kernel** — Motor de políticas que intercepta `AXREAD`, `AXSET`, `AXVERIFY`, opcodes de IA y llamadas FFI antes de cualquier efecto secundario. Falla cerrada (fail-closed) en caso de fallo de análisis de política.

**CanonFS** — Sistema de archivos direccionado por contenido. Almacena todos los objetos de código, pesos del modelo y artefactos de tiempo de ejecución como objetos inmutables identificados por hash. Proporciona procedencia para las auditorías de determinismo.

**T81Lang** — Lenguaje de alto nivel dirigido al código de bytes TISC. Tipos nativos: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`. Canalización del compilador: lexer → parser → AST tipado → análisis semántico → generación IR.

**Ternary-Native Inference (RFC-0034)** — Seis opcodes TISC para inferencia de IA sin multiplicación utilizando pesos ternarios equilibrados {−1, 0, +1}: `TWMATMUL` (matmul), `TQUANT` (cuantificar a trit), `TATTN` (atención ternaria), `TWEMBED` (incrustación de peso), `TERNACCUM` (producto punto escalar), `TACT` (activación con puerta de techo Axion). Formato de peso T81WTN. Frontend T81Lang `foreign {}` completo a través de RFC-0036.

**Governed FFI (RFC-00B8 + RFC-0036)** — Interfaz de función foránea gobernada de pila completa. Capa VM (RFC-00B8 Fase 1): `FFIDispatcher` impone comprobaciones de políticas, cuotas de recursos y pistas de auditoría antes de cualquier llamada foránea; `FFILibraryRegistry` rastrea las bibliotecas registradas por nombre y hash de versión; tres opcodes VM (`FFICall`, `FFIRegister`, `FFIPolicySet`). Capa de lenguaje (RFC-0036): `foreign deterministic { fn sin(x: T81Float) -> T81Float; }` declara firmas; `foreign.sin(angle)` en los sitios de llamada se reduce a `FFI_CALL` con el nombre de la función llevado en `text_literal`.

**TUI Frontends** — Dos interfaces de terminal complementarias construidas sobre FTXUI v5.0.0:
- `t81 studio` — barra lateral de navegación, navegador CanonFS, panel Axion, visualizador de rastreo de determinismo, paleta de comandos (`Ctrl+P`)
- `t81 agent` — sesión JSONL persistente, comandos de barra diagonal (`/compile`, `/run`, `/hash`, `/allow`, `/infer`, `/trits`, …), barra de probabilidad trit

**DPE (Deterministic Parallel Execution)** — Modelo de gráfico de tareas sobre TISC ISA congelado. Las tareas declaran entradas inmutables y regiones de salida almacenadas en búfer; la VM confirma todas las escrituras atómicamente al final de la época. No se requieren nuevos opcodes.

---

## Inicio Rápido

### Construir desde la fuente

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

## Verificación de Determinismo

Las superficies deterministas verificadas se comprueban para la reproducibilidad multiplataforma exacta de bits.

```bash
./scripts/ci/run_determinism_slice.sh
```

Plataformas verificadas para la superficie central actual: **Linux x86_64**, **macOS ARM64**. Cualquier divergencia en una superficie determinista verificada es un defecto crítico.

---

## Documentación

| Tema | Ubicación |
| :--- | :--- |
| Primeros pasos (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| Primeros pasos (IA) | `docs/user-guide/getting-started/ai-quickstart.md` |
| Guía TUI | `docs/user-guide/how-to/tui-guide.md` |
| Especificación ISA | `spec/tisc-spec.md` |
| Manual de Política de Axion | `docs/user-guide/tutorials/axion-policy-manual.md` |
| Referencia Stdlib T81Lang | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| Descripción General de Arquitectura | `docs/architecture/OVERVIEW.md` |
| Carta de Gobernanza | `docs/governance/README.md` |
| Centro de Control del Proyecto | `docs/status/PROJECT_CONTROL_CENTER.md` |

---

## Gobernanza

La Fundación T81 opera bajo un modelo de **Gobernanza Continua (C2)**. Todas las contribuciones deben mantener:

- **paridad de ejecución determinista** — los hashes de rastreo deben coincidir en las plataformas compatibles
- **coherencia arquitectónica** — los cambios que tocan la superficie determinista requieren revisión formal
- **garantías de reproducibilidad** — ningún no determinismo de punto flotante o específico de la plataforma en la superficie DCP

La superficie determinista está definida en `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. Los cambios a las superficies congeladas (TISC ISA, Tipos de Datos) requieren un incremento de versión principal.

> **Nota de límite:** Las clasificaciones DCP, gobernadas no DCP, experimentales y fuera del alcance se definen constitucionalmente en RFC-0048. Los documentos públicos no deben presentar superficies gobernadas no DCP o experimentales como componentes deterministas verificados.

---

## La Ventaja Ternaria

Si bien el hardware binario moderno está altamente optimizado para la computación de propósito general, la **Fundación T81** explota las propiedades matemáticas y estructurales únicas del **ternario equilibrado** ({−1, 0, +1}) para ofrecer ventajas que son difíciles o imposibles de lograr en los sistemas binarios convencionales — especialmente en la ejecución determinista, la inferencia de IA gobernada y las cargas de trabajo neuronales de baja complejidad.

### 1. Simetría Computacional O(1) — Negación Sin Acarreo

En el binario de complemento a dos, la negación requiere un NOT bit a bit seguido de +1, lo que puede desencadenar largas cadenas de acarreo. En el ternario equilibrado, la negación es simplemente invertir el signo de cada trit distinto de cero (+1 ↔ −1, 0 se mantiene 0) — **propagación de acarreo cero**, tiempo constante.

- **Rendimiento medido**: La negación de PackedCell alcanza **~49.9 G-ops/s** en hardware x86_64 reciente, superando la negación entera optimizada de 64 bits por **~10.9×** (benchmarks verificados en Linux x86_64 y macOS ARM64).
- Esta simetría se extiende naturalmente a muchos patrones aritméticos, reduciendo la latencia y la potencia en operaciones pesadas de signos.

### 2. Economía de Base Superior y Densidad Teórica

La base óptima de la teoría de la información para los sistemas de números posicionales está cerca de *e ≈ 2.718*. El ternario (base 3) está matemáticamente más cerca que el binario (base 2), entregando **~1.585 bits de información por trit** (log₂(3)).

- En la práctica, esto permite una mayor entropía por dígito y una representación más compacta de los rangos simétricos — especialmente valioso para pesos de redes neuronales, incrustaciones, sistemas de coordenadas y tensores dispersos a gran escala.

### 3. Determinismo Inherente de Bits Exactos y Redondeo Independiente de la Plataforma

El punto flotante IEEE 754 sufre modos de redondeo específicos de la plataforma, diferencias de asociatividad y manejo subnormal que rompen la reproducibilidad. La aritmética ternaria equilibrada es naturalmente simétrica alrededor de cero:

- El redondeo usa un simple truncamiento — ningún sesgo dependiente de la dirección.
- Cada ruta de ejecución produce **hashes de rastreo idénticos** en las plataformas compatibles (actualmente Linux x86_64 + macOS ARM64 verificado al 100% de paridad).
- Esto proporciona una reproducibilidad sólida como una roca para la computación científica, la auditoría de inferencia de IA y las ejecuciones de agentes gobernados.

### 4. Inferencia Neuronal Sin Multiplicación

Los pesos ternarios equilibrados {−1, 0, +1} permiten **productos punto sin multiplicación** — reemplazar MUL con ADD/SUB condicional (o acumulación pura al omitir los ceros). Combinado con los opcodes TISC personalizados (`TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT`):

- Permite una potencia drásticamente menor y un mayor rendimiento para la inferencia de IA.
- Se alinea con las tendencias de 2024-2026 en modelos extremos de bits bajos (BitNet b1.58, xTern, transformadores ternarios), ofreciendo una reducción de energía de 15-60× y un aumento de rendimiento de 4-90× frente a las líneas de base FP16/FP32 con una pérdida mínima de precisión.
- Formato de peso T81WTN + operaciones nativas ternarias hacen que esta ventaja esté lista para la producción en la pila.

### 5. Gobernanza Arquitectónica y Ganchos de Seguridad

Dado que todo el TISC ISA es nativo ternario, el **Axion Governance Kernel** puede interceptar y auditar las transiciones de estado a **nivel de trit de granularidad** antes de que ocurra cualquier efecto secundario. Esto permite:

- Aplicación de políticas con falla cerrada en operaciones privilegiadas (invocaciones de IA, llamadas FFI, comportamientos de agentes).
- Puertas éticas de grano fino, seguimiento de procedencia a través de CanonFS y pistas de auditoría deterministas.
- Un modelo de seguridad que es fundamentalmente más inspeccionable que la ejecución binaria de caja negra.

Estas ventajas se combinan en dominios donde **la reproducibilidad**, **la inferencia de baja complejidad**, **la ejecución gobernada** y **la simetría matemática** importan más — exactamente los casos de uso objetivo de la arquitectura T81.

---

## Licencia

Licencia MIT.
