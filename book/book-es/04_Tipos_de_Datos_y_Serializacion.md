# Capítulo 4: Tipos de Datos y Serialización

## 4.1 Tipos Primitivos

**Estado: Implementado y Probado**

La arquitectura T81 está construida sobre una base de primitivas ternarias balanceadas. Estos tipos están diseñados para ser simulados eficientemente en hardware binario mientras mantienen las propiedades matemáticas de la lógica base-3.

### 4.1.1 Trits y Trytes

*   **Trit**: El átomo fundamental de información, tomando valores $\{-1, 0, 1\}$.
    *   También denotado como $\{T, 0, 1\}$ o $\{-, 0, +\}$.
    *   Contenido de información: $\log_2 3 \approx 1.58$ bits.
*   **Tryte**: Una secuencia de trits. El equivalente estándar del "Byte" es el Tryte de 4 trits ($3^4 = 81$ valores).
*   **Palabra (Word)**: Una palabra de máquina T81 estándar es de 27 trits ($3^{27} \approx 7.6 \times 10^{12}$), que cabe cómodamente dentro de un entero de 64 bits.

**Representación Empaquetada (T3_K)**:
Para un almacenamiento eficiente, los trits a menudo se empaquetan utilizando un código de 2 bits:
*   `00` $\to$ 0
*   `01` $\to$ 1
*   `10` $\to$ -1 (T)
*   `11` $\to$ No utilizado / Relleno

### 4.1.2 T81Int (Entero de Precisión Arbitraria)
`T81Int` es un tipo entero de ancho variable. A diferencia de los enteros binarios que usan Complemento a Dos para valores negativos, los enteros ternarios balanceados son simétricos.

*   **Rango**: Intervalo simétrico $[-\frac{3^N-1}{2}, +\frac{3^N-1}{2}]$.
*   **Normalización**: Los ceros a la izquierda están estrictamente prohibidos en la forma serializada canónica. La única representación válida para el Cero es un solo trit `0`.

## 4.2 T81Float y dmath

**Estado: Implementado (Core)**

La aritmética de punto flotante es la fuente principal de no determinismo en la computación multiplataforma. T81 reemplaza los flotantes de hardware IEEE-754 con un formato totalmente definido por software: **`T81Float`**.

### 4.2.1 Definición Canónica
Un `T81Float<M, E>` es una tupla $(s, m, e)$ representando el valor:
$$
V = s \times m \times 3^{e - \text{bias}}
$$
donde:
*   $s \in \{-1, 1\}$ es el signo (almacenado como un trit).
*   $m$ es la mantisa, un entero de $M$-trits normalizado tal que el trit más significativo sea distinto de cero (a menos que $V=0$).
*   $e$ es el exponente, un entero de $E$-trits.
*   $\text{bias} = \frac{3^E - 1}{2}$.

**Valores Especiales**:
*   **Cero**: $e = 0, m = 0$.
*   **Infinito**: $e = e_{\max}, m = 0$.
*   **NaE (No es una Entidad)**: $e = e_{\max}, m \neq 0$. (Equivalente a NaN).

### 4.2.2 El Backend dmath
Para garantizar resultados exactos a nivel de bit en x86, ARM y RISC-V, T81 implementa **`dmath`** (Matemática Determinista).
*   **Aritmética**: `Add`, `Sub`, `Mul`, `Div` se implementan utilizando matemáticas enteras en las mantisas, con reglas de redondeo precisas (redondeo a pares) aplicadas en software.
*   **Trascendentales**: Funciones como `sin`, `cos`, `exp`, `log` se calculan utilizando **expansiones de Series de Taylor** con un número fijo de iteraciones y precisión constante fija. Esto elimina la dependencia de la `libm` del sistema operativo host, que varía entre glibc, musl y MSVC.

## 4.3 Tensores y Diseños Canónicos

**Estado: Implementado y Probado**

Los tensores (`T81Tensor`) son los caballos de batalla de los niveles cognitivos. Para soportar una ejecución eficiente y un hashing canónico, siguen un diseño estricto.

### 4.3.1 Diseño de Memoria
Los tensores se almacenan en orden **Row-Major** (Fila Principal, estilo C), no Column-Major (estilo Fortran).
*   **Forma**: Un vector de dimensiones $(d_0, d_1, \dots, d_n)$.
*   **Stride (Paso)**: Calculado como $s_i = \prod_{j=i+1}^n d_j$.
*   **Alineación**: Los datos del tensor están alineados a límites de 64 bytes en el segmento de memoria `Tensor` para facilitar la carga SIMD (AVX-512 / NEON) donde sea seguro.

### 4.3.2 Serialización (.t81w)
El formato `.t81w` (Pesos T81) es el contenedor estándar para persistir modelos de tensores. Está diseñado para ser **amigable con mmap** y **canónico**.

**Estructura Binaria (Versión 2)**:
1.  **Encabezado Mágico**: `0x54383157` ("T81W").
2.  **Versión**: `0x02`.
3.  **Tabla de Contenidos**: Una lista de tuplas `(Hash, Offset, Longitud)`, ordenada por Hash.
4.  **Datos Blob**: Datos contiguos del tensor, rellenados a una alineación de 64 bytes.

### 4.3.3 Cuantización (T3_K)
T81 soporta un formato de cuantización ternaria nativa llamado **T3_K**.
*   **Tamaño de Bloque**: $K$ trits (típicamente 64 o 128).
*   **Representación**: Cada valor se cuantiza a $\{-1, 0, 1\}$.
*   **Escalado**: Cada bloque tiene un factor de escala (T81Float) para aproximar la magnitud original.

## 4.4 Reglas de Serialización Canónica

**Estado: Implementado**

Para asegurar un hashing consistente (para `CanonRef`), todos los datos deben normalizarse antes de la serialización. El serializador impone un **Mapeo Biyectivo** entre valores abstractos y secuencias de bytes.

1.  **Enteros (T81Int)**:
    *   Eliminar ceros a la izquierda.
    *   El cero se codifica como un solo byte `0x00` (asumiendo codificación específica).
2.  **Floats (T81Float)**:
    *   Deben estar normalizados (desplazamiento máximo a la izquierda).
    *   El Cero Negativo está estrictamente prohibido; debe convertirse a Cero Positivo.
    *   Las cargas útiles NaE se ponen a cero (sin diferenciación de bit "señalización" vs "silencioso").
3.  **Colecciones**:
    *   **Mapas/Diccionarios**: Las claves deben ordenarse lexicográficamente por su representación binaria canónica.
    *   **Conjuntos**: Los elementos deben estar ordenados.
4.  **Grafos**:
    *   Los nodos se re-indexan por una clasificación topológica canónica. Si el grafo tiene ciclos, se aplica una regla de desempate determinista (basada en pesos de aristas).

> **Verificación**: `tests/cpp/test_property_invariants.cpp` verifica estas propiedades a través de pruebas basadas en propiedades (fuzzing), asegurando que $Serialize(Deserialize(Serialize(X))) \equiv Serialize(X)$.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
