# Capítulo 11: Apéndices

## 11.1 Lo Que Aún No Está Implementado

**Estado: Seguimiento**

Aunque el T81VM central y la ISA TISC son estables, varias características avanzadas permanecen en fases de desarrollo activo o investigación.

### 11.1.1 Fase 2: Cobertura Completa de `dmath`
Actualmente, `dmath` proporciona implementaciones deterministas para aritmética básica (`+`, `-`, `*`, `/`) y trascendentales clave (`sin`, `cos`, `exp`, `log`).
*   **Faltante**: Las funciones trigonométricas inversas (`asin`, `acos`, `atan`) dependen actualmente de la `libc` del host (a menos que se establezca `T81_DETERMINISTIC`, lo que provoca una trampa).
*   **Faltante**: Las funciones hiperbólicas (`sinh`, `cosh`, `tanh`) son parciales.
*   **Plan**: Implementar expansiones completas de series de Taylor/Maclaurin para todas las funciones matemáticas estándar en `include/t81/core/detail/dmath.hpp` para eliminar todas las dependencias de `libm`.

### 11.1.2 Fase 3: Consenso Distribuido (Nivel 4)
Los opcodes de Nivel 4 (`Gossip`, `Merge`) están especificados pero la pila de red P2P subyacente es experimental.
*   **Faltante**: Descubrimiento de pares robusto (DHT).
*   **Faltante**: Mecanismo de resistencia Sybil (marcador de posición de Prueba de Trabajo/Participación).
*   **Plan**: Integrar una capa de red direccionable por contenido (ej. libp2p o Kademlia personalizado) para soportar la fusión de estados descentralizada.

### 11.1.3 Fase 4: Formas Infinitas Completas (Nivel 5)
El Nivel 5 soporta el colapso básico de Series Geométricas.
*   **Faltante**: Continuación analítica general para series no geométricas.
*   **Faltante**: Suma simbólica de funciones generadoras más complejas.
*   **Plan**: Expandir `InfCollapse` para manejar una clase más amplia de funciones meromorfas.

## 11.2 Glosario

| Término | Definición |
| :--- | :--- |
| **Axion** | El kernel de seguridad de T81, responsable del cumplimiento de políticas y registro de auditoría. |
| **CanonRef** | Una referencia canónica (hash SHA3-256) que apunta a un objeto inmutable en CanonFS. |
| **Nivel Cognitivo** | Un nivel de capacidad computacional (1=Simbólico a 5=Infinito). |
| **Puerta de Determinismo** | El proceso de CI (`t81lang_repro_gate`) que verifica la reproducibilidad exacta a nivel de bit del compilador. |
| **dmath** | Biblioteca de Matemática Determinista; una implementación de software de aritmética de punto flotante. |
| **T81Float** | Un número de punto flotante ternario balanceado $(s, m, e)$. |
| **T81Int** | Un entero ternario balanceado de precisión arbitraria. |
| **TISC** | Computadora con Conjunto de Instrucciones Ternarias; el lenguaje de bytecode de la T81VM. |
| **Trit** | Un dígito base-3 $\{-1, 0, 1\}$. |
| **Tryte** | Una secuencia de trits (generalmente 4). |
| **Honestidad Estructural** | El principio de que un sistema no debe sintetizar información ni ocultar aproximaciones. |

## 11.3 Enlaces Útiles

*   **Repositorio**: [github.com/t81-foundation/t81](https://github.com/t81-foundation/t81)
*   **Especificación**: Directorio `spec/` en el repositorio.
*   **Panel de CI**: Pestaña de Acciones de GitHub.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
