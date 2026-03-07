# Capítulo 6: Uso de CLI e API

## 6.1 A Interface de Linha de Comando T81

**Status: Implementado**

O binário `t81` é o ponto de entrada primário para interação com o runtime. Ele segue o padrão `subcommand` (subcomando).

### 6.1.1 Compilação (`compile`)
Compila código-fonte T81 (`.t81`) em bytecode TISC (`.tisc`).

```bash
t81 compile main.t81 -o main.tisc
```

**Opções**:
*   `-o, --output <file>`: Caminho do bytecode de saída.
*   `--policy <file>`: Anexar uma política Axion ao bytecode (embutida).

### 6.1.2 Execução (`run`)
Executa um arquivo de bytecode TISC ou um arquivo fonte (JIT-compile-and-go).

```bash
# Executar bytecode
t81 run main.tisc

# Executar fonte diretamente
t81 run main.t81
```

**Opções**:
*   `--policy <file>`: Impor um arquivo de política Axion específico durante a execução.
*   `--weights <file>`: Anexar um modelo de tensor (`.t81w`, `.safetensors`, `.gguf`) ao contexto.

### 6.1.3 Análise de Trace (`trace`)
A suíte de subcomandos `trace` gerencia os logs de auditoria Axion.

*   `t81 trace show <trace_file>`: Dump legível por humanos de um trace.
*   `t81 trace diff <trace_a> <trace_b>`: Comparar dois traces por divergência.
*   `t81 trace replay <program.tisc> <trace_file>`: Reexecutar um programa e verificar se ele produz exatamente o mesmo trace que o arquivo.

### 6.1.4 Modo Interativo (`repl`)
Lança o loop de Leitura-Avaliação-Impressão (REPL).

```bash
t81 repl
```
Comandos dentro do REPL:
*   `:load <file>`: Carregar um script.
*   `:model <path>`: Carregar um arquivo de pesos dinamicamente.
*   `:trace`: Mostrar o trace da última execução.

## 6.2 Embutindo T81 (API C++)

**Status: Estável**

Para embutir o T81 em uma aplicação hospedeira (ex: um motor de jogo ou um nó distribuído), use `t81::vm::IVirtualMachine`.

```cpp
#include <t81/vm/vm.hpp>
#include <t81/isa/program.hpp>

int main() {
    // 1. Criar VM
    auto vm = t81::vm::make_interpreter_vm();

    // 2. Carregar Programa
    auto prog = t81::tisc::load_program("main.tisc");
    vm->load_program(prog);

    // 3. Configurar Política
    // (Opcional: Anexar ganchos Axion personalizados)

    // 4. Rodar
    auto result = vm->run_to_halt();
    if (!result) {
        std::cerr << "Armadilha: " << result.error().reason << "\n";
    }
}
```

## 6.3 Embutindo T81 (API Python)

**Status: Implementado**

Os bindings Python permitem controlar a T81VM a partir do Python, principalmente para testes e orquestração.

```python
import t81

# Criar VM
vm = t81.VirtualMachine()

# Carregar Código
vm.load_source("""
let x = 10;
let y = 20;
x + y;
""")

# Executar
result = vm.run()
print(f"Resultado: {result}")
```

## 6.4 Depuração

**Status: Implementado**

O comando `t81 debug` lança um depurador estilo GDB para TISC.

*   `step` / `s`: Passo de uma instrução.
*   `next` / `n`: Passo sobre chamada.
*   `reg`: Dump de registradores.
*   `stack`: Dump de pilha.
*   `trace`: Mostrar histórico de trace recente.

```bash
t81 debug main.tisc
(t81-gdb) break 10
(t81-gdb) run
Breakpoint em PC=10
(t81-gdb) reg r1
r1 = 42 (Int)
```

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
