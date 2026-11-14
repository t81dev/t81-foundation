# Capítulo 5: Instalación y Verificación de la Construcción

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Capítulo 5: Instalación y Verificación de la Construcción](#capítulo-5-instalación-y-verificación-de-la-construcción)
  - [5.1 Requisitos Previos](#51-requisitos-previos)
    - [Plataformas Soportadas](#plataformas-soportadas)
    - [Requisitos de la Cadena de Herramientas](#requisitos-de-la-cadena-de-herramientas)
  - [5.2 Construcción desde la Fuente](#52-construcción-desde-la-fuente)
    - [5.2.1 El Flujo de Trabajo CMake](#521-el-flujo-de-trabajo-cmake)
- [1. Clonar el repositorio](#1-clonar-el-repositorio)
- [2. Configurar (Recomendado modo Release para rendimiento)](#2-configurar-recomendado-modo-release-para-rendimiento)
- [3. Construir el ejecutable principal](#3-construir-el-ejecutable-principal)
    - [5.2.2 Opciones de Construcción](#522-opciones-de-construcción)
  - [5.3 Verificando la Construcción](#53-verificando-la-construcción)
    - [5.3.1 Ejecutando Pruebas Unitarias](#531-ejecutando-pruebas-unitarias)
    - [5.3.2 La Puerta de Determinismo (Determinism Gate)](#532-la-puerta-de-determinismo-determinism-gate)
- [Ejecutar la puerta de reproducción](#ejecutar-la-puerta-de-reproducción)
    - [5.3.3 Verificando Objetivos de Arquitectura](#533-verificando-objetivos-de-arquitectura)
  - [5.4 Solución de Problemas](#54-solución-de-problemas)

<!-- T81-TOC:END -->


## 5.1 Requisitos Previos

**Estado: Estandarizado**

Construir T81 requiere una cadena de herramientas C++ moderna capaz de soportar las características de C++23. El proyecto impone advertencias estrictas del compilador y cumplimiento de estándares para minimizar el comportamiento indefinido.

### Plataformas Soportadas
*   **Linux**: x86_64, ARM64 (aarch64), RISC-V (rv64gc)
*   **macOS**: Apple Silicon (M1/M2/M3), Intel (heredado)
*   **Windows**: WSL2 recomendado (el soporte de MSVC es experimental)

### Requisitos de la Cadena de Herramientas
*   **Compilador**:
    *   Clang 18+ (Recomendado por su rigurosidad)
    *   GCC 14+
    *   MSVC 19.38+ (VS 2022)
*   **Sistema de Construcción**: CMake 3.25 o más reciente.
*   **Python**: Python 3.10+ (Usado para scripts de validación y bindings).
*   **Ninja**: Recomendado para construcciones más rápidas.

## 5.2 Construcción desde la Fuente

**Estado: Automatizado**

El proceso de construcción estándar está encapsulado en el comando `make cmake-ritual`, pero puede ejecutarse manualmente a través de CMake.

### 5.2.1 El Flujo de Trabajo CMake

```bash
# 1. Clonar el repositorio
git clone https://github.com/t81-foundation/t81.git
cd t81

# 2. Configurar (Recomendado modo Release para rendimiento)
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DT81_USE_CXX23=ON \
    -DT81_BUILD_TESTS=ON

# 3. Construir el ejecutable principal
cmake --build build --target t81
```

### 5.2.2 Opciones de Construcción
Las siguientes opciones de CMake controlan la configuración de la construcción:

| Opción | Por Defecto | Descripción |
| :--- | :--- | :--- |
| `T81_USE_CXX23` | `ON` | Habilitar características de C++23 (ej. `std::expected`, `std::print`). |
| `T81_BUILD_TESTS` | `ON` | Compilar la suite de verificación (`t81_*_test`). |
| `T81_BUILD_EXAMPLES` | `ON` | Compilar programas de demostración en `examples/`. |
| `T81_ENABLE_ASAN` | `OFF` | Habilitar AddressSanitizer (Solo Debug). |
| `T81_ENABLE_UBSAN` | `OFF` | Habilitar UndefinedBehaviorSanitizer. |

> **Nota sobre Determinismo**: Para asegurar un determinismo estricto (deshabilitando los respaldos de FPU del host para trascendentales), define `T81_DETERMINISTIC` manualmente si no está configurado por defecto:
> `cmake -B build -DCMAKE_CXX_FLAGS="-DT81_DETERMINISTIC"`

## 5.3 Verificando la Construcción

**Estado: Crítico**

Después de construir, **debes** verificar que el binario producido cumpla con la especificación T81. Una compilación exitosa no garantiza una ejecución correcta.

### 5.3.1 Ejecutando Pruebas Unitarias
Ejecuta la suite de pruebas estándar a través de `ctest`. Esto ejecuta cientos de pruebas basadas en propiedades.

```bash
cd build
ctest --output-on-failure
```

### 5.3.2 La Puerta de Determinismo (Determinism Gate)
La comprobación más crítica es la **Puerta de Determinismo**. Este script compila un programa de referencia canónico, lo ejecuta y compara el hash de la Traza Axion resultante con un valor conocido como bueno.

```bash
# Ejecutar la puerta de reproducción
python3 scripts/ci/t81lang_repro_gate.py --binary ./build/t81
```

**Salida Esperada**:
```text
[PASS] Trace Hash: canon:sha3:a7f92b... MATCHES expected baseline.
[PASS] Cycles: 10420 (Exact match)
[PASS] Determinism verification successful.
```

Si este script falla, la construcción está **contaminada** y no debe usarse para tareas de producción o auditoría.

### 5.3.3 Verificando Objetivos de Arquitectura
Asegura que el gráfico de construcción coincida con la especificación arquitectónica:

```bash
python3 scripts/ci/check_architecture_targets.py
```

## 5.4 Solución de Problemas

*   **"C++23 not supported"**: Actualiza tu compilador. T81 depende en gran medida de las características modernas de C++ para la seguridad de tipos.
*   **"Trace Hash Mismatch"**: Es posible que estés enlazando con una versión diferente de las bibliotecas estándar, o se activó el respaldo `dmath`. Asegúrate de que `T81_DETERMINISTIC` esté definido.
*   **"SIMD Instruction Fault"**: T81 intenta detectar la disponibilidad de AVX2/NEON. Si estás compilando de forma cruzada, asegúrate de que las banderas de destino sean correctas.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
