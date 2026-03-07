# Capítulo 8: Verificación y Auditoría

## 8.1 Metodología de Verificación Formal

**Estado: Implementado**

T81 emplea una estrategia de verificación de múltiples capas, pasando de pruebas unitarias a la verificación formal de propiedades.

### 8.1.1 La Pila de Verificación

1.  **Pruebas Unitarias (L1)**: Verifican funciones y clases individuales.
    *   Ejemplo: `tests/cpp/test_T81Int.cpp` verifica `T81Int::add(1, 1) == 1T`.
2.  **Pruebas Basadas en Propiedades (L2)**: Generan entradas aleatorias para verificar invariantes (estilo QuickCheck).
    *   Ejemplo: `tests/cpp/test_property_invariants.cpp` verifica $\forall a, b: a + b = b + a$.
3.  **Fuzz Testing (L3)**: Alimentan entradas malformadas al parser y la VM para asegurar la resistencia a fallos.
    *   Ejemplo: `tests/cpp/frontend_fuzz_test.cpp` usa `libFuzzer`.
4.  **Pruebas de Integración (L4)**: Ejecutan programas completos de extremo a extremo y verifican la salida.
    *   Ejemplo: `tests/cpp/e2e_arithmetic_test.cpp`.
5.  **Puertas de Determinismo (L5)**: Verifican la reproducibilidad exacta a nivel de bit en todos los entornos.

## 8.2 La Matriz de Auditoría Formal

**Estado: Auditable**

Esta matriz mapea los requisitos del sistema a sus artefactos de verificación específicos.

| Requisito | ID | Artefacto / Prueba | Estado |
| :--- | :--- | :--- | :--- |
| **Determinismo Estricto** | REQ-001 | `scripts/ci/t81lang_repro_gate.py` | PASS |
| **Aritmética Ternaria** | REQ-002 | `tests/cpp/ternary_arith_test.cpp` | PASS |
| **Cumplimiento de Política** | REQ-003 | `tests/cpp/test_ethics.cpp` | PASS |
| **Seguridad de Memoria** | REQ-004 | `tests/cpp/vm_bounds_test.cpp` | PASS |
| **Integridad de Traza** | REQ-005 | `tests/cpp/axion_log_determinism_test.cpp` | PASS |
| **Hashing Canónico** | REQ-006 | `tests/cpp/canonfs_driver_test.cpp` | PASS |

## 8.3 Pruebas Basadas en Propiedades

**Estado: Implementado**

Usamos **Pruebas Basadas en Propiedades** para probar que las leyes algebraicas se mantienen para nuestros tipos numéricos personalizados.

### 8.3.1 Los Axiomas del Anillo
El archivo `tests/cpp/test_property_invariants.cpp` verifica programáticamente que `T81Int` y `T81Float` satisfacen los axiomas de un Anillo:
1.  **Asociatividad**: $(a + b) + c = a + (b + c)$
2.  **Conmutatividad**: $a + b = b + a$
3.  **Identidad**: $a + 0 = a$
4.  **Inverso**: $a + (-a) = 0$
5.  **Distributividad**: $a \times (b + c) = (a \times b) + (a \times c)$

Estas pruebas ejecutan millones de iteraciones con entradas aleatorias (incluyendo casos extremos como `MaxInt`, `MinInt`, `Zero`) para proporcionar una alta confianza estadística en la corrección.

## 8.4 La Puerta de Determinismo

**Estado: Activo**

La **Puerta de Determinismo** (`scripts/ci/t81lang_repro_gate.py`) es la comprobación final antes de fusionar cualquier código.

### 8.4.1 Mecánica
1.  **Consistencia de Compilación**: Compila un conjunto de 5+ programas de prueba canónicos (`*.t81`) dos veces. Compara la salida binaria del Paso A y el Paso B. Cualquier diferencia de bits causa una falla inmediata.
2.  **Hashing Agregado**: Calcula un hash SHA-256 del bytecode generado para todas las pruebas.
3.  **Verificación de Línea Base**: Este hash agregado se compara contra un "Hash Dorado" registrado. Si la lógica del compilador cambia (resultando en bytecode diferente), el hash variará y la construcción fallará. Esto asegura que los cambios en el compilador sean intencionales y auditados.

### 8.4.2 Trazabilidad
Cuando la puerta pasa, muestra:
```text
[PASS] T81Lang gates passed: fixtures=5 hash=a7f92b...
```
Este hash se registra en los registros de CI, proporcionando un registro inmutable del estado del compilador para esa versión.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
