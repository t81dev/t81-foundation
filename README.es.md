<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# Fundación T81 — Pila de computación ternaria determinista

![Release](https://img.shields.io/badge/release-v1.9.0--Stable-blue)
![Tests](https://img.shields.io/badge/tests-369%2F369_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.2.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

Aprovechando la eficiencia teórica de la computación en base-e, la **Fundación T81** es una pila de computación determinista construida sobre **aritmética ternaria equilibrada** ({-1, 0, +1}) con un modelo de gobernanza de cadena completa que cubre el conjunto de instrucciones, la máquina virtual, el compilador del lenguaje y el entorno de inferencia de IA.

La pila ofrece:

- **reproducibilidad exacta de bits** — cada ruta de ejecución produce un hash de rastreo idéntico en las plataformas compatibles
- **inferencia de IA gobernada** — el motor de políticas Axion intercepta y audita cada operación privilegiada antes de que tenga efectos secundarios
- **procedencia dirigida por contenido** — CanonFS registra todos los artefactos, los pesos del modelo y el estado en tiempo de ejecución de forma inmutable
- **ejecución paralela determinista** — el modelo de gráfico de tareas DPE (RFC-DPE-0002) permite cargas de trabajo TISC concurrentes con salidas comprometidas por época

---

## Estado del proyecto — Marzo 2026

**Fase: Desarrollo Activo** — v1.9.0-Stable; 369/369 pruebas pasadas; determinismo multiplataforma verificado en Linux x86\_64 + macOS ARM64.

| Componente | Madurez | Notas |
| :--- | :--- | :--- |
| **TISC ISA** | ❄️ Frozen | v1.2.0; la semántica de los opcodes es inmutable bajo v1.x; 12 nuevos opcodes desde v1.1: `AgentInvoke` (RFC-0015), 6 de inferencia nativa ternaria (RFC-0034), 3 FFI (RFC-00B8), 2 de criptografía de red (RFC-0038), 1 de anillo KEM (RFC-0039) |
| **Data Types** | ❄️ Frozen | BigInt, Float, Complex, Map, Set — codificación estable de bits; auditoría limpia del 2026-02-27 |
| **T81VM** | ✅ Stable | Despacho completo de TISC v1.2; `AgentInvoke` + inferencia nativa ternaria + FFI + criptografía de red + opcodes NTRU-KEM; 369/369 pruebas |
| **T81Lang** | ✅ Stable | especificación v1.3 Stable; `agent`/`behavior` (RFC-0015); `foreign {}` FFI (RFC-0036); `std.tnn.*` TNN stdlib (RFC-0037); `std.crypto.*` criptografía de red + NTRU-KEM (RFC-0038/0039); soporte para identificadores contextuales en todo |
| **Axion Governance Kernel** | ✅ Stable | Satisfacción de Seguridad P4 y P5 de Instrucciones Privilegiadas; cadenas de razones canónicas AX-M6; cada compuerta de activación de `AgentInvoke` + `TACT` emite un evento de auditoría |
| **Ternary-Native Inference** | ✅ Stable | RFC-0034 + RFC-0037: `TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT`; `std.tnn.*` T81Lang stdlib (6 funciones integradas → opcodes TISC); inferencia libre de multiplicación; formato de peso T81WTN; 13/13 pruebas; operaciones de inferencia ternaria listas para producción |
| **Lattice Cryptography** | ✅ Stable | RFC-0038+0039: `POLYMUL`, `POLYMOD`, `TVecSub`; anillo completo {+,−,×,mod} sobre Z\[x\]/(x^n+1); `std.crypto.{polyadd,polysub,polymul,polymod,ntru_encrypt,ntru_decrypt}`; 37/37 pruebas; criptografía de red lista para producción |
| **Governed FFI** | ✅ Stable | RFC-00B8 + RFC-0036: `FFIDispatcher`, `FFILibraryRegistry`, 3 opcodes de VM; `foreign [policy] { fn … }` gramática de T81Lang; `foreign.<name>(args)` → `FFI_CALL`; 9/9 pruebas AC; interfaz de funciones foráneas gobernada lista para producción |
| **TUI Frontends** | ✅ Beta | `t81 studio` (operador humano) + `t81 agent` (nativo en IA); FTXUI v5.0.0; RFC-0033 aceptado; interfaces de terminal listas para producción |
| **T81Graph** | ✅ Beta | Reducción de opcodes de VM + serialización del lado del lenguaje cableada; verificación DCP completa; 6/6 pruebas |
| **DPE (Parallel Execution)** | ✅ Stable | RFC-DPE-0001–0009 todos aceptados; gráfico de tareas, anillo de historial de época, eventos de auditoría de época, tiempo de espera completamente implementado; ejecución paralela determinista lista para producción |
| **Cognitive Tiers** | ✅ Beta | Cognición de Nivel 4 (RFC-0021): `Tier4Loop`, `SelfModel` (anillo de 81 entradas), `RecursiveImprovementBounds`, `TierAwarePlanner`; 4 conjuntos de pruebas pasadas; arquitectura cognitiva experimental lista para pruebas beta |
| **Benchmark Suite** | ✅ Stable | RFC-00A2: rendimiento de VM + validación de determinismo de CanonHash81 (`score=1.0` en todas las ejecuciones); `t81 internal benchmark`; validación de rendimiento lista para producción |
| **TernaryOS User Environment** | ✅ Beta | RFC-00B9: t81-init, administrador de sesiones, shell t81sh; 15/15 criterios de aceptación implementados; secuencia de arranque, ciclo de vida de sesión y arquitectura de shell en funcionamiento |
| **Cross-Platform Determinism CI** | ✅ Stable | Flujo de trabajo diario de GitHub Actions que compara hashes de código de bytes de T81Lang en Linux x86\_64 (gcc-14) y macOS ARM64 (clang); registro de evidencia auditable públicamente; validación de determinismo multiplataforma lista para producción |
| **Hanoi VM** | ✅ Alpha | RFC-0000 §4 arranque centrado en la ética; programador determinista de 81 ranuras; gestión de instantáneas; RFC-0000 §7 superficie de comandos (status, optimize, simulate, snapshot, rollback); conjunto de pruebas completo; microkernel listo para Alpha |
| **Axion OS Kernel** | ✅ Alpha | Sistema completo de gobernanza con 100% de cobertura de pruebas (28/28 pruebas); motor de políticas listo para producción y evaluación de ética; Principios Θ₁-Θ₉ completamente implementados; documentación completa de API y ejemplos de integración; kernel listo para Alpha con toma de decisiones determinista y completa integración con la pila T81 |

---

## Arquitectura

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

### Componentes clave

**TISC ISA v1.2** — Arquitectura de Conjunto de Instrucciones Ternarias. Congelado bajo v1.x; el contrato de ejecución inmutable para toda la pila. v1.2 añade 9 opcodes: `AgentInvoke` (RFC-0015), seis operaciones de inferencia nativa ternaria (RFC-0034), y tres operaciones FFI gobernadas (RFC-00B8).

**T81VM** — Intérprete TISC determinista. Garantiza salidas con bits idénticos entre plataformas; El aislamiento de pre-despacho de Axion mantiene los ganchos de gobernanza fuera de la ruta de ejecución en caliente. Despacho completo de TISC v1.2 incluyendo la inferencia nativa ternaria y FFI.

**Axion Governance Kernel** — Motor de políticas que intercepta `AXREAD`, `AXSET`, `AXVERIFY`, opcodes de IA, y llamadas FFI antes de cualquier efecto secundario. Cierre ante falla de análisis de políticas. Certificado Stable en 2026-03-15 con 54/54 pruebas pasadas.

**CanonFS** — Sistema de archivos direccionado por contenido. Almacena todos los objetos de código, pesos del modelo y artefactos de tiempo de ejecución como objetos grandes binarios (blobs) inmutables e identificados por hash. Proporciona procedencia para auditorías de determinismo.

**T81Lang** — Lenguaje de alto nivel dirigido al código de bytes TISC. Tipos nativos: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`. Declaraciones de primera clase de `agent { behavior }` compilan a `AGENT_INVOKE` con auditoría de Axion (RFC-0015). Los bloques `foreign [policy] { fn … }` declaran funciones externas gobernadas que llaman a través de `FFI_CALL` (RFC-0036). `agent`, `behavior` y `foreign` son utilizables como identificadores contextuales en todas las expresiones y posiciones de enlace. Canalización del compilador: lexer → parser → AST tipado → análisis semántico → generación de IR.

**Ternary-Native Inference (RFC-0034)** — Seis opcodes TISC para inferencia de IA libre de multiplicación utilizando pesos ternarios balanceados {−1, 0, +1}: `TWMATMUL` (matmul), `TQUANT` (cuantificar a trit), `TATTN` (atención ternaria), `TWEMBED` (inserción de peso), `TERNACCUM` (producto punto escalar), `TACT` (activación con la compuerta de límite de Axion). Formato de peso T81WTN. Frontend de T81Lang `foreign {}` completo a través de RFC-0036.

**Governed FFI (RFC-00B8 + RFC-0036)** — Interfaz de funciones foráneas gobernada para todo el stack. Capa de VM (RFC-00B8 Phase 1): `FFIDispatcher` aplica las comprobaciones de las políticas, cuotas de recursos y pistas de auditoria antes de cualquier llamada foránea; `FFILibraryRegistry` rastrea las bibliotecas registradas por nombre y hash de versión; tres opcodes de VM (`FFICall`, `FFIRegister`, `FFIPolicySet`). Capa de Lenguaje (RFC-0036): `foreign deterministic { fn sin(x: T81Float) -> T81Float; }` declara las firmas; `foreign.sin(angle)` en los sitios de la llamada se reduce a `FFI_CALL` con el nombre de la función llevado en `text_literal`. Pasan las nueve pruebas de aceptación.

**TUI Frontends** — Dos interfaces de terminal complementarias construidas sobre FTXUI v5.0.0:

- `t81 studio` — barra lateral de navegación, explorador CanonFS, panel de Axion, visualizador de rastreo de determinismo, paleta de comandos (`Ctrl+P`)
- `t81 agent` — sesión JSONL persistente, comandos de barra diagonal (`/compile`, `/run`, `/hash`, `/allow`, `/infer`, `/trits`, …), barra de probabilidad trit

**DPE (Deterministic Parallel Execution)** — Modelo de gráfico de tareas sobre la ISA de TISC congelada. Las tareas declaran entradas inmutables y regiones de salida en búfer; la VM confirma todas las escrituras atómicamente al final de la época. No requiere opcodes nuevos.

---

## Inicio Rápido

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

Indicadores de compilación opcionales:

| Indicador | Por Defecto | Propósito |
| :--- | :--- | :--- |
| `T81_BUILD_TUI` | `ON` | Interfaces frontales TUI basadas en FTXUI |
| `T81_BUILD_TESTS` | `ON` | Suite de pruebas completa |
| `T81_ENABLE_ASAN` | `OFF` | Sanitizador de direcciones |
| `T81_ENABLE_UBSAN` | `OFF` | Sanitizador UB |
| `T81_ENABLE_LLAMA_CPP` | `OFF` | Adaptador de inferencia gobernado llama.cpp |
| `T81_WARN_STRICT` | `OFF` | Modo estricto de escaneo de advertencias (usado por el preajuste `warn-strict`) |

**Escaneo de advertencia previo al push** — refleja las verificaciones `-Wswitch`, `-Wunused-variable`, y `-Wunused-function` aplicadas por Windows CI, encontrando problemas localmente en aproximadamente 2 minutos en lugar de esperar a la matriz completa:

```bash
cmake --preset warn-strict
cmake --build build-warn-strict 2>&1 | head -40
```

---

## Verificación de Determinismo

Cada lanzamiento está verificado en cuanto a la reproducibilidad de determinismo exacto de bits a través de múltiples plataformas.

```bash
./scripts/ci/run_determinism_slice.sh
```

Plataformas verificadas: **Linux x86_64**, **macOS ARM64**. Cualquier divergencia en los hashes del rastreador de la VM es un defecto crítico.

---

## Documentación

| Tema | Ubicación |
| :--- | :--- |
| Primeros pasos (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| Primeros pasos (AI) | `docs/user-guide/getting-started/ai-quickstart.md` |
| Guía de la TUI | `docs/user-guide/how-to/tui-guide.md` |
| Especificación de la ISA | `spec/tisc-spec.md` |
| Manual de la Política de Axion | `docs/user-guide/tutorials/axion-policy-manual.md` |
| Referencia de la Stdlib de T81Lang | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| Resumen de Arquitectura | `docs/architecture/OVERVIEW.md` |
| Estatutos de Gobernanza | `docs/governance/README.md` |
| Centro de Control del Proyecto | `docs/status/PROJECT_CONTROL_CENTER.md` |

---

## Hoja de Ruta

| Hito | Objetivo | Descripción |
| :--- | :--- | :--- |
| Cierre del Mes C2 | 2026-03-31 | Auditoría del libro mayor de gobernanza; PASS pre-vuelo 2026-03-10 |
| Promoción a Axion Stable | ✅ **COMPLETADO 2026-03-15** | Cadenas de razón canónicas de AX-M6 implementadas; 54/54 pruebas pasando; listo para producción |
| Promoción a T81Graph Beta | ✅ **COMPLETADO 2026-03-15** | Reducción de opcode de VM completada; Verificación de DCP; 6/6 pruebas pasando |
| Política de interrupción RFC-00B5 | ✅ **COMPLETADO 2026-03-16** | Modelo de interrupción de eventos gobernados integrado; cortes 26-28 completos |
| RFC-0034 Inferencia Ternaria-Nativa | ✅ **COMPLETADO 2026-03-16** | 6 nuevos opcodes de TISC; inferencia sin multiplicación; puerta de techo de activación TACT; 5/5 pruebas de conformidad |
| RFC-00B8 FFI Gobernado (Fase 1) | ✅ **COMPLETADO 2026-03-16** | Despachador de FFI + registro de librerías; 3 opcodes de VM; línea de montaje de gobernanza; pistas de auditoría |
| CI de Determinismo Multiplataforma | ✅ **COMPLETADO 2026-03-16** | Flujo de trabajo diario de GitHub Actions; comparación de hash entre Linux x86\_64 + macOS ARM64; registro público de evidencias |
| RFC-0036 Gramática FFI de T81Lang | ✅ **COMPLETADO 2026-03-16** | Sintaxis `foreign [policy] {}`; `foreign.<name>(args)` → `FFI_CALL`; 9/9 pruebas de criterios de aceptación; enlaza el trabajo de la VM de RFC-0034 + RFC-00B8 hacia el frontend T81Lang |
| Etapa 2: Plataforma Verificada | ✅ **LOGRADO 2026-03-16** | Todas las metas de implementación completas; depurador de rastreos, CI multiplataforma, pruebas 365/365, frontend FFI — pila reproducible externamente |
| RFC-0037 TNN stdlib | ✅ **COMPLETADO 2026-03-16** | Funciones incorporadas de T81Lang `std.tnn.*` (6 funciones → opcodes TISC de RFC-0034); 13/13 pruebas; inferencia de multiplicación sin interrupción de extremo a extremo hasta la VM |
| RFC-0038 Criptografía de Red | ✅ **COMPLETADO 2026-03-16** | opcodes TISC `POLYMUL`/`POLYMOD`; Funciones integradas `std.crypto.polymul/polymod`; multiplicación polinómica negacíclica sobre {−1,0,+1}; exactitud a T81BigInt; 13/13 pruebas |
| Promoción de la especificación de T81Lang (v1.3) | ✅ **COMPLETADO 2026-03-16** | Promoción de RFC-0036/0037/0038 a la especificación normativa; se quita el bosquejo de §5.17; agregados §5.18/5.19; el registro de opcodes actualizado a 205 entradas |
| RFC-0039 NTRU-KEM | ✅ **COMPLETADO 2026-03-16** | opcode `TVecSub`; `std.crypto.{polyadd,polysub,ntru_encrypt,ntru_decrypt}`; Capa matemática C++ para KEM; 24/24 pruebas; anillo completo {+,−,×,mod} sobre Z\[x\]/(x^n+1) |
| TernaryOS bare-metal boot | TBD | Ejecución nativa bajo host x86\_64 en QEMU + devolución de evidencias por parte de CanonFS |

---

## Gobernanza

La Fundación T81 opera bajo un modelo de **Gobernanza Continua (C2)**. Todas las contribuciones deben mantener:

- **paridad de ejecución determinista** — los hashes de rastreo deben coincidir en las plataformas soportadas
- **coherencia arquitectónica** — los cambios que afectan a la superficie determinista requieren una revisión formal
- **garantías de reproducibilidad** — no puede haber coma flotante ni no-determinismos dependientes de plataforma dentro de la superficie del DCP

La superficie determinista está definida en `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. Los cambios a las superficies congeladas (TISC ISA, Tipos de Datos) requieren un incremento de versión principal.

> **Nota de límites:** Las superficies experimentales (Capas Cognitivas, Distribuidas, Trace-JIT, TernaryOS, adaptador de llama.cpp) están gobernadas sin DCP y no deben ser presentadas como componentes deterministas verificados.

---

## La Ventaja Ternaria

Aunque el hardware binario moderno está altamente optimizado, la **Fundación T81** aprovecha las características matemáticas únicas del **Ternario Equilibrado ({-1, 0, +1})** para lograr eficiencias estructurales que la tecnología binaria no puede replicar.

### 1. Simetría Computacional $O(1)$

En el complemento a dos del sistema binario, negar un número es una operación asimétrica (NOT + 1) que requiere propagación de acarreo. En T81, la negación es un simple trit-flip con **cero gastos adicionales en el acarreo**.

* **Rendimiento:** El rendimiento de la negación de T81 alcanza **~46.6 G-ops/s** (a través de `PackedCell`), superando la negación binaria optimizada de 64 bits por un factor de **10.4x**.

### 2. Mayor Economía Base

Basado en el teorema de que la base más eficiente para un sistema de números es $e \approx 2.718$, el ternario (Base 3) es matemáticamente más eficiente que el binario (Base 2).

* **Densidad de la Información:** T81 alcanza una densidad teórica de **1.58 bits por trit**. Esto se traduce a mayor entropía por ciclo de reloj y huellas de almacenamiento reducidas para la gran escala de sistemas coordinados y pesos de redes neuronales.

### 3. Determinismo de bits exacto

Las operaciones de punto flotante binarias (IEEE 754) frecuentemente sufren de las incertidumbres específicas de sus plataformas de no-determinismo a la hora de redondear. La aritmética equilibrada de T81 aporta:

* **Simetría Inherente:** El redondeo es hecho por simple truncación, porque el sistema está naturalmente centrado alrededor del cero.
* **Paridad de Rastreo:** 100% "Exactitud Roundtrip" sobre todas las plataformas testadas (Linux x86_64, macOS ARM64) con zero divergencia de los hashes rastreados de las VMs.

### 4. Direct Governance Hook

Debido a que TISC ISA es ternario por naturaleza, el **Kernel de Gobernanza de Axion** puede auditar las transiciones de estados con gran nivel de granularidad. Las operaciones de inferencia de las IA pueden ser interceptadas a nivel-trit, antes de que suceda cualquier efecto colateral, permitiéndole a la seguridad ser "fail-closed", un modelo de seguridad que arquitectónicamente es imposible de reproducir en ejecuciones binarias de la "caja negra".

---

# T81: Un Informe Completo de Sistemas y Subsistemas

## 1. Resumen Ejecutivo

T81 Foundation no es un solo programa. Es un repositorio en múltiples capas que combina un modelo de datos ternario, una ISA, una VM, una superficie de lenguaje/compilador, una capa de gobernanza, un almacenamiento direccionable a contenidos, mecanismos de pruebas y despliegues automáticos (CI), un sistema operativo / esfuerzo de entorno de desarrollo experimental, y un largo aparato de gobernanza / documentación. La raíz del repositorio demuestra un solo tamaño del sistema: `.github`, `benchmarks`, `book`, `contracts`, `core`, `docs`, `experimental`, `kernel`, `lang`, `runtime`, `spec`, `src`, `tests`, `tooling`, `tools`, sumado a `internal`, `legacy`, `notebooks`, `pdf`, y bienes orientados hacia diferentes lenguajes y clientes. El proyecto tiene 3.636 contribuciones y es mayormente en lenguaje C++ con ciertos y pequeños componentes de Python/CMake/Shell/C. ([GitHub][1])

La porción técnica más robusta es una cadena de control orientada a ejecuciones centralizadas en determinismos: Tipos de Datos, TISC, un camino de la T81VM de intérprete sin compilación a tiempo real (non-JIT), y los respectivos registros a los límites de una arquitectura a medida, conocidos como Determinism Registry / DCP. Son listadas explícitamente y son de uso específico bajo el “Perfil Nuclear del Determinismo” (Deterministic Core Profile o DCP), emparejadas al proceso de la validación del chequeo por parte de la CI y pruebas concisas. De la misma forma que una vista superficial, nos muestra las partes consolidadas o experimentales. ([GitHub][2])

A su vez la mayor debilidad a la que se enfrenta la misma radica sobre su consistencia a escala de gobierno de superficies: Los reportes de repositorios afirman diferentes fases / madurez en diversas de sus presentaciones a la misma vez: el README de la raíz declara “v1.9.0-Stable; 369/369 tests”, mientras que su contraparte la ventana de control asienta un “v1.4.1-Stable; 363/363 tests”, su CMakeLists.txt menciona una versión número `1.3.6`, su modelo para el documento de especificación es “Version 1.1 — Stable”, mientras que, de la misma manera que antes el README la menciona con una versión “TISC ISA v1.2,” y con un desarrollo en Axion de estado tanto “Estable” en la implementación del tablero como un desarrollo de estado “Alpha” en la parte normativa del archivo e interrumpiendo en su propia ventana experimental en proceso de avance en su correspondiente cuaderno de anotaciones. Nada de ésto le remueve el factor veracidad de su correcto y preciso ensamblado, pero hace gran efecto restándole su validación auditiva como así también todo vestigio de ser un sistema pulcro. ([GitHub][1])

## Licencia

MIT License.

## 2. Metodología de Evaluación

Esta evaluación se basa en seis fuentes de evidencia: la estructura e historial de la raíz del repositorio; las especificaciones normativas en `spec/`; los documentos de arquitectura, gobernanza y estado en `docs/`; las superficies de CI y compilación como `CMakeLists.txt` y `.github/workflows/ci.yml`; los paneles de madurez como `PROJECT_CONTROL_CENTER.md` y `IMPLEMENTATION_MATRIX.md`; y el registro de progreso experimental del SO bajo `experimental/ternaryos/docs/PROGRESS.md`. El orden propio de autoridad del repositorio es explícito: `/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`. ([GitHub][1])

Esto es, por lo tanto, una auditoría orientada sobre las superficies del sistema/especificaciones/implementaciones, no una verificación de fuente completa línea-por-línea de cada archivo C++. Donde el repositorio en sí mismo se define en estado de implementación, fases o etapas, yo trato a dichos aspectos como un estado ya superado en la etapa, a no ser que sea apoyado por una fuente adjunta como lo son sus chequeos por la CI, fases de demostración evidentes, o la consistencia en el cruce de documentos. Donde el propio repositorio tiene contradicciones, yo lo trato como una desviación de gobernanza. ([GitHub][3])

## 3. Mapa de Sistema-de-Sistemas

### Bases arquitectónicas

Propósito: definir los límites arquitectónicos, los modelos de autorización y la demarcación central y determinista. Ubicaciones principales: `docs/architecture/OVERVIEW.md`, `docs/product/DETERMINISTIC_CORE_PROFILE.md`, `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`, `spec/`. Madurez: relativamente alta como una forma de control de la documentación. Acoplamiento: todo depende de ésto. Riesgo clave: desviación de estados de los archivos contra la comunicación del alto y público nivel del núcleo. ([GitHub][2])

### Modelo de datos / modelo numérico

Propósito: datos numéricos ternarios nativos y su codificación en determinismo. Ubicaciones principales: `core/types/`, `spec/t81-data-types.md`, tests de relación listados en el DCP y registro. Madurez: entre los dominios más robustos; marcado como Frozen/Verified (Congelado/Verificado) en la matriz de implementación y en el DCP. Riesgo clave: El ámbito del coma flotante es cuidadosamente delimitado, pero su mensajería general puede causar confusión con aquellos mismos limitantes. ([GitHub][4])

### ISA / VM / runtime

Propósito: la sintaxis estática de la máquina en ejecución determinística. Ubicaciones principales: `core/isa/`, `core/vm/`, `runtime/`, `spec/tisc-spec.md`, `spec/t81vm-spec.md`. Madurez: la parte del compilador del TISC en ruta es donde comienza lo central-operativo; el T81VM queda como una etapa de Beta aun cuando la pizarra indique que su estado es Stable. Riesgo clave: desfases de sincronización y carencia analógica con JIT. ([GitHub][2])

### Superficies lenguaje / compilador

Propósito: Lenguaje de alto-nivel de compilación para apuntar solo hacia la TISC. Ubicaciones principales: `lang/`, `spec/t81lang-spec.md`, archivos del reporte y accesorios nombrados explícitamente en el cuerpo del documento. Madurez: superior a la que presentan diversos lenguajes de investigación debido a una gramática normada, la demostración de todo el trayecto desde el compilador y notas puntuales de su proceso de determinismo. Riesgo clave: lo expuesto sobre su desarrollo es superior a lo que se ve expuesto visible y textualmente de la información proveniente aquí. ([GitHub][5])

### Gobernanza / política / Axion

Propósito: monitoreo de los permisos especiales, los diagnósticos y las ejecuciones visibles. Ubicaciones principales: `kernel/axion/`, `spec/axion-kernel.md`, información referente de manera general, y datos informativos ligados hacia los recorridos de revisión. Madurez: media. Es real pero no en el sentido de que tenga algo a ser utilizado aún en base al compilador o a otras operaciones y en algunas cuestiones descritas son solo de origen introductorio. Riesgo clave: El mensaje dado supera el desarrollo final en diversas partes operacionales. ([GitHub][6])

### Almacenamiento / CanonFS / modelo de objetos

Propósito: Inmutabilidad centrada en su contenido de origen con un alcance permanente. Ubicaciones principales: `src/canonfs/`, `include/t81/canonfs/`, información técnica anexa y progreso de SO de Axion. Madurez: de estado contenido/estable en resumen, pero aún son especulativos en los trabajos de las experimentaciones de sistemas en SO. Riesgo clave: los resultados logrados actualmente y lo propuesto sobrepasa el nivel general/núcleo llegando también hacia las partes experimentales en OS y su base de alojamiento de trabajo. ([GitHub][2])

### Núcleo / SO / Axion OS surfaces

Propósito: base estructural que conforma el operativo, gestor, procesador, rutinas MMU y del tipo HAL, sistemas de arranque e intercomunicaciones. Ubicaciones principales: `experimental/ternaryos/`, `kernel/`, información referida sobre los procesos al OS. Madurez: host en desarrollo de fases experimentales que avanza y demuestra consistencias fuertes, aún no está consolidado al completo. Riesgo clave: confunciones acerca del Axion Kernel y el Axion-como-kernel de gobernanza operativo. ([GitHub][7])

### Inteligencia Artificial / IA

Propósito: Inferencias Nativas Ternarias, procesos de razonamiento distribuido, ambientes manejados, las integraciones tipo FFI y experimentaciones AI de forma genérica. Ubicaciones principales: `experiments/ ai`, `experimental/`, referencias y propuestas a otras arquitecturas AI en README. Madurez: Mixta; La construcción de la parte de los inferencia se demostraron aplicadas y bien armadas en cuanto su uso real, mientras que otros sistemas más pesados en torno y ligados o propuestos para esta rama quedan por debajo y hasta siendo excluidas por parte de las especificaciones y normativas propias del proyecto en su rama Deterministic Core. Riesgo clave: abarcar demasiado. ([GitHub][1])

### Testing / benchmarks / CI enforcement

Propósito: forzar la estabilidad estructural de todo el sistema y asegurar determinismos en normativas referenciadas para lograr una consistencia. Ubicaciones principales: `.github/workflows`, `benchmarks/`, `tests/`, `scripts/ci/`, `scripts/governance/`. Madurez: Alta frente al nivel que presenta este proyecto. Riesgo clave: es ambicioso debido a sus trabajos e informaciones dadas a que muchas de las acciones en uso de su pipeline son con base en recolectar información e incompatibles las unas y otras. ([GitHub][1])

### Documentación / comunicación pública

Propósito: normativas e informaciones anexadas con especificaciones. Ubicaciones principales: `docs/`, `book/`, Lenguajes en su formato README y sus diversas traducciones en idioma. Madurez: Es elevado por sí mismo en cantidades generales, se cruzan las bases con la calidad del núcleo que manejan sus actualizaciones y el seguimiento de su integridad. Riesgo clave: El desbalance ocasiona sobre exceso de ramas y no ser un trabajo constante que apunte al punto central al final. ([GitHub][1])

### Legado / interno / notebooks / pdf / archivo

Propósito: soporte y material generado de usos pasados que ya formaron o pasaron por revisiones al sistema. Ubicaciones principales: `legacy`, `internal`, `notebooks`, `pdf`, `artifacts/archive`. Madurez: no identificable en estructura de manera adrede. Riesgo clave: no se les aplican parches y son olvidados al corto plano llevando confusión al usuario sin notificar y dejando la incógnita si las usarán luego. ([GitHub][1])

## 4. Inventario de subsistemas

### T81 Tipos de datos

Propósito: estructura principal sobre sus operaciones ternarias de núcleo y datos canon. Ubicaciones: `core/types/`, `spec/t81-data-types.md`, listados de tests. Estado: verificado de manera interna a estado frozen(congelado). Evidencia de verificación: `v1_canonical_numeric_contract_test.cpp`, `tisc_binary_io_determinism_test.cpp`. Su relación frente a su modelo a la de un determinismo se lo considera alto con las anotaciones hechas al respecto en la fase y nivel de estado; también se las toman de una calidad grande frente al nivel de su entorno en el desarrollo por los aportes a un modo de estado gobernado como factor base que permite de manera fiel la manipulación constante de las revisiones o cambios de políticas que en ella se operan y revisan. Cuidado con que esto también afecta fuertemente y de manera atada al tema del Flote que ha sido descrito. ([GitHub][4])

### TISC ISA

Propósito: modelo operativo base inalterable que dicta el marco del ensamblaje y ejecuciones que permiten la vida al conjunto que interactuará luego en capas superiores sobre una fase estática y cerrada, y bajo la mirada de control que supervisará sus estados y normativas. Ubicaciones: `core/isa/`, especificación en la máquina en modelo e información en determinismo `spec/tisc-spec.md`. Estado: su funcionamiento e integridad está activa en estos instantes de su ciclo pero sin ir al mismo nivel para todos los documentos informativos acerca de en qué fase va. Se evidencia de ésto en los esquemas dados, ya que, se muestran como algo terminado/congelado; el documento de cabecera lo marca con base a un `1.2`, a pesar que la especificación que lo dicta la anexa como "Version 1.1 — Stable" mientras las partes del workflow refieren con “Verify TISC v1.1.0 Freeze Integrity.” Todo recae firmemente sobre sus hombros con respecto a qué se aplica un "Determinismo", incluyendo también sobre este modelo base en gobernanza y que toma el aspecto que esta misma dicte de la mano con las normativas e informes. La preocupación más relevante la ocupa que su congelamiento esté muy resguardado bajo esta superficie pero de una forma ineficaz sin reportar un estado sólido en la presentación del seguimiento en versiones y documentos hacia el público. ([GitHub][4])

### T81VM Interpreter

Propósito: El marco ejecutor en su uso de tipo Interprete (No-JIT) sobre sus rutinas deterministas que es acoplado y funciona de manera conjunta sobre el entorno cerrado en su arquitectura ISA y con su modelo de Tipos de Datos. Ubicaciones: `core/vm/`, las normativas e información en sí están detalladas en la especificación propia de `spec/t81vm-spec.md`. Estado: el componente es operativo desde un entorno que le acoge ya y es base fundamental de los demás sistemas por su puesto, su visualizador principal refiere que va sobre la etiqueta `Stable`, mientras su normativa principal expone algo por el nivel Beta aún. Evidencia de verificación: la comprobación de trazados, la inclusión en los perfiles Determinísticos Core, y lo referente a estar dictados bajo pruebas y la conformidad misma desde un nivel externo y pruebas hechas por el trabajo de su CI. Posee el peso total y un grado de implicación del sistema muy alto, y su estado va conectado y gobernado a un seguimiento total desde un marco controlador al Axion (de allí su relación al control) sobre sí y con el núcleo, este marco aún denota que "no ha sido totalmente revelada/abierta", la trazabilidad aún le falta ser categorizada como primera orden y clase frente a registros. Los trabajos en este marco bajo la perspectiva y arquitecturas cruzadas en sus funciones referidas con Flotes tienen una marca/limitante al marco especificado de usos explícitos en este lugar. ([GitHub][8])

### T81Lang

Propósito: un desarrollo del alto nivel hacia compilar bajo una superficie de TISC que interactuará de modo determinista al 100%. Ubicaciones: `lang/`, su documento anexo en su versión correspondiente. Estado: de los más sobresalientes del repositorio en parte a estar altamente especificado por un apartado con la guía misma de los métodos, las funciones aplicadas del propio marco que utiliza el compilador y los avisos / limitantes para cuando un determinismo deje de poder aplicar por métodos dependientes a ser llevadas fuera de marco. Evidencia de verificación: herramientas que comprueban métodos deterministas ya en código y funcionamiento de rutinas en su compilador con el seguimiento completo bajo la normativa y sintaxis dictada. Esta posee una alta correlación e importancia frente al rol "Determinismo", aunque es necesario poner ojo frente a la dependencia propia desde partes de la arquitectura donde las bases en la propia compilación varíen por la propia matemática del sistema en flotantes/trascendentales y divisiones. Se observa de cerca, controla niveles propios/puros frente a Axion, y advierte: la seguridad / solidez y veracidad expuesta supera una fase real de implementación visual/verificable que pueda ser llevada por personas desde fuera con las partes o archivos actuales hoy presentes en sí. ([GitHub][5])

### Axion Governance Kernel

Propósito: ser el motor de manejo en control para los reportes de actividad visual en ejecuciones sobre los cambios operados de manera nativa / de lado del núcleo mismo a manera controladora. Ubicaciones: `kernel/axion/`, `spec/axion-kernel.md`. Estado: es más visual, explicativo a la aspiración propia de la idea que poseía antes que de la parte terminada en general. Se ha ascendido el motor y se nombra estar Stable por sus pruebas de pase de un valor al 100, mas sigue detallado a su modelo Alpha como especifican varios recursos informativos de los cuales hacen hincapié explícito de su estado parcial sobre las métricas en su complejidad en relación con ser la máxima y su forma de seguimiento hacia lo determinista. Esto se lo vincula estrechamente e inherentemente al pilar principal de ser en su totalidad hacia "Gobernanza" que expuso como concepto central, con sus limitantes, donde las descripciones para nombrar acciones a nivel profundo exceden la forma propia en la que un detector se aplica aquí y ahora mismo al uso frente al proyecto o repositorio actual. ([GitHub][4])

### CanonFS

Propósito: base o lugar a retener la procedencia / inmutabilidad que recae a estados de modelo / objetos. Ubicaciones: `src/canonfs/`, `include/t81/canonfs/` e información extra. Estado: es categorizado de estado Stable - Limitado como nos señala toda su ruta e informe, y DCP, pero con aspiraciones o detalles en arranques a modelos huéspedes en desarrollo para su ruta de trabajo experimental al nivel de su entorno y sistemas que lo rigen dentro en SO. Se hace constar que se trabaja este tema y existen las pruebas de su trabajo, el punto de conflicto principal radica al mezclar temas del núcleo estable que se relacionan en sí al desarrollo propio junto al OS experimental de fases donde la información brindada llega al mismo plano con ambos sistemas de la misma paridad. ([GitHub][2])

### Deterministic Core Profile / Registry

Propósito: acotar todo entorno que recaiga como verificado, controlado, validado o dentro del alcance al 100% de la rama. Ubicaciones: `docs/product/DETERMINISTIC_CORE_PROFILE.md`, `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. Estado: el centro fundamental para el seguimiento real del determinismo (ya no tan solo dicho al vuelo), no con aspiraciones a usar a futuro sino una parte del trabajo que verifica en sus rutinas / pruebas que va ligado en conjunto y por su rama de trabajo de la mano, pero la falta y cruce / mal entendimiento en este nivel a niveles que sobrepasan o mezclan esto de la mano de informativos y resúmenes como el principal a manera general de la raíz de la ruta. ([GitHub][9])

### CI / Governance Scripts

Propósito: una aplicación general del forzamiento y los bloqueos hacia donde los archivos, código, o la base estructural que lo acopla rompen al determinismo al salir y al entrar de los sistemas que operan o son dictados en normativa para proteger sus versiones. Ubicaciones: `.github/workflows/ci.yml`, `scripts/ci/`, `scripts/governance/`. Estado: se reportan trabajos de fuerza poco esperables u observados de este ambiente, donde sí es posible apreciar rutinas de fuerza a bases determinísticas en vez de simple pruebas o verificaciones. Aún no se cuenta del todo si solo es algo netamente superficial o puramente informativo que el CI verifica sin accionar (o frenar el flujo en la cadena) para esto último es muy difícil verificar dado que habría que indagar qué forma compone el propio entorno en donde todo fue hecho (fuera de éste documento) para afirmar con veracidad esto. ([GitHub][3])

### Benchmarks

Propósito: comprobaciones al lado y nivel de rendimiento a flujos sobre la maquinaria. Ubicaciones: `benchmarks/`. Estado: funcionales como prueba de concepto para comprobar las fases o niveles requeridos a operar, aún les hace falta sincronizar del todo o ser llevados hacia la fase del control absoluto del ciclo sobre las notas y la forma central a las que el entorno los aplica, además que de igual forma su "poder" descriptivo supera toda la carga y evidencia al momento al ver que faltan rutinas (por ejemplo para el chequeo de "TISC_vs_Binary"). ([GitHub][1])

### Axion OS / TernaryOS

Propósito: kernel que hace uso completo sobre las capas en los IPC, un gestor, un marco para un host y el sistema en sí de interrupciones para servicios, un marco y superficie para dispositivos con sus correspondientes líneas en base a boots. Ubicaciones: `experimental/ternaryos/`, `kernel/`. Estado: el prototipo ya tiene sus fundamentos operables al usar bases externas y las partes hechas ya brindan un apoyo o una demostración verídica acerca de un progreso más allá al ser algo en solo texto (véase, fases en rutinas). El nombre o término Axion en base al controlador / kernel en sistema crea más fricciones de confusión y la ambición sobre "algo completo y robusto para operar" es mucho o muy anticipada para que alguien que opere la misma se sienta con poder o comodidad en esta ruta y es todavía para desarrolladores o constructores en el núcleo más base y primario (ejem. en qemu/VirtualBox). ([GitHub][7])

### TUI / CLI / Tooling

Propósito: frontend o ambiente / interfaces orientadas para el desarrollo humano de base terminal, comandos del operador en uso, las muestras del funcionamiento. Ubicaciones: `tools/`, `tooling/`, `examples/`. Estado: se puede trabajar, están desarrolladas en un plano suficiente y el uso también tiene lo necesario. Riesgo: Las bases que soportan este progreso tal vez vayan en menor medida a pasos y desarrollos más distantes que las que sí tendrían que ir y de dónde estos programas toman información base. ([GitHub][10])

### Experimental / Distributed / Cognitive Tiers / AI

Propósito: un espacio para escalar proyectos y cosas alejadas en sí del modelo DCP / determinista y nuclear, y más hacia algo en uso o exploratorio con AIs. Ubicaciones: `experimental/`, `experiments/ ai`, los apartados "distributed y tier references". Estado: esto de acá tiene fases exploratorias (por su propio creador, se menciona o etiqueta este espacio en base experimental - "Fuera de un marco verificado y base o certificado de tipo determinista-puro DCP). Estas áreas son o representan grandes aportes en funcionalidades hacia el concepto pero el código aún avanza lento frente a la visión de esta idea expuesta a simple vista. ([GitHub][1])

## 5. Evaluación de la Coherencia de la Arquitectura

Existe un modelo por capas real. El resumen de arquitectura presenta una cadena coherente desde T81Lang hasta TISC, T81VM, Axion y CanonFS, con las capas experimentales mostradas explícitamente como opcionales/no-DCP. El DCP y el registro de determinismo refuerzan esto al limitar las garantías a superficies verificadas nombradas. Esa es una fuerte señal de autoconciencia arquitectónica. ([GitHub][2])

Sin embargo, los límites del subsistema son solo parcialmente reales. El límite de ejecución principal parece razonablemente real: los tipos de datos, la ISA, el intérprete y algunas superficies de serialización/prueba deterministas seleccionadas. Fuera de ese núcleo, los límites se difuminan. "Axion" se refiere tanto a un kernel de gobernanza en la pila estable como a un sistema operativo experimental. La especificación de T81VM está en fase Beta, mientras que los paneles de control la llaman Stable. La especificación de Axion es Alpha, mientras que la matriz de implementación llama al Kernel de Axion Stable. El documento de progreso del sistema operativo todavía utiliza el nombre interno `ternaryos`. Esto no es un problema menor de redacción; significa que la arquitectura tiene múltiples autodescripciones superpuestas. ([GitHub][6])

Las interfaces son parcialmente explícitas y parcialmente implícitas. Las especificaciones hacen un buen trabajo al nombrar comportamientos requeridos, invariantes y programas de conformidad. La CI también nombra comprobaciones de alineación. Pero varias interfaces críticas todavía se rastrean como incompletas o indirectas: el modo de ejecución de la máquina virtual no está expuesto a través de una API pública limpia; los eventos del planificador no son eventos de rastreo de primera clase; la detección activa del no determinismo de Axion es solo parcial. Esto significa que algunos contratos están mejor especificados que implementados. ([GitHub][8])

Las uniones más fuertes son el límite del DCP y el modelo de autoridad interno del repositorio. Las uniones más débiles son la sincronización de nombres/versiones/estados y la expansión desde un núcleo determinista riguroso hacia narrativas de sistema operativo, cognición, inteligencia artificial y hardware. La dispersión de la documentación es visible desde el árbol del repositorio y la densa ecología del panel de control. ([GitHub][1])

## 6. Evaluación del Determinismo y la Reproducibilidad

El determinismo es el tema de ingeniería más serio del repositorio. Se afirma en el archivo README, se formaliza en las especificaciones de TISC y T81VM, se delimita en la descripción general de la arquitectura, se reduce en el DCP y se enumera en el Registro de Superficie de Determinismo. El registro es especialmente útil porque distingue las superficies verificadas de las excluidas o planificadas. ([GitHub][1])

Dónde se aplica el determinismo de manera genuina: la semántica del código de operación TISC, la ejecución del intérprete de VM, la codificación de tipo de datos canónicos y las matemáticas deterministas de flotante suave se enumeran como verificadas con pruebas con nombre y aplicación de CI. Luego, el DCP utiliza ese registro como puerta para las garantías de lanzamiento. La CI también ejecuta comprobaciones de integridad de congelación, comprobaciones de coherencia de arquitectura y aplicación de afirmaciones de determinismo. ([GitHub][11])

Donde el determinismo es solo parcial o está limitado: la emisión de código de bytes del compilador es explícitamente solo "Parcial" en el registro; la equivalencia JIT está planificada, no verificada; la E/S de red, la programación en tiempo real, el comportamiento de la FPU del hardware fuera del formato flotante suave y el determinismo del rendimiento quedan explícitamente fuera del alcance. T81Lang también establece que la división de formato flotante y el comportamiento trascendental pueden variar entre las arquitecturas. Esa es una buena honestidad de ingeniería. ([GitHub][11])

El desajuste principal no es la ambición técnica versus la implementación nula; es la ambición técnica versus la disciplina de estado. El repositorio tiene un límite de determinismo bien diseñado, pero sus superficies de versión/estado de cara al público son lo suficientemente ruidosas como para debilitar la confianza en la exactitud de la versión. Cuando un proyecto se basa en la reproducibilidad exacta en bits, la exactitud de la versión también importa. ([GitHub][9])

## 7. Evaluación de Gobernanza, Seguridad y Capa de Políticas

La gobernanza aquí es arquitectónica, no puramente retórica. Axion está presente en las especificaciones de ISA y VM, en la descripción general de la arquitectura, en el límite de DCP y en la matriz de implementación. La CI del repositorio también contiene múltiples comprobaciones de gobernanza y exageración de afirmaciones, lo que es más sólido que una historia de gobernanza convencional basada únicamente en un archivo README. ([GitHub][12])

Dicho esto, la especificación en sí muestra que la capa de gobernanza no está completamente terminada. La "administración del determinismo" de Axion es explícitamente parcial; la detección activa del no determinismo aún no está implementada. La medición de la complejidad es parcial. La especificación también señala que la concatenación de la cadena de razón canónica fue una brecha rastreada. Por lo tanto, la pila de gobernanza es real, pero aún no lo suficientemente completa como para justificar el marco más amplio de "inteligencia de supervisión" del repositorio sin reservas. ([GitHub][6])

El mecanismo de gobernanza más convincente no es el lenguaje ético; es la intercepción acotada de políticas de operaciones privilegiadas más los requisitos explícitos de trazabilidad. La parte menos convincente es la extensión del lenguaje de gobernanza al nivel cognitivo y la supervisión del razonamiento avanzado mientras que los mecanismos centrales de detección y medición aún están incompletos. ([GitHub][6])

## 8. Verificación de Realidad de Implementación

Lo que parece funcionar verdaderamente ahora: el perfil del núcleo determinista alrededor de los tipos de datos, TISC, la ruta del intérprete de T81VM, los accesorios de reproducibilidad del compilador seleccionados, un sustancial conjunto de CI/gobernanza y suficiente integración con Axion para sustentar afirmaciones de ejecución compatibles con las políticas. El repositorio tiene pruebas concretas, flujos de trabajo con nombre y designaciones explícitas de nivel estable/congelado alrededor de ese núcleo. ([GitHub][9])

Lo que parece funcionar parcialmente: partes del modelo de administración más rico de Axion, algunas superficies de compilación y reproducibilidad, superficies de servicios/consultas en el esfuerzo del SO, y la disciplina de versiones/referencias como una superficie de control completamente sincronizada. Estos no están ausentes; simplemente no son tan completos como sugiere el lenguaje más ambicioso del repositorio. ([GitHub][6])

Lo que parece estar andamiado pero aún no es operativo en el sentido más fuerte: equivalencia trace-JIT, determinismo distribuido/nivel cognitivo, aceleradores de hardware externos, adaptadores de hardware reales en la ruta del SO y ambiciones más amplias de una "plataforma de computación determinista global". El propio repositorio clasifica muchas de estas como experimentales, planificadas o fuera del DCP. ([GitHub][9])

Áreas más maduras: tipos de datos del núcleo determinista, ISA, intérprete, andamiaje de CI/gobernanza y, a juzgar por la madurez de la documentación, la superficie del lenguaje. Áreas más ambiciosas: inteligencia de gobernanza completa, niveles distribuidos/cognitivos, realización de hardware nativo y la conversión del SO en un sustrato de kernel de propósito general. ([GitHub][4])

## 9. Evaluación de Pruebas, Verificación e Integración Continua (CI)

La postura de prueba/CI es un punto fuerte importante. El flujo de trabajo de CI inspeccionado incluye la verificación de la estructura, comprobaciones de destino de arquitectura, comprobaciones de integridad de congelamiento TISC, comprobaciones de límites de experimentos de IA, comprobaciones de coherencia de arquitectura, comprobaciones de fijación de flujos de trabajo y auditorías de permisos, un amplio conjunto de comprobaciones de gobernanza, comprobaciones de integridad DCP, métricas de gobernanza y comprobaciones de cargas de trabajo de referencia. Esto es sustancialmente más riguroso que un repositorio típico de sistemas creados por pasatiempo. ([GitHub][3])

El repositorio también vincula las afirmaciones de determinismo a pruebas y scripts nombrados de una manera que facilita las auditorías. El registro de determinismo y el DCP enumeran rutas explícitas en lugar de garantías vagas. Ese es el patrón correcto. ([GitHub][11])

Aún quedan puntos ciegos. Algunos trabajos de CI son informativos, no puertas estrictas. La emisión del código de bytes del compilador es solo parcial en el registro de determinismo. La propia especificación de VM admite la falta de superficies de consulta directas y una visibilidad incompleta de los rastros de programación. Los recuentos de referencias/informes son inconsistentes en todos los documentos autoritativos. ([GitHub][3])

## 10. Evaluación de la Integridad de la Documentación y las Especificaciones

La cantidad de documentación es muy alta y la intención de control de ingeniería es seria. La descripción general de la arquitectura establece explícitamente el orden de autoridad. La matriz de implementación intenta reducir la deriva narrativa a una fila por subsistema. El centro de control del proyecto centraliza el estado de las puertas. Ese es un buen diseño de gobernanza. ([GitHub][2])

El problema es la integridad de la sincronización. Se pueden observar al menos cinco inconsistencias importantes en los materiales inspeccionados: la versión/los recuentos de pruebas de README versus los de Project Control Center versus la versión de CMake; TISC v1.2 versus la especificación TISC v1.1; la especificación Beta de T81VM versus el panel de control Stable; la especificación Alpha de Axion versus la matriz Stable; y los mensajes alfa/prototipo de Axion OS versus algunos mensajes del kernel estable más amplios en otros lugares. Estos no son superficiales. Crean incertidumbre sobre qué debería ser tratado como autoritativo por un nuevo responsable de mantenimiento, socio o evaluador. ([GitHub][1])

Para un nuevo colaborador técnico, el repositorio es impresionante pero difícil de analizar. La presencia de `docs`, `book`, `spec`, `contracts`, `internal`, `legacy`, `pdf`, `notebooks`, múltiples paneles de control y superficies públicas multilingües deja en claro que se trata de un entorno con mucha documentación. Sin una sincronización sólida, eso se convierte en un laberinto. ([GitHub][1])

## 11. Evaluación del Kernel / Sistema Operativo (SO) / Axion

El trabajo en el sistema operativo es considerablemente más profundo que una nota de concepto. El registro de progreso describe las fases implementadas para MMU, la programación/comunicación entre procesos (IPC), la persistencia, las estructuras de los dispositivos, la interfaz binaria de la aplicación (ABI) del paginador, los fragmentos de gobernanza de interrupciones y la validación de la vía de arranque. Cita miles de afirmaciones y una hoja de ruta por fases con archivos y pruebas concretas. Esto es ingeniería real. ([GitHub][7])

Pero todavía es mejor clasificarlo como un prototipo alojado avanzado, no un kernel completo. El registro de progreso describe repetidamente las rutas de simulación alojadas, las rutas de desarrollo QEMU, las estructuras de VirtualBox, los paquetes de transferencia de artefactos recuperados y el trabajo abierto del adaptador de hardware real. La arquitectura avanza a través de fragmentos por etapas y la validación respaldada por la simulación, lo cual es respetable, pero no es lo mismo que un sistema operativo autónomo maduro. ([GitHub][7])

La división de nombres también importa. "Axion" se refiere al sistema operativo en el registro de progreso, mientras que la especificación define "Axion Kernel" como la capa de inteligencia de supervisión del ecosistema. Estos conceptos están relacionados, pero no son idénticos. Ese conflicto de nombres seguirá causando confusión hasta que el repositorio trace una línea semántica más estricta. ([GitHub][7])

Veredicto: actualmente, este es un prototipo arquitectónico avanzado / sustrato de sistema operativo parcial con subsistemas reales, que aún no es un sistema operativo de uso general o de producción. ([GitHub][7])

## 12. Evaluación de Pila de Lenguaje / VM / ISA

TISC es el ancla conceptual más estable. Es normativo, está orientado a la congelación y define explícitamente la ejecución determinista, el comportamiento indefinido nulo, la visibilidad de Axion y la compatibilidad con tipos de datos, máquinas virtuales y capas de lenguaje. ([GitHub][12])

T81VM es el corazón operativo. La ruta del intérprete es claramente fundamental para el núcleo determinista y el DCP excluye el compilador en tiempo de ejecución (JIT) hasta que se pruebe su equivalencia. La especificación también muestra una honestidad saludable al nombrar las superficies actualmente incompletas en lugar de pretender que están listas. ([GitHub][8])

T81Lang está más maduro que muchos lengrada de investigación porque tiene una gramática normativa, un marco de tipo/efecto, notas de determinismo explícitas y una canalización de compilador. Sin embargo, también es donde es más fácil superar la ambición del proyecto. El estado del lenguaje "estable" es plausible para una cadena de herramientas compilador a TISC limitada, pero no necesariamente para la semántica del ecosistema completo implicada en torno a los niveles, agentes y superficies externas controladas sin un marco de lanzamiento más restringido. ([GitHub][5])

Como sustrato de cálculo estable, la combinación de tipos de datos + TISC + T81VM sin JIT es el activo técnico más confiable del repositorio. Esa es la parte que de manera plausible podría considerarse como una plataforma experimental seria. ([GitHub][9])

## 13. Evaluación de Investigación vs. Comercialización

En la actualidad, el repositorio se sitúa entre "plataforma experimental" y "base técnica previa al producto". Va más allá de un mero artefacto de investigación porque tiene sistemas de compilación, integración continua (CI), pruebas, documentos sobre la disciplina de los lanzamientos, referencias de conformidad, paneles de control y un núcleo determinista delimitado. Sin embargo, todavía no es una plataforma para desarrolladores ni un candidato a infraestructura en el sentido estricto, porque hay demasiadas capas externas que aún son experimentales, contradictorias o tienen una carga administrativa desproporcionada en comparación con su madurez de implementación. ([GitHub][3])

Para avanzar a la siguiente etapa, el repositorio necesitaría un cambio decisivo: separar el núcleo determinista certificable del ecosistema exploratorio con un etiquetado de lanzamiento mucho más estricto. Esto significa una fuente de versión oficial, una fuente de estado, un registro de superficie promocionado y marcas explícitas de "no es parte de los límites del producto" en cualquier otra parte. Después de eso, necesitaría una reproducción independiente fuera del contexto del mantenedor principal y pruebas más estrictas de las interfaces del lenguaje, la cadena de herramientas y la gobernanza que ya afirma que son estables. ([GitHub][9])

## 14. Análisis FODA

**Fortalezas**
El repositorio tiene una arquitectura de sistemas genuina, no es solo imagen de marca. El límite del determinismo es inusualmente explícito. La disciplina de CI/Gobernanza es avanzada. El núcleo de ejecución centrado en el intérprete parece ser materialmente real. El proyecto también cuenta con sólidos hábitos de documentación e intención de abarcar varias capas. ([GitHub][2])

**Debilidades**
La incoerencia del estado/versión es grave. La documentación es muy extensa. A veces, el lenguaje de gobernanza va más allá de la implementación. Las superficies del SO y de inteligencia artificial/gobernanza avanzada aumentan el alcance más rápido de lo que aumentan la certeza auditable. Los conflictos de nombres, especialmente los que rodean a Axion, desdibujan la arquitectura. ([GitHub][13])

**Oportunidades**
Un lanzamiento del núcleo determinista claramente delimitado podría convertirse en la punta de lanza creíble del proyecto para adentrarse en la computación de investigación, la ejecución reproducible o la experimentación con lenguajes/VM. El repositorio ya cuenta con el andamiaje de gobernanza para respaldar esto, siempre y cuando deje de intentar que cada anillo exterior parezca igual de maduro. ([GitHub][9])

**Amenazas**
La mayor amenaza es la pérdida de credibilidad a través del desvío debido a la formulación de afirmaciones desmesuradas. En un proyecto creado en torno al determinismo y la auditabilidad, tener superficies de estado contradictorias resulta especialmente perjudicial. La segunda amenaza es el colapso de la capacidad de mantenimiento debido a la amplitud del alcance: núcleo, lenguaje, máquina virtual, almacenamiento, inteligencia artificial, hardware, documentos, libro, paneles de control y la divulgación multilingüe, todo bajo un único gráfico de autoridad en evolución. ([GitHub][1])

## 15. Registro de Riesgos

| Riesgo | Sistemas afectados | Gravedad | Evidencia | Mitigación |
| ---------------------------- | ------------------------------------------------------------------------------ | -------: | ----------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- |
| Desviación del estado/versión | Todo el repositorio | Crítica | README `v1.9.0` / 369 pruebas frente a Centro de control `v1.4.1` / 363 pruebas frente a CMake `1.3.6` | Crear una única fuente fidedigna para la versión, el estado del lanzamiento y los totales de pruebas; generar documentos descendentes a partir de ella. ([GitHub][1]) |
| Desajuste entre las especificaciones y la implementación | ISA, VM, Axion | Crítica | Especificación de TISC 1.1 frente a README TISC 1.2; especificación beta de la VM frente a Panel de control Estable; especificación Alpha de Axion frente a Matriz Estable | Agregar comprobaciones de sincronización automática de estado para la versión/el estado de la especificación frente a los paneles de control frente a README. ([GitHub][12]) |
| Riesgo de teatro de la gobernanza | Axion, niveles cognitivos | Alta | La especificación de Axion indica que la administración del determinismo es parcial; las métricas de complejidad son parciales | Limitar las afirmaciones públicas únicamente a los ganchos implementados y los puntos de control verificados. ([GitHub][6]) |
| Extensión excesiva | Experimental, Inteligencia Artificial (IA), Sistema Operativo (SO), hardware | Alta | El perfil de núcleo determinista (DCP) excluye muchas superficies exteriores, mientras que README y la hoja de ruta abarcan hardware, nube determinista, cognición | Congelar la comercialización de superficies exteriores; publicar un mapa de “núcleo versus investigación” en los documentos de la raíz. ([GitHub][9]) |
| Confusión de terminología/nombres | Axion, TernaryOS, kernel | Alta | El registro de progreso dice sistema operativo (SO) Axion con el interno `ternaryos`; la especificación usa Axion Kernel como capa de supervisión | Cambiar de nombre o colocar prefijos en el kernel de gobernanza frente al kernel del sistema operativo sistemáticamente. ([GitHub][7]) |
| Expansión experimental | `experimental`, `experiments/ ai`, `legacy`, `internal`, cuadernos/pdf/archivo | Alta | Amplia superficie del repositorio con muchos directorios de soporte/estado no claro | Publicar una taxonomía de estado de soporte por directorio de nivel superior: núcleo mantenido, soporte mantenido, experimental, legado, archivado, solo interno. ([GitHub][1]) |
| Puntos ciegos de las pruebas | Programación de máquinas virtuales, reproducción de compiladores, JIT | Media | Los eventos de seguimiento de programación no son de primera clase; la emisión de compiladores es parcial; el JIT se excluye del DCP | Elevar las superficies de seguimiento faltantes a pruebas de primera clase antes de continuar con la expansión de funciones. ([GitHub][8]) |
| Riesgo de mantenimiento | Documentos, código y paneles de control | Media | Reduxado marco de documentación con múltiples paneles de control de estado y capas de autoridad | Generar matrices/paneles de control a partir de metadatos legibles por máquinas. ([GitHub][2]) |
| Riesgo de integración | Colaboradores nuevos | Media | Amplitud del árbol de raíz más capas competitivas de documentos/especificaciones/libros/centros de control | Añadir mapa de rutas para el colaborador: “dónde confiar primero, en dónde no confiar todavía”. ([GitHub][1]) |
| Riesgo de credibilidad | Entidades colaboradoras/financiadoras externas | Crítica | La marca determinista/auditable se ve socavada por la incoherencia interna | Tratar los defectos de sincronización como defectos que bloquean los lanzamientos. ([GitHub][1]) |

## 16. Cuadro de Mandos de Madurez

Las puntuaciones son mi síntesis de las especificaciones inspeccionadas, los paneles de control, el CI, el DCP/registro y los documentos de progreso del sistema operativo. Son juicios de valor, no números que proporciona el repositorio. ([GitHub][2])

| Dominio | Claridad conceptual | Profundidad de la implementación | Evidencia en pruebas | Estabilidad de las interfaces | Claridad de gobernanza | Preparación operativa | Integridad de la documentación |
| ---------------------------- | -----------------: | -------------------: | ------------: | ------------------: | -----------------: | --------------------: | ----------------------: |
| Tipos de datos | 5 | 4 | 4 | 5 | 4 | 4 | 4 |
| TISC ISA | 5 | 4 | 4 | 5 | 4 | 4 | 3 |
| T81VM | 4 | 4 | 4 | 3 | 4 | 4 | 3 |
| T81Lang | 4 | 3 | 3 | 3 | 4 | 3 | 4 |
| Gobernanza de Axion | 4 | 3 | 3 | 3 | 4 | 3 | 3 |
| CanonFS | 4 | 3 | 3 | 3 | 3 | 3 | 3 |
| CI / herramientas de gobernanza | 4 | 4 | 4 | 4 | 5 | 4 | 4 |
| SO Axion / TernaryOS | 4 | 3 | 4 | 2 | 3 | 2 | 3 |
| Inteligencia Artificial / cognitiva / distribuida | 3 | 2 | 2 | 2 | 3 | 1 | 3 |
| Documentos / comunicación pública | 4 | 4 | 3 | 2 | 4 | 3 | 2 |

Perfil de madurez general ponderado: **3.4 / 5**. Esto se corresponde con una plataforma experimental seria con un núcleo determinista confiable, pero con suficiente ambigüedad en la capa exterior y en la gobernanza y documentación para evitar que se le otorgue una calificación más sólida de “lista para infraestructura”. ([GitHub][9])

## 17. Recomendaciones Estratégicas

### Prioridades Inmediatas (0–30 días)

Unificar la autoridad de versión/estado. Elija una fuente canónica para el número de lanzamiento, madurez de las especificaciones, madurez del subsistema y totales de las pruebas; genere el archivo README, el Centro de control, la Matriz de implementación y los metadatos de compilación a partir de ella. Hasta que eso no se haga, cada afirmación de "estable" debería tratarse como provisional. ([GitHub][1])

Renombrar los dos conceptos de Axion. Uno es un kernel de gobernanza/motor de políticas; el otro es un sistema operativo experimental. Necesitan diferentes nombres orientados al usuario o prefijos estrictos. ([GitHub][6])

Publicar un índice de estado de soporte del repositorio según su directorio de nivel superior: núcleo mantenido, soporte mantenido, experimental, legado, archivado, solo interno. El árbol es demasiado grande para dejarlo implícito. ([GitHub][1])

### Prioridades a corto plazo (1 a 3 meses)

Ajuste las normas de la etapa de implementación a especificación. No se debería calificar a un subsistema como Estable en los paneles de control si su especificación normativa sigue en fase Beta/Alpha, a menos que en el repositorio se distinga de forma explícita entre “implementación estable y especificación pendiente”. Hoy en día no se hace de forma ordenada. ([GitHub][8])

Termine las superficies de observabilidad que faltan en el límite de la VM/Axion: la consulta directa del modo de ejecución, los eventos de trazabilidad de la programación como componentes de primera clase y cualquier tarea remanente de las cadenas causales canónicas y el trabajo de detección del no determinismo. Son puntos de referencia porque convierten la gobernanza de filosofía en instrumentación. ([GitHub][8])

Separar las notas de la versión del DCP de las del ecosistema. El núcleo se merece una disciplina de publicación ágil y certificable. El resto del ecosistema requiere un registro con los avances de la investigación. Al combinar ambos, la eficacia de ambos disminuye. ([GitHub][9])

### Prioridades a medio plazo (de 3 a 12 meses)

Procurar una reproducción independiente y externa del núcleo determinista, porque en el propio repositorio se cita esto como el criterio de progreso restante de la verificación estilo Fase 2. Eso ayudaría más a la credibilidad que un nuevo nivel de paneles. ([GitHub][1])

En el caso del sistema operativo (SO), hay que decantarse por la próxima prueba de veracidad: o bien un “prototipo con alojamiento con simulación rigurosa del dispositivo o arranque”, o bien un “sustrato del núcleo real en un entorno específico”. Hoy en día el avance es responsable, pero sigue dándose en demasiados escenarios. Si se limitara el alcance de aceptación, el programa sería más preciso. ([GitHub][7])

En cuanto al perímetro de la IA o gobernanza, deje de publicitar enfoques hardware/distribuidos/cognitivos como componentes próximos del núcleo. Manténgalos en medios de investigación ajenos de forma explícita al DCP, hasta conseguir un nivel de validación análogo a la estructura (stack) de la capa modelo de datos/ISA/VM. ([GitHub][9])

## 18. Veredicto Final

En la actualidad, T81 Foundation es fundamentalmente **una plataforma de cálculo determinista experimental y rigurosa con un núcleo VM/ISA/modelo de datos auténtico y una superestructura de documentación/gobernanza extraordinariamente elaborada**. ([GitHub][2])

Su mayor recurso técnico lo representa **el perfil de base determinista circunscrito, así como el hecho de que el repositorio trate de forma fáctica de vincular las pretensiones a los ensayos citados, comprobaciones CI y a límites de actuación concretos**. ([GitHub][9])

La debilidad estructural de mayor relevancia es **la discordancia en la situación general existente entre especificaciones, paneles informativos, el documento LÉAME (README) y el registro metadato de compilaciones**. En un entorno común es algo incómodo, pero tratándose de un repositorio con la reproducibilidad auditable de fondo, el efecto resulta pernicioso. ([GitHub][1])

La medida aislada que mejor podría corregir su rumbo sería la de **convertir la sincronización de por sí en un entorno determinista de primera clase**: una jerarquía con autoridad, un registro de progresión en redacciones, una base probada definitiva de lanzamiento y la rígida segregación entre la estructura esencial apta para la cualificación y lo fronterizo o de exploración.

[1]: https://github.com/t81dev/t81-foundation/ "GitHub - t81dev/t81-foundation: T81 es una arquitectura computacional unificada, determinista y de base ternaria concebida para rebasar los limitantes del procesamiento binario. · GitHub"
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

## Licencia

MIT License.
