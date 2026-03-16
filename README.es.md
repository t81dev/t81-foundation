<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# Fundación T81: pila de computación ternaria determinista

![Release](https://img.shields.io/badge/release-v1.6.0--Stable-blue)
![Tests](https://img.shields.io/badge/tests-367%2F367_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.2.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

Aprovechando la eficiencia teórica de la computación base-e, **T81 Foundation** es una pila informática determinista construida sobre **aritmética ternaria equilibrada** ({-1, 0, +1}) con un modelo de gobernanza de cadena completa que cubre el conjunto de instrucciones, la máquina virtual, el compilador de lenguajes y el entorno de inferencia de IA.

La pila ofrece:

- **reproducibilidad exacta en bits**: cada ruta de ejecución produce un hash de seguimiento idéntico en todas las plataformas compatibles
- **inferencia de IA gobernada**: el motor de políticas de Axion intercepta y audita cada operación privilegiada antes de que se produzcan efectos secundarios.
- **procedencia del contenido**: CanonFS registra todos los artefactos, pesos de modelo y estado de tiempo de ejecución de manera inmutable
- **ejecución paralela determinista**: el modelo de gráfico de tareas DPE (RFC-DPE-0002) permite cargas de trabajo TISC simultáneas con resultados comprometidos por época

---

## Estado del proyecto: marzo de 2026

**Fase: Desarrollo Activo** — v1.6.0-Estable; 368/368 pruebas aprobadas; Determinismo multiplataforma verificado en Linux x86\_64 + macOS ARM64.

| Componente | Madurez | Notas |
| :--- | :--- | :--- |
| **TISC UNO** | ❄️ Congelado | v1.2.0; la semántica del código de operación es inmutable en v1.x; 12 nuevos códigos de operación desde v1.1: `AgentInvoke` (RFC-0015), 6 inferencias nativas ternarias (RFC-0034), 3 FFI (RFC-00B8), 2 criptografía de celosía (RFC-0038), 1 anillo KEM (RFC-0039) |
| **Tipos de datos** | ❄️ Congelado | BigInt, Float, Complex, Map, Set: codificación de bits estable; 2026-02-27 auditoría limpia |
| **T81VM** | ✅ Estable | Despacho completo de TISC v1.2;  `AgentInvoke` + inferencia nativa ternaria + FFI + criptografía de celosía + códigos de operación NTRU-KEM; 368/368 pruebas |
| **T81Idioma** | ✅ Estable | especificación v1.3 Estable;  `agent`/`behavior` (RFC-0015);  `foreign {}` FFI (RFC-0036);  `std.tnn.*` TNN biblioteca estándar (RFC-0037);  `std.crypto.*` criptomoneda reticular + NTRU-KEM (RFC-0038/0039); soporte de identificador contextual en todo |
| **Núcleo de gobernanza de Axion** | ✅ Estable | P4 Seguridad y P5 Instrucción privilegiada satisfechos; Cadenas de razones canónicas AX-M6; cada puerta de activación `AgentInvoke` + `TACT` emite un evento de auditoría |
| **Inferencia ternaria-nativa** | ✅ Aceptado | RFC-0034 + RFC-0037: `TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT`;  `std.tnn.*` T81Lang stdlib (6 funciones integradas → operaciones TISC); inferencia sin multiplicación; formato de peso T81WTN; 13/13 pruebas |
| **Criptografía reticular** | ✅ Aceptado | RFC-0038+0039: `POLYMUL`, `POLYMOD`, `TVecSub`; anillo completo {+,−,×,mod} sobre Z\[x\]/(x^n+1);  `std.crypto.{polyadd,polysub,polymul,polymod,ntru_encrypt,ntru_decrypt}`; 37/37 pruebas |
| **IFF gobernada** | ✅ Aceptado | RFC-00B8 + RFC-0036: `FFIDispatcher`, `FFILibraryRegistry`, 3 códigos de operación VM;  `foreign [policy] { fn … }` T81Gramática lingüística;  `foreign.<name>(args)` → `FFI_CALL` ; 9/9 pruebas de CA |
| **Interfaces TUI** | ✅ Aceptado | `t81 studio` (operador humano) + `t81 agent` (nativo de IA); FTXUI v5.0.0; RFC-0033 aceptado |
| **T81Gráfico** | ✅Beta | Reducción del código de operación de VM + serialización del lado del idioma cableada; Verificación DCP completa; 6/6 pruebas |
| **DPE (ejecución paralela)** | ✅ Aceptado | RFC-DPE-0001–0009 todos aceptados; gráfico de tareas, anillo de historial de época, eventos de auditoría de época, tiempo de espera completamente implementado |
| **Niveles cognitivos** | ✅ Aceptado | Cognición Tier4 (RFC-0021): `Tier4Loop`, `SelfModel` (anillo de 81 entradas), `RecursiveImprovementBounds`, `TierAwarePlanner`; Pasan 4 conjuntos de pruebas |
| **Suite de referencia** | ✅ Aceptado | RFC-00A2: rendimiento de VM + validación de determinismo CanonHash81 (`score=1.0` en todas las ejecuciones);  `t81 internal benchmark` |
| **CI determinista multiplataforma** | ✅ Aceptado | El flujo de trabajo diario de GitHub Actions compara hashes de código de bytes T81Lang en Linux x86\_64 (gcc-14) y macOS ARM64 (clang); registro de evidencia públicamente auditable |
| **Núcleo del sistema operativo Axion** | 🔬 Experimentales | TernaryOS: buscapersonas, programador, IPC, marco de interrupción, carril EFI QEMU x86\_64 operativo; 9/9 pruebas de ternaryOS pasan |

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

**TISC ISA v1.2**: Arquitectura de conjunto de instrucciones ternarias. Congelado bajo v1.x; el contrato de ejecución inmutable para toda la pila. v1.2 agrega 9 códigos de operación: `AgentInvoke` (RFC-0015), seis operaciones de inferencia nativas ternarias (RFC-0034) y tres operaciones FFI gobernadas (RFC-00B8).

**T81VM** — Intérprete TISC determinista. Garantiza una salida de bits idénticos en todas las plataformas; El aislamiento previo al envío de Axion mantiene los ganchos de gobernanza fuera de la ruta de ejecución activa. Envío completo de TISC v1.2 que incluye inferencia nativa ternaria y FFI.

**Axion Governance Kernel**: motor de políticas que intercepta `AXREAD`, `AXSET`, `AXVERIFY`, códigos de operación de IA y llamadas FFI antes de cualquier efecto secundario. Fallo cerrado debido a un error en el análisis de la política. Certificado estable el 15 de marzo de 2026 con 54/54 pruebas aprobadas.

**CanonFS**: sistema de archivos dirigido a contenido. Almacena todos los objetos de código, pesos de modelo y artefactos de tiempo de ejecución como blobs inmutables identificados mediante hash. Proporciona procedencia para auditorías de determinismo.

**T81Lang**: lenguaje de alto nivel dirigido al código de bytes TISC. Tipos nativos: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`. Las declaraciones `agent { behavior }` de primera clase se compilan en `AGENT_INVOKE` con auditoría Axion (RFC-0015).  Los bloques `foreign [policy] { fn … }` declaran funciones externas gobernadas que llaman vía `FFI_CALL` (RFC-0036).  `agent` , `behavior` y `foreign` se pueden utilizar como identificadores contextuales en todas las posiciones de expresión y enlace. Tubería del compilador: lexer → analizador → AST escrito → análisis semántico → generación de IR.

**Inferencia nativa ternaria (RFC-0034)**: seis códigos de operación TISC para inferencia de IA sin multiplicación utilizando pesos ternarios equilibrados {−1, 0, +1}: `TWMATMUL` (matmul), `TQUANT` (cuantizar a trit), `TATTN` (atención ternaria), `TWEMBED` (incrustación de peso), `TERNACCUM` (producto escalar escalar), `TACT` (activación con puerta de techo Axion). Formato de peso T81WTN. Interfaz T81Lang `foreign {}` completa a través de RFC-0036.

**FFI gobernada (RFC-00B8 + RFC-0036)**: interfaz de función externa gobernada por pila completa. Capa de VM (RFC-00B8 Fase 1): `FFIDispatcher` aplica verificaciones de políticas, cuotas de recursos y seguimientos de auditoría antes de cualquier llamada externa;  `FFILibraryRegistry` rastrea las bibliotecas registradas por nombre y hash de versión; tres códigos de operación de VM (`FFICall`, `FFIRegister`, `FFIPolicySet`). Capa de idioma (RFC-0036): `foreign deterministic { fn sin(x: T81Float) -> T81Float; }` declara firmas;  `foreign.sin(angle)` en los sitios de llamadas se reduce a `FFI_CALL` con el nombre de la función en `text_literal`. Pasan nueve pruebas de aceptación.

**TUI Frontends**: dos interfaces de terminal complementarias basadas en FTXUI v5.0.0:

- `t81 studio`: barra lateral de navegación, navegador CanonFS, panel de control Axion, visualizador de seguimiento de determinismo, paleta de comandos (`Ctrl+P`)
- `t81 agent`: sesión JSONL persistente, comandos de barra diagonal (`/compile`, `/run`, `/hash`, `/allow`, `/infer`, `/trits`,…), barra de probabilidad trit

**DPE (ejecución paralela determinista)**: modelo de gráfico de tareas sobre TISC ISA congelado. Las tareas declaran entradas inmutables y regiones de salida almacenadas en búfer; la VM confirma todas las escrituras de forma atómica al final de la época. No se requieren nuevos códigos de operación.

---

## Inicio rápido

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

Banderas de compilación opcionales:

| Bandera | Por defecto | Objetivo |
| :--- | :--- | :--- |
| `T81_BUILD_TUI` | `ON` | Interfaces TUI basadas en FTXUI |
| `T81_BUILD_TESTS` | `ON` | Conjunto de pruebas completo |
| `T81_ENABLE_ASAN` | `OFF` | desinfectante de direcciones |
| `T81_ENABLE_UBSAN` | `OFF` | desinfectante UB |
| `T81_ENABLE_LLAMA_CPP` | `OFF` | Adaptador de inferencia llama.cpp gobernado |
| `T81_WARN_STRICT` | `OFF` | Modo de escaneo de advertencia estricta (utilizado por el preajuste `warn-strict`) |

**Análisis de advertencia previo al envío**: refleja las comprobaciones `-Wswitch`, `-Wunused-variable` y `-Wunused-function` aplicadas por Windows CI, detectando problemas localmente en aproximadamente 2 minutos en lugar de esperar la matriz completa:

```bash
cmake --preset warn-strict
cmake --build build-warn-strict 2>&1 | head -40
```

---

## Verificación del determinismo

Cada versión se verifica para garantizar una reproducibilidad multiplataforma con bits exactos.

```bash
./scripts/ci/run_determinism_slice.sh
```

Plataformas verificadas: **Linux x86_64**, **macOS ARM64**. Cualquier divergencia en los hashes de seguimiento de VM es un defecto crítico.

---

## Documentación

| Tema | Ubicación |
| :--- | :--- |
| Primeros pasos (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| Primeros pasos (IA) | `docs/user-guide/getting-started/ai-quickstart.md` |
| Guías TUI | `docs/user-guide/how-to/tui-guide.md` |
| Especificación ISA | `spec/tisc-spec.md` |
| Manual de políticas de Axión | `docs/user-guide/tutorials/axion-policy-manual.md` |
| Referencia T81Lang Stdlib | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| Descripción general de la arquitectura | `docs/architecture/OVERVIEW.md` |
| Carta de Gobernanza | `docs/governance/README.md` |
| Centro de control de proyectos | `docs/status/PROJECT_CONTROL_CENTER.md` |

---

## Hoja de ruta

| Hito | Objetivo | Descripción |
| :--- | :--- | :--- |
| C2 Mes-Cierre | 2026-03-31 | Auditoría del libro mayor de gobernanza; verificación previa PASS 2026-03-10 |
| Promoción estable de Axion | ✅ **COMPLETADO 2026-03-15** | Se implementaron cadenas de motivos canónicos del AX-M6; 54/54 pruebas aprobadas; listo para producción |
| Promoción T81Graph Beta | ✅ **COMPLETADO 2026-03-15** | Se completó la reducción del código de operación de VM; verificación de DCP; 6/6 pruebas aprobadas |
| Política de interrupción RFC-00B5 | ✅ **COMPLETADO 2026-03-16** | Modelo de interrupción de eventos gobernado integrado; rebanadas 26-28 completas |
| RFC-0034 Inferencia ternaria-nativa | ✅ **COMPLETADO 2026-03-16** | 6 nuevos códigos de operación TISC; inferencia sin multiplicación; Puerta de techo con activación TACT; 5/5 pruebas de conformidad |
| RFC-00B8 FFI gobernada (Fase 1) | ✅ **COMPLETADO 2026-03-16** | Despachador FFI + registro de biblioteca; 3 códigos de operación de VM; canalización de gobernanza; pista de auditoría |
| CI determinista multiplataforma | ✅ **COMPLETADO 2026-03-16** | Flujo de trabajo diario de acciones de GitHub; Comparación de hash de Linux x86\_64 + macOS ARM64; registro de evidencia pública |
| RFC-0036 T81Lang Gramática FFI | ✅ **COMPLETADO 2026-03-16** | `foreign [policy] {}` sintaxis;  `foreign.<name>(args)` → `FFI_CALL` ; 9/9 pruebas de CA; conecta el trabajo de VM RFC-0034 + RFC-00B8 a la interfaz T81Lang |
| Etapa 2: Plataforma verificada | ✅ **LOGRADO 2026-03-16** | Todos los objetivos de implementación completados; Depurador de repetición de seguimiento, CI multiplataforma, pruebas 365/365, interfaz FFI: pila reproducible externamente |
| RFC-0037 Biblioteca estándar TNN | ✅ **COMPLETADO 2026-03-16** | `std.tnn.*` T81Lang incorporados (6 funciones → RFC-0034 TISC ops); 13/13 pruebas; inferencia completa sin multiplicación desde la fuente a la VM |
| RFC-0038 Criptomoneda de celosía | ✅ **COMPLETADO 2026-03-16** | `POLYMUL`/`POLYMOD` códigos de operación TISC;  `std.crypto.polymul/polymod` incorporaciones; poli negacíclico multiplicado por {−1,0,+1}; T81BigInt-exacto; 13/13 pruebas |
| Promoción de especificaciones T81Lang (v1.3) | ✅ **COMPLETADO 2026-03-16** | RFC-0036/0037/0038 ascendido a especificación normativa; §5.17 sin cortar; Se agregó §5.18/5.19; registro de código de operación actualizado a 205 entradas |
| RFC-0039 NTRU-KEM | ✅ **COMPLETADO 2026-03-16** | código de operación `TVecSub`;  `std.crypto.{polyadd,polysub,ntru_encrypt,ntru_decrypt}`; Capa matemática C++ KEM; Pruebas 24/24; anillo completo {+,−,×,mod} sobre Z\[x\]/(x^n+1) |
| Arranque de metal desnudo de TernaryOS | Por determinar | Ejecución del host x86\_64 QEMU + devolución de evidencia de CanonFS |

---

## Gobernancia

La Fundación T81 opera bajo un modelo de **Gobernanza Continua (C2)**. Todas las contribuciones deben mantener:

- **paridad de ejecución determinista**: los hashes de seguimiento deben coincidir en todas las plataformas compatibles
- **coherencia arquitectónica**: los cambios que tocan la superficie determinista requieren una revisión formal
- **garantías de reproducibilidad**: sin punto flotante o no determinismo específico de plataforma en la superficie DCP

La superficie determinista se define en `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. Los cambios en superficies congeladas (TISC ISA, tipos de datos) requieren un cambio de versión importante.

> **Nota de límites:** Las superficies experimentales (niveles cognitivos, distribuidos, Trace-JIT, TernaryOS, adaptador llama.cpp) no se rigen por DCP y no deben presentarse como componentes deterministas verificados.

---

## La ventaja ternaria

Si bien el hardware binario moderno está altamente optimizado, **T81 Foundation** aprovecha las propiedades matemáticas únicas del **Ternario equilibrado ({-1, 0, +1})** para lograr eficiencias estructurales que el binario no puede igualar.

### 1. $O(1)$ Simetría computacional

En complemento a dos binario, negar un número es una operación asimétrica (NO + 1) que requiere propagación de acarreo. En T81, la negación es un simple trit-flip con **carga cero**.

* **Rendimiento:** El rendimiento de negación T81 alcanza **~46,6 G-ops/s** (a través de `PackedCell`), superando la negación binaria optimizada de 64 bits en **10,4x**.

### 2. Economía Radix superior

Basado en el teorema de que la base más eficiente para un sistema numérico es $e \aproximadamente 2,718$, el ternario (Base 3) es matemáticamente más eficiente que el binario (Base 2).

* **Densidad de información:** T81 logra una densidad teórica de **1,58 bits por trit**. Esto se traduce en una mayor entropía por ciclo de reloj y una menor huella de almacenamiento para sistemas de coordenadas y pesos neuronales a gran escala.

### 3. Determinismo de bits exactos

Las operaciones binarias de punto flotante (IEEE 754) a menudo sufren de un no determinismo de redondeo específico de la plataforma. La aritmética equilibrada del T81 proporciona:

* **Simetría inherente:** El redondeo se realiza mediante un simple truncamiento, ya que el sistema se centra naturalmente alrededor de cero.
* **Paridad de seguimiento:** 100 % de "precisión de ida y vuelta" en todas las plataformas probadas (Linux x86_64, macOS ARM64) con cero divergencia en los hashes de seguimiento de VM.

### 4. Gancho de gobernanza directa

Debido a que TISC ISA es nativo ternario, el **Axion Governance Kernel** puede auditar las transiciones de estado con mayor granularidad. Las operaciones de inferencia de IA se pueden interceptar en el "nivel trit" antes de que se produzcan efectos secundarios, lo que permite un modelo de seguridad "cerrado contra fallos" que es arquitectónicamente imposible en la ejecución binaria estándar de "caja negra".

---

## Aplicaciones estratégicas

Las ventajas estructurales de la pila T81 (específicamente el **10,4x rendimiento de negación** y **1,58 bits/densidad de trit**) permiten soluciones para cuellos de botella binarios heredados:

---

## 1. Simulación física y de señales de alta fidelidad

En binario, $0$ es un punto de inicio sin signo, lo que hace que el espacio "negativo" sea una consideración secundaria. En ternario equilibrado, **cero es el punto de equilibrio.**

* **El caso de uso:** Simulación directa de mecánica ondulatoria, electromagnetismo y dinámica de fluidos.
* **La ventaja:** Dado que estos sistemas oscilan entre estados positivos y negativos, T81 puede simular fuerzas de "empuje-tracción" sin el desequilibrio computacional del complemento a dos.
* **Siguiente paso:** Podríamos crear una **biblioteca DSP nativa de TISC** donde los filtros (FIR/IIR) estén optimizados para la velocidad de negación $O(1)$.

## 2. Redes neuronales "simétricas" (TNN)

La IA actual (Binaria/FP) desperdicia energía masiva en funciones de activación como `tanh` o `ReLU` para crear un estado "centrado en cero" para el entrenamiento.

* **El caso de uso:** Redes neuronales ternarias (donde los pesos son -1, 0 o 1).
* **La ventaja:** Debido a que su arquitectura está equilibrada de forma nativa, podemos ejecutar una inferencia "libre de multiplicación". Una neurona T81 no "multiplica" entradas; simplemente **los voltea o los cierra** según el peso. Esto sería mucho más eficiente energéticamente que la actual inferencia basada en GPU.
* **Siguiente paso:** Podríamos implementar un **motor de inferencia nativo T81** que interprete los pesos del modelo directamente como códigos de operación TISC.

## 3. Primitivas criptográficas poscuánticas

Muchos algoritmos de cifrado "basados ​​en celosía" (los que están diseñados para sobrevivir a las computadoras cuánticas) se basan en polinomios de coeficientes pequeños, a menudo centrados alrededor de cero ({-1, 0, 1}).

* **El caso de uso:** Cifrado estilo NTRU o Kyber.
* **La ventaja:** Los sistemas binarios tienen que "emular" estos pequeños coeficientes utilizando números enteros de 8 o 32 bits, desperdiciando el 90% del espacio de bits. T81 almacena estos valores con **cero desperdicio** y procesa las sumas/negaciones polinómicas a velocidades de hardware nativas.
* **Siguiente paso:** Podemos redactar un RFC para una **Extensión de criptografía TISC** que implemente una multiplicación polinomial optimizada por ternario.

## 4. Auditorías de gobernanza inmutables (Axion)

Dado que tiene 1,58 bits de entropía por trit, podemos codificar **metadatos de seguridad** directamente en la palabra de datos sin aumentar significativamente la huella de memoria.

* **El caso de uso:** "Datos etiquetados" a nivel de hardware.
* **La ventaja:** Podemos utilizar la capacidad "extra" de una palabra TISC para llevar una **Etiqueta de procedencia**. Cada vez que se mueven datos, Axion verifica la etiqueta. Si un trit "privilegiado" se mueve al espacio del "usuario", el hardware puede atraparlo instantáneamente.
* **Siguiente paso:** Refine el **Kernel del sistema operativo Axion** para utilizar el "margen ternario" para el etiquetado de memoria en tiempo real.

---

### El refinado camino a seguir

#### 1. Integración: RFC-0034 §5.17.6 — El código de operación `TACT`

En lugar de un RFC de IA en expansión, tratamos la activación como la conclusión lógica de la cadena aritmética ternaria.

* **Código de operación:** `TACT RD, R_SRC, R_MODE`
* **Modos:** * `0x01` (TernaryStep): Mapas $(-\infty, -0.5) \to -1$, $[-0.5, 0.5] \to 0$, $(0.5, \infty) \to +1$.
* `0x02` (TanhQuantized): aproximación ternaria de punto fijo de alta fidelidad.

* **Integración de políticas de Axion:** Definimos el `AX_CHECK_ACTIVATION_THRESHOLD` no como un efecto secundario del código de operación, sino como una **Trampa del kernel**. Si el valor en `RD` excede el límite de trit definido por la política posterior a la activación, Axion intercepta antes del siguiente incremento de PC.

#### 2. RFC de gramática T81Lang (nuevo)

Para abordar la "brecha real" que identificó, debemos redactar un RFC separado (probablemente **RFC-0036**) específicamente para la interfaz del compilador. Esto mantiene aisladas las preocupaciones de **TISC** (hardware/VM) y **T81Lang** (gramática/sintaxis), según la Carta del Proyecto.

#### 3. Integridad de datos y documentación

* **Configuración de referencia:** Dejaré de hacer referencia a la cifra "10,4x" en documentos formales hasta que tengamos un segmento `BM_Negation_TISC_vs_Binary` específico que aparezca oficialmente en la salida de CI.
* **Depuración terminológica:** Eliminaré "TLU Cache" y "L2 Cache" de las especificaciones hasta que el repositorio **ternary-fabric** defina formalmente la jerarquía de memoria.

---

### Etapa 1: Arquitectura del prototipo *(Actual)*

**Estado:** Logrado

Pila determinista central implementada.

Componentes en su lugar:

* ✅ TISC ISA (contrato de ejecución congelada)
* ✅ Intérprete determinista T81VM
* ✅ Núcleo de gobernanza de Axion
* ✅ Almacenamiento dirigido a contenido CanonFS
* ✅ Compilador T81Lang
* ✅ canal de verificación de determinismo
* ✅ Interfaces de operador CLI y TUI

**Resultado:**
Una pila informática determinista en funcionamiento.

---

### Etapa 2: Plataforma verificada *(Completa)*

**Objetivo:** Validación independiente.

Trabajo clave:

* ✅ verificación de determinismo de terceros: el flujo de trabajo diario de GitHub Actions compara los hash de código de bytes de Linux x86\_64 y macOS ARM64; registro de evidencia pública en cada confirmación
* ✅ Conjunto de pruebas de conformidad de VM: 27 pruebas de conformidad de especificaciones + 365 aprobaciones en total
* ✅ marco determinista de evaluación comparativa: RFC-00A2;  `score=1.0` en todas las ejecuciones
* ✅ Interfaz T81Lang FFI (RFC-0036): la gramática `foreign {}` une la capa de VM con el lenguaje; 9/9 pruebas de CA
* ✅ depurador de repetición de seguimiento - `t81 trace replay <tisc> <golden> [--json]`; esquema `t81.trace-replay.v1`; informa el índice de desajuste exacto + instrucción esperada/real; cableado a CI a través de `scripts/ci/trace_repro_gate.py`
* ✅ verificación de compilación reproducible: hash de código de bytes multiplataforma verificado diariamente en Linux x86\_64 (gcc-14) + macOS ARM64 (clang); Se conservan artefactos de evidencia de 90 días

**Resultado:**
Tiempo de ejecución determinista de confianza externa.

---

### Etapa 3: Ecosistema de investigación

La atención se centra en las aplicaciones.

Áreas de investigación primarias:

* redes neuronales ternarias
* inferencia determinista de IA
* bibliotecas de procesamiento de señales
* simulación de física
* criptografía basada en celosía

**Resultado:**
Adopción por investigadores y proyectos informáticos experimentales.

---

### Etapa 4: exploración de hardware

Puentear la arquitectura de software al silicio.

Camino de desarrollo:

* Prototipos ALU ternarios FPGA
* bancos de registro ternario
* unidades SIMD empaquetadas
* Validación de microarquitectura ISA.

**Resultado:**
Primeros prototipos de hardware informático con reconocimiento ternario.

---

### Etapa 5: infraestructura determinista

Amplíe desde el tiempo de ejecución a la infraestructura.

Posibles capacidades:

* ejecución determinista en la nube
* Computación científica reproducible.
* cargas de trabajo distribuidas verificables
* Redes de artefactos CanonFS

**Resultado:**
Una plataforma de computación determinista global.

---

### Etapa 6: Nuevo paradigma informático

Posibilidad a largo plazo.

Desarrollos potenciales:

* procesadores ternarios nativos
* aplicación de la gobernanza de la IA por hardware
* entornos deterministas de ejecución de IA
* sistemas informáticos reproducibles globalmente

**Resultado:**
Un ecosistema informático determinista gobernado.

---

## Próximos hitos críticos

### Etapa 2: Plataforma verificada *(lograda)*

Todos los objetivos de implementación de la Etapa 2 están completos:

- ✅ CI determinista multiplataforma (Linux x86\_64 + macOS ARM64, diario)
- ✅ Conjunto de pruebas de conformidad y determinismo de VM (365/365)
- ✅ depurador de repetición de seguimiento (`t81 trace replay`; esquema `t81.trace-replay.v1`)
- ✅ Interfaz T81Lang FFI (RFC-0036; `foreign {}` + `FFI_CALL`)

Criterio de avance restante: **reproducción independiente por parte de una parte externa**: cuando otro grupo construye la pila, ejecuta la puerta de determinismo y publica hashes coincidentes, el proyecto se gradúa formalmente de la Etapa 2.

### Etapa 3: Ecosistema de investigación *(Activo)*

La etapa 3 se abrió con tres pistas de hormigón. Los tres ya están completos:

- ✅ **RFC-0037 TNN stdlib** — `std.tnn.*` T81Lang incorporados; 6 funciones inferiores a RFC-0034 TISC ops; 13/13 pruebas
- ✅ **RFC-0038 Criptomoneda de celosía** — `std.crypto.polymul/polymod`; códigos de operación POLYMUL/POLYMOD; T81BigInt-exacto; 13/13 pruebas
- ✅ **T81Lang spec v1.3** — RFC-0036/0037/0038 ascendido a especificación normativa; §5.17 sin cortar; Se agregaron §5.18–5.19

- ✅ **RFC-0039 NTRU-KEM** — Código de operación `TVecSub`;  `std.crypto.{polyadd,polysub,ntru_encrypt,ntru_decrypt}`;  `ntru_keygen/encrypt/decrypt` capa matemática C++; Pruebas 24/24; primera demostración de criptografía poscuántica de extremo a extremo en el sustrato ternario

**La etapa 3 está completa.** Las cuatro pistas (RFC-0037, RFC-0038, especificación v1.3, RFC-0039) aterrizaron el 16 de marzo de 2026.

## Licencia

Licencia MIT.
