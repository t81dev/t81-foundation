# T81 Foundation: Стек троичных вычислений

<div align="center">
<br/>
<img src="docs/assets/img/banner.png" alt="T81 Foundation" width="100%"/>
<br/><br/>

[![Парадигма: Троичные вычисления](https://img.shields.io/badge/Paradigm-Ternary%20Computing-red?style=flat-square)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Дизайн: Спецификация-First](https://img.shields.io/badge/Design-Specification%20First-blue?style=flat-square)](#)
[![CI Статус](https://github.com/t81dev/t81-foundation/actions/workflows/ci.yml/badge.svg)](https://github.com/t81dev/t81-foundation/actions/workflows/ci.yml)
[![Ядро: C++20](https://img.shields.io/badge/Core-C%2B%2B20-0d1117?style=flat-square&logo=cplusplus)](#)
[![Лицензия: MIT/GPL-3.0](https://img.shields.io/badge/License-MIT%20%2F%20GPL--3.0-green?style=flat-square)](ЛИЦЕНЗИЯ-MIT)

* * *

[![сборка / macos-latest / clang](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=macos%20clang&style=flat-square&logo=apple)](https://github.com/t81dev/t81-foundation/actions)
[![сборка / windows-latest / clang-cl](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=windows%20clang-cl&style=flat-square&logo=windows&color=brightgreen)](https://github.com/t81dev/t81-foundation/actions)
[![сборка / windows-latest / msvc](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=windows%20msvc&style=flat-square&logo=visualstudio&color=brightgreen)](https://github.com/t81dev/t81-foundation/actions)
[![сборка / ubuntu-latest / gcc](https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?label=ubuntu%20gcc&style=flat-square&logo=ubuntu)](https://github.com/t81dev/t81-foundation/actions)

* * *

[![Отрицание](https://img.shields.io/badge/Negation-7.18_Gops/s_(faster_per_digit_than_int64)-brightgreen)](https://github.com/t81dev/t81-foundation/)
[![Диапазон](https://img.shields.io/badge/Диапазон-40×_greater_than___int128-blue)](https://github.com/t81dev/t81-foundation/)
[![Переполнение](https://img.shields.io/badge/Overflow-NEVER-red)](https://github.com/t81dev/t81-foundation/)
[![Точное Математика](https://img.shields.io/badge/Math-Perfect-yellow)](https://github.com/t81dev/t81-foundation/)

<br/><br/>

</div>

## 1. Резюме

T81 — это суверенный, детерминированный стек, построенный на сбалансированной троичной системе счисления (−1, 0, +1). Всё, от основных арифметических типов до компилятора, виртуальной машины, библиотеки тензоров и набора инструментов для бенчмаркинга, призвано продемонстрировать, что троичная математика может быть точной, контролируемой и производительной в сочетании с современным оборудованием C++ и SIMD.

Основные возможности:
- **Сбалансированные троичные примитивы**: `T81Int`, `T81Fraction`, `T81Float`, `T81Tensor` и подобные реализуют точную арифметику с нулевым скрытым переносом, безопасностью кругового обхода и ловушками, поддерживающими Axion.
- **Компилятор T81Lang + виртуальная машина TISC**: анализ кода T81, создание байт-кода TISC и детерминированное выполнение внутри виртуальной машины Hanoi.
- **Бенчмаркинг Native + Classic**: сравнение представлений на основе tryte (классических) и AVX2 (национных), вывод столбцов Classic/Native/Binary и метрик задержки/пропускной способности. - **Инструментарий весов**: импорт SafeTensors/GGUF в `t81w`, проверка метаданных и квантизация тензоров в модели T3_K GGUF (с новой CLI `weights quantize`).

В настоящее время стек представляет собой позднюю альфа-/раннюю бета-версию набора высоконадежных числовых данных (хорошо протестированные основные библиотеки), обёрнутых вокруг экспериментального, но пригодного к использованию конвейера компилятора/виртуальной машины.

## 2. Быстрый старт

### Сборка и тестирование

```bash
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake -S . -B сборка -DCMAKE_BUILD_TYPE=Release
cmake --build сборка --parallel
ctest --test-dir сборка --output-on-failure
```

### Шпаргалка по CLI

```text
t81 compile <file.t81> [-o <file.tisc>]
t81 run <file.t81|.tisc>
t81 check <file.t81>
t81 benchmark [флаги бенчмарка]
t81 weights import <safetensors|gguf> [--format <safetensors|gguf>] [-o out.t81w]
t81 weights info <model.t81w>
t81 weights quantize <dir|file.safetensors> --to-gguf <out.gguf>
```

Основные моменты работы с инструментом Weights:
- `weights import` преобразует BitNet/SafeTensors/GGUF в канонический файл `.t81w` с метаданными SHA3-512 и статистикой плотности.
- `weights info` выводит триты, лимбы, объём памяти (биты/трит), разреженность, формат, контрольную сумму и канонические подсказки CanonFS.
- `weights quantize … --to-gguf` запускает квантователь T3_K (128-элементные трит-блоки, масштабирование по блокам) и создаёт файл GGUF, готовый для llama.cpp с поддержкой T3_K.

## 3. Сводка команд

| Команда | Что она делает |
| --- | --- |
| `t81 compile` | Компилирует исходный файл `.t81` в байт-код TISC с диагностикой. |
| `t81 run` | Компилирует (при необходимости) и выполняет программы TISC внутри HanoiVM. |
| `t81 check` | Быстрая проверка только синтаксиса исходного кода T81. |
| `t81 benchmark` | Запускает `benchmarks/benchmark_runner`, обновляет `docs/benchmarks.md`, добавляя статистику и основные моменты для Classic/Native/Binary. |
| `t81 weights import` | Импорт BitNet/SafeTensors/GGUF в нативный двоичный файл `.t81w`
