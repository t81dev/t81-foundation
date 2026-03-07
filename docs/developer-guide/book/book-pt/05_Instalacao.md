# Capítulo 5: Instalação e Verificação de Build

## 5.1 Pré-requisitos

**Status: Padronizado**

Compilar o T81 requer uma toolchain C++ moderna capaz de suportar recursos do C++23. O projeto impõe avisos estritos do compilador e conformidade com padrões para minimizar comportamento indefinido.

### Plataformas Suportadas
*   **Linux**: x86_64, ARM64 (aarch64), RISC-V (rv64gc)
*   **macOS**: Apple Silicon (M1/M2/M3), Intel (legado)
*   **Windows**: WSL2 recomendado (suporte MSVC é experimental)

### Requisitos da Toolchain
*   **Compilador**:
    *   Clang 18+ (Recomendado pela rigorosidade)
    *   GCC 14+
    *   MSVC 19.38+ (VS 2022)
*   **Sistema de Build**: CMake 3.25 ou mais recente.
*   **Python**: Python 3.10+ (Usado para scripts de validação e bindings).
*   **Ninja**: Recomendado para builds mais rápidos.

## 5.2 Compilando a partir da Fonte

**Status: Automatizado**

O processo de build padrão é encapsulado no comando `make cmake-ritual`, mas pode ser executado manualmente via CMake.

### 5.2.1 O Fluxo de Trabalho CMake

```bash
# 1. Clonar o repositório
git clone https://github.com/t81-foundation/t81.git
cd t81

# 2. Configurar (Modo Release recomendado para desempenho)
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DT81_USE_CXX23=ON \
    -DT81_BUILD_TESTS=ON

# 3. Compilar o executável principal
cmake --build build --target t81
```

### 5.2.2 Opções de Build
As seguintes opções do CMake controlam a configuração do build:

| Opção | Padrão | Descrição |
| :--- | :--- | :--- |
| `T81_USE_CXX23` | `ON` | Habilitar recursos C++23 (ex: `std::expected`, `std::print`). |
| `T81_BUILD_TESTS` | `ON` | Compilar a suíte de verificação (`t81_*_test`). |
| `T81_BUILD_EXAMPLES` | `ON` | Compilar programas de demonstração em `examples/`. |
| `T81_ENABLE_ASAN` | `OFF` | Habilitar AddressSanitizer (Somente Debug). |
| `T81_ENABLE_UBSAN` | `OFF` | Habilitar UndefinedBehaviorSanitizer. |

> **Nota sobre Determinismo**: Para garantir determinismo estrito (desabilitando fallbacks de FPU do hospedeiro para transcendentais), defina `T81_DETERMINISTIC` manualmente se não definido por padrão:
> `cmake -B build -DCMAKE_CXX_FLAGS="-DT81_DETERMINISTIC"`

## 5.3 Verificando o Build

**Status: Crítico**

Após a compilação, você **deve** verificar se o binário produzido está em conformidade com a especificação T81. Uma compilação bem-sucedida não garante execução correta.

### 5.3.1 Executando Testes Unitários
Execute a suíte de testes padrão via `ctest`. Isso executa centenas de testes baseados em propriedades.

```bash
cd build
ctest --output-on-failure
```

### 5.3.2 O Portão de Determinismo (Determinism Gate)
A verificação mais crítica é o **Portão de Determinismo**. Este script compila um programa de referência canônico, executa-o e compara o hash do Trace Axion resultante com um valor conhecido como bom.

```bash
# Executar o repro gate
python3 scripts/ci/t81lang_repro_gate.py --binary ./build/t81
```

**Saída Esperada**:
```text
[PASS] Trace Hash: canon:sha3:a7f92b... MATCHES expected baseline.
[PASS] Cycles: 10420 (Exact match)
[PASS] Determinism verification successful.
```

Se este script falhar, o build está **contaminado** e não deve ser usado para tarefas de produção ou auditoria.

### 5.3.3 Verificando Metas de Arquitetura
Certifique-se de que o grafo de build corresponda à especificação arquitetural:

```bash
python3 scripts/ci/check_architecture_targets.py
```

## 5.4 Solução de Problemas

*   **"C++23 not supported"**: Atualize seu compilador. O T81 depende fortemente de recursos modernos do C++ para segurança de tipo.
*   **"Trace Hash Mismatch"**: Você pode estar vinculando a uma versão diferente das bibliotecas padrão, ou o fallback `dmath` foi acionado. Certifique-se de que `T81_DETERMINISTIC` está definido.
*   **"SIMD Instruction Fault"**: O T81 tenta detectar a disponibilidade de AVX2/NEON. Se estiver fazendo compilação cruzada, verifique se as flags de destino estão corretas.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
