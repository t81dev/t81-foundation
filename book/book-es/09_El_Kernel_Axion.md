# Capítulo 9: El Kernel de Seguridad Axion

## 9.1 Definición Formal

**Estado: Implementado y Probado**

El **Kernel Axion** es el guardián del runtime T81. A diferencia de los sistemas operativos tradicionales donde la seguridad se impone en el límite de la llamada al sistema (cambio de modo Usuario/Kernel), Axion impone la seguridad a **nivel de instrucción**.

Formalmente, el Kernel Axion es una función $\alpha$ que mapea el estado actual de la máquina $S$ y la operación propuesta $Op$ a un veredicto $V$:
$$
\alpha: (S, Op) \to \{\text{Allow}, \text{Deny}, \text{Warn}, \text{Defer}\}
$$

Esta evaluación ocurre **antes** de que ocurra la transición de estado $S \xrightarrow{Op} S'$. Si $\alpha(S, Op) = \text{Deny}$, la transición se aborta y la máquina se detiene con un `SecurityFault`.

## 9.2 El Modelo de Políticas

**Estado: Implementado**

Las políticas son conjuntos de reglas declarativas que definen las restricciones para un contexto de ejecución específico. Una política no dice *qué* computar, sino *cómo* se permite computar.

### 9.2.1 Lenguaje de Política (Expresiones-S)
Las políticas de Axion se definen utilizando una sintaxis de expresión-S similar a Lisp, asegurando un análisis y canonicalización fáciles.

**Ejemplo: Una Política Estricta de Nivel 1**
```lisp
(policy
  (tier 1)                  ; Restringir a Nivel Simbólico (Sin recursión, sin reflexión)
  (max-instructions 10000)  ; Límite duro de gas
  (max-stack 256)           ; Límite de profundidad de pila
  (max-tensors 0)           ; No se permiten asignaciones de tensores
  (allowed-tensor-hashes []) ; No se permiten pesos externos
)
```

**Ejemplo: Una Política de Inferencia de IA de Nivel 3**
```lisp
(policy
  (tier 3)
  (max-recursion 1024)
  (max-tensors 50)
  (max-tensor-elements 1000000)
  (allowed-tensor-hashes [
    "canon:sha3:a7f..." ; Pesos de modelo permitidos específicos
  ])
)
```

### 9.2.2 Capacidades
Las capacidades son permisos granulares otorgados a un proceso.
*   **NetAccess**: Capacidad de usar manejadores `IoNet` (Nivel 4).
*   **MetaWrite**: Capacidad de modificar el segmento Meta (Reflexión).
*   **InfExpand**: Capacidad de instanciar formas infinitas (Nivel 5).

## 9.3 Intercepción de Instrucciones

**Estado: Implementado y Probado**

El Kernel Axion está integrado directamente en el bucle de obtención-decodificación-ejecución de la VM.

### 9.3.1 El Gancho Interceptor
En `src/vm/vm.cpp`, el bucle principal invoca al motor de políticas:

```cpp
// Pseudocódigo del Bucle del Intérprete
while (!halted) {
    Opcode op = fetch();

    // 1. Verificación Axion
    Verdict v = axion->evaluate(ctx);
    if (v == Verdict::Deny) {
        throw SecurityFault(v.reason);
    }

    // 2. Ejecución
    execute(op);

    // 3. Registro de Auditoría
    if (v == Verdict::Warn || policy.audit_all) {
        trace.log(op, v, state_hash);
    }
}
```

### 9.3.2 ¿Abstracciones de Costo Cero?
No. T81 rechaza explícitamente las "Abstracciones de Costo Cero" si comprometen la seguridad. La verificación de Axion impone una sobrecarga de rendimiento. Esta es una elección de diseño deliberada: **Corrección > Rendimiento**. Sin embargo, para trazas compiladas por JIT, las comprobaciones de política se realizan una vez durante la grabación de la traza y se integran en la traza optimizada como aserciones protegidas, reduciendo significativamente la sobrecarga en tiempo de ejecución.

## 9.4 El Registro de Auditoría (Traza)

**Estado: Implementado y Probado**

La **Traza** es la prueba criptográfica de lo que sucedió. No es solo un registro de depuración; es una cadena Merkle de eventos.

### 9.4.1 Estructura de la Traza
Cada entrada en el registro contiene:
1.  **Tick**: La hora del reloj lógico.
2.  **Opcode**: La instrucción ejecutada.
3.  **Verdict**: La decisión de Axion.
4.  **StateHash**: Un hash SHA3-256 del estado relevante de la máquina *después* de la operación.

$$
H_{t} = \text{Hash}(H_{t-1} || \text{Op}_t || \text{Verdict}_t || \text{StateDiff}_t)
$$

El hash final $H_n$ es la **Prueba de Ejecución**. Si dos partes ejecutan el mismo código y obtienen el mismo $H_n$, están criptográficamente garantizadas de haber alcanzado exactamente el mismo estado a través exactamente del mismo camino.

## 9.5 Promoción Cognitiva

**Estado: Implementado**

Un programa comienza en un Nivel Cognitivo específico (generalmente Nivel 1). Puede solicitar **Promoción** a un nivel superior para realizar operaciones más complejas.

*   **Solicitud**: El programa ejecuta un opcode `Promote` con un token de capacidad firmado.
*   **Evaluación**: Axion valida el token contra la política.
*   **Resultado**: Si se permite, el `tier_status` de la VM se actualiza, desbloqueando nuevos opcodes (por ejemplo, `Recurse` o `Gossip`).

**Ruta de Escalada de Nivel**:
1.  **Nivel 1**: Seguro, acotado, tiempo polinomial.
2.  **Nivel 2**: Dinámico, reflexivo.
3.  **Nivel 3**: Recursivo, potencial de tiempo exponencial (requiere límites de gas).
4.  **Nivel 4**: No local, dependiente de la red (requiere límites de consenso).
5.  **Nivel 5**: Infinito (requiere contención estricta).

> **Verificación**: `tests/cpp/test_ethics.cpp` verifica que los intentos de usar opcodes de Nivel 3 en una política de Nivel 1 resultan en un `SecurityFault`.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
