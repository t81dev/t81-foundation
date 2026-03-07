# Capítulo 13: Modelado Adversarial y Ataques de Determinismo

## 13.1 Modelo de Amenazas

**Estado: Teórico**

T81 asume un entorno hostil. El **Host** (SO, Hardware, Operador) se considera un adversario que puede intentar:
1.  **Introducir Entropía**: Inyectar aleatoriedad en la ejecución determinista.
2.  **Falsificar Estados**: Afirmar que una computación alcanzó el estado $S'$ cuando en realidad alcanzó $S$.
3.  **Denegar Servicio**: Consumir recursos infinitos.
4.  **Filtrar Información**: Exponer datos privados a través de canales laterales.

## 13.2 Ataques a Nivel de Compilador

**Vector de Ataque**: "Fuente Troyana" / Homoglifos.
**Descripción**: Un atacante usa caracteres de control Unicode (ej. Right-to-Left Override) para hacer que el código fuente parezca diferente a los humanos que al compilador.
**Mitigación**: El Lexer T81 impone un subconjunto estricto de UTF-8. Los caracteres de control y no imprimibles se rechazan durante la tokenización.

**Vector de Ataque**: Reordenamiento de Tokens / Deriva de Optimización.
**Descripción**: Un compilador malicioso podría reordenar instrucciones de una manera que preserve la semántica en una arquitectura pero no en otra (ej. debido a diferencias en el modelo de memoria).
**Mitigación**: El Compilador T81 emite un **AST Canónico**. La fase de generación de IR es determinista y agnóstica de la plataforma. El `t81lang_repro_gate` verifica que la salida del compilador sea idéntica bit a bit entre ejecuciones.

## 13.3 Vectores de Ataque de VM y GC

**Vector de Ataque**: Rowhammer / Cambios de Bit.
**Descripción**: Ataques físicos a DRAM para cambiar bits en memoria sensible (ej. cambiando un veredicto `Deny` a `Allow`).
**Mitigación**: T81 usa **Manejadores Opacos** y **Segmentación de Memoria**. Las estructuras críticas del kernel se almacenan en páginas aisladas (donde es posible) y se validan mediante sumas de comprobación. Sin embargo, el software no puede mitigar completamente fallas de hardware sin memoria ECC.

**Vector de Ataque**: No Determinismo del Recolector de Basura.
**Descripción**: Si el GC se ejecuta basado en el tiempo del reloj de pared o la presión de memoria, las trazas de ejecución divergirán entre ejecuciones.
**Mitigación**: El GC de T81 es **determinista**. Se activa únicamente por recuentos de asignación (`bytes_allocated > threshold`). Esto asegura que las pausas del GC ocurran en la misma instrucción exacta en cada ejecución.

**Vector de Ataque**: Canales Laterales de Temporización.
**Descripción**: Observar el tiempo que tarda en computar una función (ej. exponenciación modular) para inferir claves secretas.
**Mitigación**: `dmath` apunta a implementaciones de tiempo constante para primitivas criptográficas, pero no se garantiza que la aritmética de propósito general sea de tiempo constante. T81 se enfoca en el determinismo *funcional*, no en el determinismo *temporal* (ciclos constantes).

## 13.4 Ataques a CanonFS y Hash

**Vector de Ataque**: Colisión de Hash / Preimagen.
**Descripción**: Encontrar dos entradas diferentes $A \neq B$ tal que $Hash(A) = Hash(B)$.
**Mitigación**: T81 usa **SHA3-256** (Keccak), que es resistente a ataques de extensión de longitud y ataques de colisión. Las reglas de serialización canónica (ordenar claves, normalizar flotantes) minimizan la superficie de ataque reduciendo el espacio de entrada de objetos válidos.

## 13.5 Ataque de Viaje en el Tiempo de Nivel Distribuido

**Vector de Ataque**: Retención de Estado / Reproducción.
**Descripción**: En el Nivel 4, un nodo computa una transición de estado $S_t \to S_{t+1}$ pero la retiene, liberándola más tarde para invalidar el trabajo de otros nodos (un equivalente a "minería egoísta").
**Mitigación**:
1.  **Marcas de Tiempo Lamport**: Cada transición debe seguir causalmente a la anterior.
2.  **Quórums de Consenso**: Un estado solo se finaliza cuando es firmado por $2/3$ del clúster cognitivo.
3.  **Fusión de Trazas**: Si las ramas divergen, la función de fusión determinista resuelve conflictos basándose en el trabajo computacional total (longitud de la traza).

## 13.6 Plantilla Post-Mortem de Brecha de Determinismo

**Estado: Proceso**

Si se detecta una brecha de determinismo (es decir, `t81lang_repro_gate` falla), se invoca el siguiente procedimiento:

1.  **Aislamiento**: Identificar las entradas divergentes y el índice de instrucción específico donde la traza $A$ difiere de la traza $B$.
2.  **Reproducción**: Crear un caso de reproducción mínima (`repro.t81`).
3.  **Análisis**:
    *   ¿Es un error del compilador? (Verificar volcado AST)
    *   ¿Es un error de la VM? (Verificar implementación `dmath`)
    *   ¿Es un problema de la biblioteca host? (Verificar enlace `libc`)
4.  **Remediación**:
    *   Parchar `dmath` para reemplazar el respaldo del host.
    *   Actualizar `t81lang_repro_gate` con la nueva prueba de regresión.
5.  **Divulgación**: Publicar un "Aviso de Determinismo" (si los sistemas de producción se ven afectados).

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
