# T81 Foundation: A Pilha de Computação Nativa Ternária

<div align="center">

<br/>

<img src="docs/assets/img/banner.png" alt="T81 Foundation" width="100%"/>

<br/><br/>

[![Paradigma: Computação Ternária](https://img.shields.io/badge/Paradigm-Ternary%20Computing-red?style=flat-square)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Design: Especificação em Primeiro Lugar](https://img.shields.io/badge/Design-Specification%20First-blue?style=flat-square)](#)
[![CI Status](https://github.com/t81dev/t81-foundation/actions/workflows/ci.yml/badge.svg)](https://github.com/t81dev/t81-foundation/actions/workflows/ci.yml)
[![Core: C++20](https://img.shields.io/badge/Core-C%2B%2B20-0d1117?style=flat-square&logo=cplusplus)](#)
[![License: MIT/GPL-3.0](https://img.shields.io/badge/License-MIT%20%2F%20GPL--3.0-green?style=flat-square)](LICENSE-MIT)

* * *

[![build / macos-latest / clang](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=macos%20clang&style=flat-square&logo=apple)](https://github.com/t81dev/t81-foundation/actions)
[![build / windows-latest / clang-cl](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=windows%20clang-cl&style=flat-square&logo=windows&color=brightgreen)](https://github.com/t81dev/t81-foundation/actions)
[![build / windows-latest / [![build / ubuntu-latest / gcc](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=windows%20msvc&style=flat-square&logo=visualstudio&color=brightgreen)](https://github.com/t81dev/t81-foundation/actions)
[![build / ubuntu-latest / gcc](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=ubuntu%20gcc&style=flat-square&logo=ubuntu)](https://github.com/t81dev/t81-foundation/actions)

* * *

[![Negação](https://img.shields.io/badge/Negation-7.18_Gops/s_(faster_per_digit_than_int64)-brightgreen)](https://github.com/t81dev/t81-foundation/)
[![Intervalo](https://img.shields.io/badge/Range-40×_greater_than___int128-blue)](https://github.com/t81dev/t81-foundation/)
[![Estouro](https://img.shields.io/badge/Overflow-NEVER-red)](https://github.com/t81dev/t81-foundation/)
[![Exato](https://img.shields.io/badge/Overflow-NEVER-red)](https://github.com/t81dev/t81-foundation/)
[![Exato](https://img.shields.io/badge/Overflow-NEVER-red) Matemática](https://img.shields.io/badge/Math-Perfect-yellow)](https://github.com/t81dev/t81-foundation/)

<br/><br/>

</div>

## 1. Apresentação Rápida

T81 é uma pilha soberana e determinística construída em torno de tipos ternários balanceados (−1, 0, +1). Tudo, desde os tipos aritméticos principais até o compilador, a máquina virtual, a biblioteca de tensores e a cadeia de ferramentas de benchmark, foi projetado para demonstrar que a matemática ternária pode ser exata, auditável e eficiente quando combinada com C++ moderno e hardware SIMD.

Recursos principais:
- **Primitivas ternárias balanceadas**: `T81Int`, `T81Fraction`, `T81Float`, `T81Tensor` e similares implementam aritmética exata com zero carry oculto, segurança de ida e volta e traps compatíveis com Axion. - **Compilador T81Lang + VM TISC**: analisa o código T81, emite bytecode TISC e executa deterministicamente dentro da HanoiVM.

- **Benchmarking Nativo + Clássico**: compara representações baseadas em trytes (clássicas) com representações compatíveis com AVX2 (nativas), relatando colunas Clássicas/Nativas/Bigráficas e métricas de latência/largura de banda.

- **Ferramentas de Pesos**: importa SafeTensors/GGUF para `t81w`, inspeciona metadados e quantiza tensores em modelos GGUF T3_K (com o novo comando de linha de comando `weights quantize`).

A pilha atualmente consiste em uma coleção em estágio avançado de alfa/início de beta de ferramentas numéricas de alta confiabilidade (bibliotecas principais bem testadas) integradas a um pipeline de compilador/VM experimental, porém utilizável.

## 2. Início Rápido

### Compilar e Testar

```bash
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Guia de Referência Rápida da CLI

```texto
t81 compile <arquivo.t81> [-o <arquivo.tisc>]
t81 run <arquivo.t81|.tisc>
t81 check <arquivo.t81>
t81 benchmark [flags de benchmark]
t81 weights import <safetensors|gguf> [--format <safetensors|gguf>] [-o out.t81w]
t81 weights info <model.t81w>
t81 weights quantize <dir|arquivo.safetensors> --to-gguf <out.gguf>
```

Destaques das ferramentas de pesos:
- `weights import` converte BitNet/SafeTensors/GGUF para `.t81w` canônico com metadados SHA3-512 e estatísticas de densidade.
- `weights info` imprime trits, limbs, armazenamento (bits/trit), esparsidade, formato, checksum e dicas canônicas do CanonFS.
- `weights quantize … --to-gguf` executa o quantizador T3_K (blocos de trits de 128 elementos, escala por bloco) e gera um arquivo GGUF pronto para llama.cpp com suporte a T3_K.

## 3. Resumo dos Comandos

| Comando | O que faz |

| --- | --- |
| `t81 compile` | Compila um arquivo fonte `.t81` para bytecode TISC com diagnósticos. |
| `t81 run` | Compila (se necessário) e executa programas TISC dentro da HanoiVM. |
| `t81 check` | Validação rápida apenas da sintaxe do código fonte T81. |
| `t81 benchmark` | Executa `benchmarks/benchmark_runner`, atualiza `docs/benchmarks.md` com estatísticas e destaques para Classic/Native/Binary. |
| `t81 weights import` | Importa BitNet/SafeTensors/GGUF para um binário nativo `.t81w`
