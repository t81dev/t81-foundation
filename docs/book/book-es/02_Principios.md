# Capítulo 2: Principios Centrales e Invariantes

## 2.1 El Invariante de Determinismo

**Estado: Implementado y Probado**

El axioma central de la arquitectura T81 es el **Determinismo Estricto**. En este sistema, un programa $P$ no es una sugerencia al hardware; es una definición matemática de una función de transición de estado $f$.

Formalmente, dado un estado inicial $S$ y un vector de entrada $I$, la función debe satisfacer:
$$
\forall \text{hardware } H: \text{Exec}_H(S, I) \to S' \implies S' \text{ es invariante}
$$

Lograr esto requiere eliminar todas las fuentes de no determinismo comunes en la computación moderna. T81 trata el entorno host (SO, CPU, FPU) como una "fuente de entropía adversarial" que debe ser restringida.

### 2.1.1 Superficies de Determinismo y Vectores de Ataque

La "Superficie de Determinismo" es el límite donde la máquina abstracta interactúa con la realidad física. Cualquier fuga de realidad física (tiempo, ruido aleatorio, peculiaridades del hardware) hacia el estado lógico constituye una **Brecha de Determinismo**.

| Capa     | Riesgo de Determinismo       | Mitigación                | Evidencia de Implementación |
| :--- | :--- | :--- | :--- |
| **Compilador** | Orden de tokens, iteración de mapas | Ordenación canónica de AST | `src/frontend/ast.cpp` (Mapas Ordenados) |
| **Memoria VM** | Fuga de direcciones de punteros | Manejadores Opacos (Índices) | `src/vm/vm.cpp` (Segmentos de Memoria) |
| **Garbage Collector** | Ciclos de recolección no deterministas | Disparadores por conteo de asignaciones | `src/vm/gc.cpp` (GC basado en instrucciones) |
| **Concurrencia** | Condiciones de carrera, planificación | Corrutinas cooperativas | `src/vm/scheduler.cpp` (Tics Deterministas) |
| **Punto Flotante** | Deriva de FPU del host (IEEE-754) | `dmath` float por software | `include/t81/core/T81Float.hpp` |
| **Trascendentales** | Varianza de implementación de Libm | Series de Taylor (Iter. fijas) | `include/t81/core/detail/dmath.hpp` |
| **JIT** | Divergencia de optimización  | Comprobaciones de Equivalencia de Traza | `src/vm/jit_compiler.cpp` |

### 2.1.2 La "Brecha Libm" y `dmath`
Una vulnerabilidad crítica en el determinismo multiplataforma es la "Brecha Libm". El estándar IEEE-754 define formatos de punto flotante pero deja las funciones trascendentales (sin, cos, pow) vagamente especificadas. Como resultado, `std::sin(x)` en x86_64/GLIBC puede diferir en 1 ULP (Unidad en el Último Lugar) de `std::sin(x)` en ARM64/MUSL.

T81 resuelve esto con **`dmath`** (Matemáticas Deterministas), una biblioteca personalizada que implementa:
*   **Aritmética Soft-Float**: `Add`, `Sub`, `Mul` son exactos a nivel de bit.
*   **Trascendentales Personalizadas**: `Sin`, `Cos`, `Exp` se implementan mediante series de Taylor/Maclaurin con un número fijo de iteraciones y constantes fijas, ignorando la `libm` del host.
*   **Modo de Redondeo**: El redondeo a pares (ties-to-even) se impone en software.

> **Invariante**: $\text{dmath::sin}(x)$ produce el mismo patrón de bits exacto en un Intel i9, un Apple M3 y una placa de desarrollo RISC-V.

## 2.2 Lógica Ternaria (Base-3)

**Estado: Implementado y Probado**

T81 es un sistema **ternario balanceado**. La unidad fundamental es el **trit**, con valores $\{-1, 0, 1\}$ (a menudo denotados como $-, 0, +$ o $T, 0, 1$).

### 2.2.1 ¿Por qué Ternario?
1.  **Aritmética Simétrica**: El rango de valores es simétrico alrededor de cero. En binario (Complemento a Dos), el rango es asimétrico (ej. -128 a +127). En ternario balanceado, un entero de $N$-trits cubre $-\frac{3^N-1}{2} \dots +\frac{3^N-1}{2}$.
2.  **Eficiencia de Redondeo**: Redondear al entero más cercano es equivalente al truncamiento. $0.5$ no es representable exactamente, evitando el "problema del redondeo de 0.5".
3.  **Economía de Base**: La economía de base $E(r, N) = r \lfloor \log_r N \rfloor$ se minimiza cuando $r = e \approx 2.718$. El entero $3$ está más cerca de $e$ que $2$, haciendo que el ternario sea teóricamente más eficiente para la densidad de almacenamiento de información.
4.  **Representación con Signo**: Los números negativos no requieren un bit de signo separado. El signo es llevado por el trit no cero más significativo.

### 2.2.2 Implementación
En el código base de C++, los trits se simulan en hardware binario para eficiencia.
*   **Almacenamiento Empaquetado**: `T81Int` usa un esquema de codificación de 2 bits por trit (00=0, 01=1, 11=-1/T). Esto permite que 4 trits quepan en un byte (un Tryte).
*   **Aritmética**: Las operaciones se implementan usando matemáticas enteras que simulan cadenas de acarreo ternario balanceado.
    *   Ejemplo: $1 + 1 = 1T$ (que es $3 - 1 = 2$).
    *   Ejemplo: $T + T = T1$ (que es $-3 + 1 = -2$).

