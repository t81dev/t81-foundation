# Capítulo 6: Uso de CLI y API

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Capítulo 6: Uso de CLI y API](#capítulo-6-uso-de-cli-y-api)
  - [6.1 La Interfaz de Línea de Comandos T81](#61-la-interfaz-de-línea-de-comandos-t81)
    - [6.1.1 Compilación (`compile`)](#611-compilación-`compile`)
    - [6.1.2 Ejecución (`run`)](#612-ejecución-`run`)
- [Ejecutar bytecode](#ejecutar-bytecode)
- [Ejecutar fuente directamente](#ejecutar-fuente-directamente)
    - [6.1.3 Análisis de Traza (`trace`)](#613-análisis-de-traza-`trace`)
    - [6.1.4 Modo Interactivo (`repl`)](#614-modo-interactivo-`repl`)
  - [6.2 Embebiendo T81 (API C++)](#62-embebiendo-t81-api-c++)
  - [6.3 Embebiendo T81 (API Python)](#63-embebiendo-t81-api-python)
- [Crear VM](#crear-vm)
- [Cargar Código](#cargar-código)
- [Ejecutar](#ejecutar)
  - [6.4 Depuración](#64-depuración)

<!-- T81-TOC:END -->


## 6.1 La Interfaz de Línea de Comandos T81

**Estado: Implementado**

El binario `t81` es el punto de entrada principal para la interacción con el runtime. Sigue el patrón estándar de `subcommand` (subcomando).

### 6.1.1 Compilación (`compile`)
Compila código fuente T81 (`.t81`) en bytecode TISC (`.tisc`).

```bash
t81 compile main.t81 -o main.tisc
```

**Opciones**:
*   `-o, --output <file>`: Ruta del bytecode de salida.
*   `--policy <file>`: Adjuntar una política Axion al bytecode (embebida).

### 6.1.2 Ejecución (`run`)
Ejecuta un archivo de bytecode TISC o un archivo fuente (JIT-compile-and-go).

```bash
# Ejecutar bytecode
t81 run main.tisc

# Ejecutar fuente directamente
t81 run main.t81
```

**Opciones**:
*   `--policy <file>`: Imponer un archivo de política Axion específico durante la ejecución.
*   `--weights <file>`: Adjuntar un modelo de tensor (`.t81w`, `.safetensors`, `.gguf`) al contexto.

### 6.1.3 Análisis de Traza (`trace`)
El conjunto de subcomandos `trace` gestiona los registros de auditoría Axion.

*   `t81 trace show <trace_file>`: Volcado legible por humanos de una traza.
*   `t81 trace diff <trace_a> <trace_b>`: Comparar dos trazas por divergencia.
*   `t81 trace replay <program.tisc> <trace_file>`: Reejecutar un programa y verificar que produce exactamente la misma traza que el archivo.

### 6.1.4 Modo Interactivo (`repl`)
Lanza el bucle de Lectura-Evaluación-Impresión (REPL).

```bash
t81 repl
```
Comandos dentro de REPL:
*   `:load <file>`: Cargar un script.
*   `:model <path>`: Cargar un archivo de pesos dinámicamente.
*   `:trace`: Mostrar la traza de la última ejecución.

## 6.2 Embebiendo T81 (API C++)

**Estado: Estable**

Para embeber T81 en una aplicación host (ej. un motor de juegos o un nodo distribuido), usa `t81::vm::IVirtualMachine`.

```cpp
#include <t81/vm/vm.hpp>
#include <t81/isa/program.hpp>

int main() {
    // 1. Crear VM
    auto vm = t81::vm::make_interpreter_vm();

    // 2. Cargar Programa
    auto prog = t81::tisc::load_program("main.tisc");
    vm->load_program(prog);

    // 3. Configurar Política
    // (Opcional: Adjuntar hooks personalizados de Axion)

    // 4. Ejecutar
    auto result = vm->run_to_halt();
    if (!result) {
        std::cerr << "Atrapado: " << result.error().reason << "\n";
    }
}
```

## 6.3 Embebiendo T81 (API Python)

**Estado: Implementado**

Los bindings de Python permiten controlar la T81VM desde Python, principalmente para pruebas y orquestación.

```python
import t81

# Crear VM
vm = t81.VirtualMachine()

# Cargar Código
vm.load_source("""
let x = 10;
let y = 20;
x + y;
""")

# Ejecutar
result = vm.run()
print(f"Resultado: {result}")
```

## 6.4 Depuración

**Estado: Implementado**

El comando `t81 debug` lanza un depurador estilo GDB para TISC.

*   `step` / `s`: Paso de una instrucción.
*   `next` / `n`: Paso sobre llamada.
*   `reg`: Volcar registros.
*   `stack`: Volcar pila.
*   `trace`: Mostrar historial de traza reciente.

```bash
t81 debug main.tisc
(t81-gdb) break 10
(t81-gdb) run
Breakpoint en PC=10
(t81-gdb) reg r1
r1 = 42 (Int)
```

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
