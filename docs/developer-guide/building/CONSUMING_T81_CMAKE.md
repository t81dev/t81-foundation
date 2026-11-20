# Consuming T81 via CMake

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Consuming T81 via CMake](#consuming-t81-via-cmake)
  - [Public API Rule](#public-api-rule)
  - [Install T81](#install-t81)
  - [Build External Consumer](#build-external-consumer)
  - [Troubleshooting](#troubleshooting)

<!-- T81-TOC:END -->


This guide shows how an external CMake project can consume T81 using installed package exports and public headers only.

## Public API Rule

External consumers must include headers from `include/t81/**` only.
Do not include from `core/**`, `internal/**`, `kernel/**`, `runtime/**`, or relative source paths.

## Install T81

From repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cmake --install build --prefix /tmp/t81_install
```

Installed package configs:

- `/tmp/t81_install/lib/cmake/T81/T81Config.cmake`
- `/tmp/t81_install/lib/cmake/T81Foundation/T81FoundationConfig.cmake`

## Build External Consumer

Example consumer lives at `examples/consumer_cmake/`.

```bash
cmake -S examples/consumer_cmake -B /tmp/t81_consumer_build -DCMAKE_PREFIX_PATH=/tmp/t81_install
cmake --build /tmp/t81_consumer_build --parallel
/tmp/t81_consumer_build/t81_consumer
```

Expected output:

```text
t81-consumer: value=42
```

## Troubleshooting

- `Could not find a package configuration file`: ensure `-DCMAKE_PREFIX_PATH=/tmp/t81_install` is set and install step completed.
- Header not found for `t81/...`: verify the include is under `include/t81/**` and not an internal/source path.
- Link errors for `T81::...` targets: rerun install and ensure exported target files exist under `lib/cmake/T81Foundation/`.