## 2.3 Auditabilidad y la Traza Axion

**Estado: Implementado y Probado**

El determinismo por sí solo es insuficiente; la ejecución debe ser **auditable**. El Kernel Axion produce un registro criptográfico llamado **Traza**.

### 2.3.1 La Estructura de la Traza
Una traza $\mathcal{T}$ es una secuencia ordenada de eventos $E_0, E_1, \dots, E_k$. Cada evento captura una transición de estado significativa o una verificación de política.

```cpp
struct AxionEvent {
    uint64_t tick;          // Marca de tiempo lógica
    Opcode op;              // La operación intentada
    Verdict verdict;        // La decisión del kernel (Allow/Deny)
    CanonHash81 state_hash; // Raíz de Merkle del estado de la VM
    std::string metadata;   // Información de depuración contextual
};
```

Esta traza sirve como una **Prueba de Ejecución**. Al reproducir la traza contra el estado inicial, un auditor puede probar matemáticamente que la computación produjo el resultado afirmado sin confiar en el hardware que lo produjo.

## 2.4 Los Nueve Principios (Cumplimiento Ético)

**Estado: Implementado y Probado**

T81 integra un conjunto de "Principios Constitucionales" inmutables ($\Theta_1 \dots \Theta_9$) directamente en el motor de políticas de la VM. Estas no son meramente pautas; son restricciones en tiempo de ejecución impuestas por el Kernel Axion.

| Símbolo | Principio | Descripción | Impuesto Por |
| :--- | :--- | :--- | :--- |
| $\Theta_1$ | **No Daño** | Capa de seguridad fundamental; previene corrupción de memoria y segfaults. | Verificaciones de Límites de Memoria |
| $\Theta_2$ | **No Coacción** | Previene transiciones de estado forzadas sin autorización criptográfica. | Verificación de Firmas |
| $\Theta_3$ | **Verdad** | La información debe ser canónica; dos hashes diferentes no pueden mapear al mismo objeto. | Verificaciones de Colisión CanonFS |
| $\Theta_4$ | **Interpretabilidad** | Se advierte contra la ejecución opaca de "caja negra"; la generación de trazas es obligatoria para el Nivel 3+. | Registrador de Trazas |
| $\Theta_5$ | **Integridad de Identidad** | Los nodos distribuidos deben mantener claves de identidad consistentes. | Handshake de Nivel 4 |
| $\Theta_6$ | **Prioridad Ética** | Las políticas de seguridad anulan las optimizaciones de rendimiento. | Preempción de Política |
| $\Theta_7$ | **Contención de Entropía** | Previene la expansión de recursos no acotada (ej. bucles infinitos, fugas de memoria). | Límites de Recursión / Gas |
| $\Theta_8$ | **Consistencia Canónica** | Todos los datos deben normalizarse antes del hashing. | Serializador |
| $\Theta_9$ | **Ejecución Transparente** | El sistema no debe ocultar efectos secundarios; `MetaWrite` requiere permiso explícito de política. | Interceptor Axion |

> **Ejemplo**: Si un programa intenta recurrir infinitamente, viola $\Theta_7$ (Contención de Entropía). El Kernel Axion detecta que `recursion_depth > policy.max_depth` y emite un veredicto `Deny`, convirtiendo la operación en un `Trap::SecurityFault`.

## 2.5 Lista de Verificación

*   [ ] **Consistencia de Float**: ¿Produce `T81Float` patrones de bits idénticos para funciones trascendentales (`sin`, `exp`) en todas las plataformas? (Ejecutar `tests/cpp/test_T81Float.cpp` y `tests/cpp/test_property_float.cpp`)
*   [ ] **Determinismo del GC**: ¿El Recolector de Basura se ejecuta en recuentos exactos de instrucciones (asignaciones), no en tiempo de pared? (Verificar `kGcInterval` en `src/vm/vm.cpp`)
*   [ ] **Integridad de la Traza**: ¿Es inmutable el registro de Axion durante la ejecución? (Verificado por `tests/cpp/axion_log_determinism_test.cpp`)
*   [ ] **Cumplimiento Ético**: ¿Se activan correctamente las comprobaciones $\Theta$ cuando se exceden los límites? (Verificado por `tests/cpp/test_ethics.cpp`)

## 2.6 Matriz de Auditoría Formal

| Principio | Sección de Especificación | Implementación | Cobertura de Pruebas |
| :--- | :--- | :--- | :--- |
| Determinismo Estricto | `spec/determinism-profile.md` | `src/vm/vm.cpp` | `tests/cpp/test_property_invariants.cpp` |
| Lógica Ternaria | `spec/t81-data-types.md` | `include/t81/ternary.hpp` | `tests/cpp/ternary_arith_test.cpp` |
| Auditabilidad | `spec/axion-kernel.md` | `include/t81/axion/api.hpp` | `tests/cpp/test_ethics.cpp` |
| Almacenamiento Canónico | `spec/supplemental/canonfs-spec.md` | `fs/` | `tests/cpp/canonfs_driver_test.cpp` |

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
