# Fundación T81: La pila de computación nativa ternaria

<div align="center">
<br/>
<img src="docs/assets/img/banner.png" alt="Fundación T81" width="100%"/>
<br/><br/>

[![Paradigma: Computación ternaria](https://img.shields.io/badge/Paradigm-Ternary%20Computing-red?style=flat-square)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Diseño: Especificación primero](https://img.shields.io/badge/Design-Specification%20First-blue?style=flat-square)](#)
[![CI Estado](https://github.com/t81dev/t81-foundation/actions/workflows/ci.yml/badge.svg)](https://github.com/t81dev/t81-foundation/actions/workflows/ci.yml)
[![Núcleo: C++20](https://img.shields.io/badge/Core-C%2B%2B20-0d1117?style=flat-square&logo=cplusplus)](#)
[![Licencia: MIT/GPL-3.0](https://img.shields.io/badge/License-MIT%20%2F%20GPL--3.0-green?style=flat-square)](LICENCIA-MIT)

* * *

[![build / macos-latest / Sonido metálico](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=macos%20clang&style=flat-square&logo=apple)](https://github.com/t81dev/t81-foundation/actions)
[![compilación / windows-latest / sonido metálico-cl](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=windows%20clang-cl&style=flat-square&logo=windows&color=brightgreen)](https://github.com/t81dev/t81-foundation/actions)
[![compilación / windows-latest / msvc](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=windows%20msvc&style=flat-square&logo=visualstudio&color=brightgreen)](https://github.com/t81dev/t81-foundation/actions)
[![build / ubuntu-latest / gcc](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=ubuntu%20gcc&style=flat-square&logo=ubuntu)](https://github.com/t81dev/t81-foundation/actions)

* * *

[![Negación](https://img.shields.io/badge/Negation-7.18_Gops/s_(faster_per_digit_than_int64)-brightgreen)](https://github.com/t81dev/t81-foundation/)
[![Rango](https://img.shields.io/badge/Range-40×_greater_than___int128-blue)](https://github.com/t81dev/t81-foundation/)
[![Desbordamiento](https://img.shields.io/badge/Overflow-NEVER-red)](https://github.com/t81dev/t81-foundation/)
[![Exacto Matemáticas](https://img.shields.io/badge/Math-Perfect-yellow)](https://github.com/t81dev/t81-foundation/)

*   *   *

<div align="center">

[![English](https://img.shields.io/badge/Language-English-blue?style=flat-square)](/README.md)
[![简体中文](https://img.shields.io/badge/Language-%E7%AE%80%E4%BD%93%E4%B8%AD%E6%96%87-red?style=flat-square)](/README.zh-CN.md)
[![Español](https://img.shields.io/badge/Language-Español-green?style=flat-square)](/README.es.md)
[![Русский](https://img.shields.io/badge/Language-Русский-brightgreen?style=flat-square)](/README.ru.md)
[![Português](https://img.shields.io/badge/Language-Português%20(Brasil)-blueviolet?style=flat-square)](/README.pt-BR.md)

*   *   *

</div>
<br>
<br/><br/>

</div>

## 1. Presentación del Elevator

T81 es una pila soberana y determinista construida alrededor de la matemática ternaria balanceada (−1, 0, +1). Todo, desde los tipos aritméticos principales hasta el compilador, la máquina virtual, la biblioteca de tensores y la cadena de herramientas de benchmarking, está diseñado para demostrar que la matemática ternaria puede ser exacta, auditable y eficiente cuando se combina con hardware moderno de C++ y SIMD.

Características principales:
- **Primitivas ternarias balanceadas**: `T81Int`, `T81Fraction`, `T81Float`, `T81Tensor` y similares implementan aritmética exacta con cero acarreos ocultos, seguridad de ida y vuelta y trampas compatibles con Axion. - **Compilador T81Lang + VM TISC**: Analiza código T81, emite bytecode TISC y ejecuta de forma determinista dentro de HanoiVM.
- **Comparación de rendimiento nativo y clásico**: Compara representaciones basadas en tryte (clásicas) con representaciones compatibles con AVX2 (nativas), generando informes de columnas clásicas, nativas y binarias, así como métricas de latencia y ancho de banda.
- **Herramientas de ponderaciones**: Importa SafeTensors/GGUF a `t81w`, inspecciona metadatos y cuantifica tensores en modelos GGUF T3_K (con la nueva CLI `weights quantize`).

La pila es actualmente una colección de valores numéricos de alta confianza (bibliotecas centrales bien probadas) en fase alfa tardía o beta temprana, integrada en una canalización de compilador/VM experimental pero utilizable.

## 2. Inicio rápido

### Compilación y pruebas

```bash
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake -S . -B build -DCMAKE_BUILD_TYPE=Versión
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Hoja de referencia de la CLI

```texto
t81 compilar <archivo.t81> [-o <archivo.tisc>]
t81 ejecutar <archivo.t81|.tisc>
t81 comprobar <archivo.t81>
t81 benchmark [indicadores de benchmark]
t81 importar pesos <safetensors|gguf> [--format <safetensors|gguf>] [-o out.t81w]
t81 información de pesos <modelo.t81w>
t81 cuantificar pesos <dir|archivo.safetensors> --to-gguf <out.gguf>
```

Características destacadas de las herramientas de pesos:
- `weights import` convierte BitNet/SafeTensors/GGUF a `.t81w` canónico con metadatos SHA3-512 y estadísticas de densidad.
- `weights info` imprime trits, limbs, almacenamiento (bits/trit), dispersión, formato, suma de comprobación y sugerencias canónicas de CanonFS.
- `weights quantize … --to-gguf` ejecuta el cuantificador T3_K (bloques trit de 128 elementos, escala por bloque) y emite un archivo GGUF listo para llama.cpp compatible con T3_K.

## 3. Resumen de comandos

| Comando | Qué hace |
| --- | --- |
| `t81 compile` | Compila un archivo fuente `.t81` a código de bytes TISC con diagnósticos. |
| `t81 run` | Compila (si es necesario) y ejecuta programas TISC dentro de HanoiVM. |
| `t81 check` | Validación rápida de sintaxis del código fuente T81. |
| `t81 benchmark` | Ejecuta `benchmarks/benchmark_runner`, actualiza `docs/benchmarks.md` con estadísticas clásicas, nativas y binarias y resaltados. |
| `t81 weights import` | Importa BitNet/SafeTensors/GGUF a un binario nativo `.t81w`
