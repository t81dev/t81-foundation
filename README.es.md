# T81 Foundation

[English](README.md) · [简体中文](README.zh-CN.md) · [Español](README.es.md) · [Português (Brasil)](README.pt-BR.md) · [Русский](README.ru.md)

### Investigando representaciones alternativas, ejecución adaptativa y computación verificable.

**Ternario equilibrado · IA/ML consciente de la representación · Ejecución determinista · Arquitectura experimental**

[Carta de Investigación](docs/research/RESEARCH_CHARTER.md) · [Hoja de Ruta](docs/ROADMAP.md) · [Arquitectura](docs/explanation/ARCHITECTURE.md) · [Contexto de Investigación en IA](docs/ai-context/) · [Registro de superficie de determinismo](docs/governance/DETERMINISM_SURFACE_REGISTRY.md)

---

## ¿Qué es T81?

**T81 Foundation es una arquitectura informática experimental implementada y un programa de investigación científica de código abierto que investiga si las representaciones computacionales alternativas —en particular el ternario equilibrado— pueden ofrecer ventajas medibles sobre los sistemas binarios convencionales.**

T81 es una **arquitectura implementada y funcional**: define el Computador de Conjunto de Instrucciones Ternarias (TISC, por sus siglas en inglés) como su contrato de instrucciones computacionales, con una ejecución acotada entre perfiles demostrada tanto en una máquina virtual alojada (T81VM) como en un entorno de micronúcleo aislado *bare-metal* (Axion OS / TernaryOS en AArch64 QEMU).

El proyecto estudia la computación en múltiples dominios: representación matemática, algoritmos, ejecución de software, almacenamiento, arquitectura de hardware y comportamiento verificable del sistema. Su arquitectura orientada a la investigación se originó en las matemáticas discretas, CWEB/Hanoi y la investigación de sistemas numéricos alternativos antes de expandirse a los subsistemas de lenguajes, máquinas virtuales, instrucciones, políticas y garantía de investigación.

Una dirección secundaria, explícitamente experimental, es la **ejecución adaptativa consciente de la representación**: seleccionar representaciones de datos, algoritmos y rutas de ejecución para cargas de trabajo particulares, especialmente en IA/ML, midiendo al mismo tiempo los costos completos de selección, conversión, computación y precisión. Esta es una hipótesis arquitectónica, **no** una ventaja de rendimiento establecida de T81.

Su pregunta central es:

> ¿Bajo qué condiciones la computación ternaria puede superar, simplificar o mejorar de otro modo un enfoque binario equivalente?

La respuesta no se da por sentada.

Algunos experimentos han demostrado propiedades matemáticas y de representación útiles. Otros han demostrado que las implementaciones binarias convencionales siguen siendo más rápidas o más eficientes. Varias preguntas fundamentales permanecen sin resolver.

T81 existe para investigar esos límites de manera sistemática.

**Esta es una plataforma de investigación experimental, no una afirmación de que ya se haya construido una computadora ternaria superior.**

---

## ¿Por qué ternario?

La computación moderna utiliza predominantemente estados binarios: 0 y 1.

La computación ternaria utiliza tres estados. En el ternario equilibrado, estos se representan como:

\[
T = \{-1,0,+1\}
\]

Esto crea posibilidades distintivas para la aritmética con signo, la representación numérica, la codificación de estados, los sistemas de decisión y la arquitectura computacional.

Por ejemplo:

| Propiedad | Relación matemática |
|---|---|
| Un trit | Tres estados posibles |
| Cuatro trits | \(3^4 = 81\) estados |
| Seis trits | \(3^6 = 729\) estados |
| Información por trit | \(\log_2 3 \approx 1.585\) bits |
| Dígito base-81 | Cuatro trits agrupados |

Estas relaciones son hechos matemáticos.

**No** demuestran que los procesadores ternarios sean más rápidos, que la memoria ternaria sea físicamente más densa o que los algoritmos ternarios consuman menos energía.

Esas son cuestiones de ingeniería independientes que requieren comparaciones rigurosas y emparejadas frente a alternativas binarias optimizadas.

T81 investiga la distinción.

---

## ¿Qué hemos aprendido?

La investigación es valiosa cuando identifica tanto oportunidades como limitaciones.

**Resultado principal — una comparación desfavorable frente a binario optimizado:** En el experimento histórico de producto matriz-vector con AVX2 detallado a continuación, INT8 nativo alcanzó **17.15 GFLOP/s**, frente a **12.91 GFLOP/s** para el ternario empaquetado de 2 bits (aproximadamente un **32.8% más de rendimiento** para la implementación nativa INT8). Este es un resultado específico de una carga de trabajo, no una clasificación universal de bases; T81 lo publica de forma destacada porque un resultado desfavorable es evidencia, no un defecto a ocultar.

La historia experimental de T81 ha establecido varios hallazgos importantes.

### 1. Resultado negativo en AVX2: la densidad de representación no es velocidad de ejecución

Los datos ternarios empaquetados pueden reducir los requisitos de almacenamiento.

Sin embargo, el software aún debe decodificar, manipular y ejecutar esas representaciones en hardware binario convencional.

En un experimento histórico de matriz-vector AVX2:

| Implementación | Rendimiento reportado |
|---|---:|
| INT8 nativo AVX2 | 17.15 GFLOP/s |
| Ternario empaquetado de 2 bits AVX2 | 12.91 GFLOP/s |

La representación ternaria compacta no superó a la implementación convencional de INT8 en esa carga de trabajo.

Este es un resultado experimental delimitado, no una prueba de que la computación ternaria sea universalmente más lenta.

[Explorar la investigación sobre ternario empaquetado](docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md)

### 2. Las representaciones adaptativas tienen ventajas condicionales

La investigación sobre niveles de representación adaptativa ha identificado configuraciones en las que una representación ternaria tiene un costo modelado menor que una alternativa binaria emparejada.

Esas ventajas dependen de supuestos de carga de trabajo, sobrecarga de metadatos, costos de conversión y límites de representación.

La investigación corregida H04-R02 preserva estos hallazgos condicionales mientras deja explícitamente sin resolver el rendimiento físico.

[Leer los hallazgos corregidos de H04-R02](experiments/h04_r02_adaptive_representation/RESULTS.md)

### 3. La corrección no establece optimización

La investigación experimental T81-AIOPT-001 examinó múltiples representaciones del emparejamiento de patrones de cuatro trits.

Su implementación M1 superó extensas pruebas de corrección locales, pero la carga de trabajo experimental podía reducirse a una simple comparación ordinal.

El resultado demuestra por qué una prueba de corrección exhaustiva aún puede ser insuficiente para evaluar una ventaja computacional.

La pregunta relevante no es simplemente si otra representación funciona, sino si la carga de trabajo elegida puede distinguir diferencias significativas entre algoritmos competidores.

[Explorar T81-AIOPT-001](docs/research/aiopt/)

### 4. Demostración de ejecución acotada entre perfiles (CPA-004/CPA-005)

**Clasificación de evidencia: determinismo y portabilidad acotados, no rendimiento comparativo ni madurez de todo el sistema.** Las garantías de determinismo están estrictamente acotadas por el [Registro de superficie de determinismo](docs/governance/DETERMINISM_SURFACE_REGISTRY.md).

Investigaciones recientes en el programa de Arquitectura Entre Perfiles (CPA) demostraron que el núcleo computacional de T81 se ejecuta de manera idéntica en entornos de ejecución divergentes:

- **Artefacto de Programa Canónico:** Un programa canónico congelado de 351 bytes que comprende 27 registros canónicos de 13 bytes (SHA-256 `b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b`) ejecutó un subconjunto admitido de 17 instrucciones TISC.
- **Equivalencia entre Perfiles:** Ejecuciones limpias en tres procesos T81VM alojados y tres arranques bare-metal limpios en AArch64 QEMU (tras `ExitBootServices` de UEFI) coincidieron con un oráculo matemático derivado de forma independiente en **los 25 límites de observación** (estado inicial + 24 pasos de instrucción).
- **Coincidencia Integral de Límites:** Se verificó la equivalencia en los 81 valores y etiquetas de registros, 1,563 palabras y etiquetas de memoria lógica, banderas y registros de sistema (R75–R80), deteniéndose deterministamente en PC=27, SP=283.
- **Distinción:** Demostrar la corrección de ejecución entre perfiles prueba que la arquitectura computacional funciona según lo especificado; **no** es evidencia de superioridad ternaria en rendimiento, energía o hardware sobre las líneas base binarias.

### 5. Programa de investigación activo SCG-08: propagación de acarreo y trabajo aritmético

El programa **SCG-08** investiga la propagación de acarreo en ternario equilibrado y el trabajo aritmético de precisión múltiple bajo una métrica de costo ponderado congelada:

- **Carta de Investigación Fase 0:** Aceptada e integrada (`SCG08_PHASE0_RESEARCH_CHARTER.md`).
- **Banco de Pruebas Aritmético Fase 1 Puerta 1:** Implementado y verificado en el entorno aislado `experiments/scg08_arithmetic_work/`. Establece corrección matemática a través de 5 anchos de bits de operando estandarizados (64, 128, 256, 512, 1024 bits) frente a un oráculo entero exacto desacoplado de precisión arbitraria en base $10^9$, reproducción determinista, contabilidad exacta de operaciones primitivas y cumplimiento estricto del presupuesto de ensayos ($\le 1,000$ ensayos; 179 ejecutados en el conjunto smoke).
- **Estado de la Puerta 2 de Benchmarking:** **ESTRICTAMENTE RETENIDO.** Las campañas empíricas a gran escala y las evaluaciones de hipótesis estadísticas permanecen no autorizadas hasta una revisión explícita y autorización de la gobernanza. El código integrado del arnés no constituye autorización experimental.

### 6. La computación física ternaria sigue siendo una pregunta abierta

La computación ternaria emulada por software en CPUs binarias no equivale a computación en hardware ternario nativo.

Evaluar dispositivos físicos requiere evidencia emparejada de área, energía, latencia, fiabilidad, detección, codificación y circuitería periférica.

La evaluación física permanece bloqueada debido a que no se dispone de datos de caracterización de dispositivos emparejados adecuados (Investigación 10A). T81 no interpreta la falta de evidencia como prueba de imposibilidad, ni considera la eficiencia teórica de base como prueba de ventaja física.

---

## La arquitectura de investigación de T81

El repositorio contiene varios subsistemas desarrollados para investigar representaciones computacionales y ejecución controlada.

```text
                     T81 FOUNDATION
                Arquitectura Implementada e Investigación
                             |
          +------------------+------------------+
          |                  |                  |
    REPRESENTACIÓN       EJECUCIÓN          GARANTÍA
          |                  |                  |
    Ternario equilibrado TISC ISA (Contrato)  Axion Policy
    Base-81 / Base-243   +-- T81VM (Alojada)  CanonFS
    Datos empaquetados   +-- Axion (Baremetal)Evidencia
    Niveles adaptativos  T81Lang Frontend     Procedencia
          |                  |                  |
          +------------------+------------------+
                             |
                   EXPERIMENTOS CONTROLADOS
                             |
               Líneas base binarias convencionales
                             |
                  Verificación y comparación
                             |
                   Conclusiones de investigación
```

### Contrato Computacional y Perfiles de Ejecución

En la arquitectura T81:
- **TISC (Ternary Instruction Set Computer):** Sirve como contrato de instrucciones computacionales central, definiendo la semántica de opcodes, vectores de estado de 64 bits y reglas de transición deterministas.
- **T81VM Alojada:** Perfil de ejecución alojado para desarrollo en espacio de usuario, herramientas, pruebas y reproducción determinista en sistemas operativos anfitriones convencionales (Linux, macOS, Windows).
- **Axion OS / TernaryOS Aislado:** Perfil de ejecución *bare-metal* que se ejecuta directamente sobre hardware o hipervisores (p. ej., AArch64 QEMU), ejecutando TISC sin dependencias de un SO anfitrión.
- **CanonFS:** Proporciona identidad de objetos dirigida por contenido, almacenamiento inmutable de artefactos, procedencia criptográfica y contratos de intercambio RFC-00D1.
- **DAIOS (Deterministic AI OS):** Permanece como un concepto arquitectónico prospectivo y propuesta de arquitectura objetivo (RFC-00D2) para la gobernanza en tiempo de ejecución de agentes autónomos e IA de alto nivel, independiente del motor de ejecución TISC implementado.

### Clasificaciones de Madurez de Subsistemas

El proyecto distingue diferentes clasificaciones de madurez en sus subsistemas. Una implementación funcional no implica igual madurez ni ventajas validadas:

| Componente | Madurez | Rol |
|---|---|---|
| **TISC** | Frozen | Contrato de instrucciones computacionales que rige la semántica de ejecución (v1.9.0) |
| **T81VM** | Stable | Perfil de ejecución alojado para ejecución en máquina virtual y determinismo (acotado por el [Registro de superficie de determinismo](docs/governance/DETERMINISM_SURFACE_REGISTRY.md)) |
| **Axion Kernel** | Stable | Perfil de ejecución independiente (*bare-metal* AArch64 microkernel/HAL) y autorización de políticas previa a efectos secundarios |
| **T81Lang** | Stable | Frontend de lenguaje e infraestructura de compilación con destino a TISC |
| **T81 Core** | Stable | Tipos numéricos, contenedores y primitivas de representación |
| **CanonFS** | Stable | Artefactos direccionados por contenido y mecanismos de integridad |
| **DAIOS** | Concept | Propuesta de arquitectura objetivo para sistemas operativos de IA deterministas (RFC-00D2) |
| **Research experiments** | Experimental | Prueba de hipótesis, validación de corrección y mediciones comparativas |

---

## Primeros Pasos

### Requisitos Previos

La compilación de T81 requiere:
- **CMake** 3.24 o superior
- **Compilador compatible con C++20** (GCC 12+, Clang 15+, o MSVC 2022)
- **Python** 3.10+ (para scripts de gobernanza, validación y pruebas)
- *Opcional:* Sistema de compilación Ninja, QEMU AArch64 (para objetivos bare-metal)

### Compilación del Proyecto

Configurar y compilar el proyecto principal:

```bash
# Configurar compilación fuera del árbol de fuentes
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Compilar objetivos principales
cmake --build build --target t81 -j$(nproc)

# Ejecutar el conjunto estándar de pruebas
ctest --test-dir build --output-on-failure
```

### Ejecución de Experimentos (Opcional)

Los experimentos de investigación y arneses (como SCG-08) están protegidos tras banderas de CMake opcionales:

```bash
# Configurar con arneses experimentales habilitados
cmake -B build -S . -DT81_BUILD_EXPERIMENTS=ON -DCMAKE_BUILD_TYPE=Release

# Compilar y ejecutar la validación aritmética de SCG-08 Puerta 1
cmake --build build --target t81_exp_scg08_01_test
ctest --test-dir build -R scg08 --output-on-failure

# Validar el esquema de evidencia
python3 scripts/ci/validate_scg08_gate1_evidence.py
```

---

## Metodología de investigación

T81 sigue un enfoque basado en la evidencia.

Cada experimento comparativo debe responder cinco preguntas:

1. **¿Cuál es la hipótesis?** Definir una proposición falsable.
2. **¿Cuál es la línea base?** Comparar con una alternativa convencional funcionalmente equivalente y adecuadamente optimizada.
3. **¿Qué se mide?** Separar corrección, almacenamiento, latencia, rendimiento, energía y tráfico de memoria.
4. **¿Cuáles son las limitaciones?** Identificar supuestos, hardware anfitrión, alcance de la carga de trabajo y dependencias sin resolver.
5. **¿Qué conclusión está justificada?** Distinguir resultados respaldados, no respaldados, no concluyentes, bloqueados y no probados.

El proyecto distingue estrictamente entre:

**Implementado → Probado → Medido → Comparativamente establecido**

Estos son diferentes niveles probatorios, no etiquetas intercambiables. Un resultado experimental no debe afirmar más de lo que respaldan sus pruebas.

---

## Navegación por la documentación

| Directorio / Archivo | Descripción |
|---|---|
| [`docs/research/`](docs/research/) | Investigaciones científicas primarias y conclusiones experimentales |
| [`experiments/`](experiments/) | Implementaciones experimentales acotadas, bancos de prueba y registros de evidencia |
| [`spec/`](spec/) | Especificaciones de conjuntos de instrucciones y catálogo de RFCs |
| [`docs/governance/`](docs/governance/) | Registros de superficie de gobernanza, modelo de autoridad y [Registro de superficie de determinismo](docs/governance/DETERMINISM_SURFACE_REGISTRY.md) |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | Hoja de ruta para colaboradores y secuenciación activa de investigación |
| [`docs/ai-context/`](docs/ai-context/) | Documentación de continuidad fidedigna y revisiones de estado para colaboradores de IA |
| [`include/t81/`](include/t81/) | Encabezados C++ públicos y contratos del núcleo |
| [`vm/`](vm/) | Implementación de la máquina virtual alojada (T81VM) |
| [`kernel/axion/`](kernel/axion/) | Motor de políticas de Axion y núcleo de gobernanza |
| [`fs/`](fs/) | Controlador de almacenamiento e intercambio dirigido por contenido CanonFS |
| [`PROJECT_STRUCTURE.md`](PROJECT_STRUCTURE.md) | Mapa completo de directorios y estructura del repositorio |
| [`SUPPORT_TAXONOMY.md`](SUPPORT_TAXONOMY.md) | Taxonomía de niveles de soporte, límites de plataformas y perfiles |

---

## Lo que T81 no afirma

T81 no afirma:

- Superioridad universal de la computación ternaria sobre la computación binaria.
- Superioridad demostrada o viabilidad comercial del silicio ternario nativo.
- Una ventaja general de rendimiento para la arquitectura integrada T81 sobre entornos convencionales optimizados.
- Un optimizador de IA adaptativo validado que supere a líneas base sólidas de política fija o de enrutamiento algorítmico.
- Que las cifras de casos de estudio de los PDF históricos constituyan mediciones reproducidas de manera independiente.
- Que los pesos de modelos ternarios aceleren intrínsecamente la inferencia de IA.
- Que la ejecución determinista o la aplicación de políticas requieran una máquina virtual ternaria.
- Que un experimento generado por IA esté validado independientemente solo porque pasen sus pruebas locales.
- Que el código del arnés integrado o la calificación de Puerta 1 autoricen el benchmarking de Fase 2 sin aprobación explícita de gobernanza.

Estos límites son esenciales para la integridad científica de la investigación.

---

## La pregunta sigue abierta

La computación binaria domina la infraestructura física y de software actual por razones de ingeniería convincentes.

Ese dominio no elimina el valor de investigar representaciones alternativas, modelos computacionales o futuros sustratos de hardware.

El propósito de T81 es determinar dónde son útiles esas alternativas y dónde no lo son.

**No estamos construyendo una conclusión para buscar evidencia que la respalde.**

**Estamos construyendo la base experimental necesaria para descubrir la conclusión.**

---

**T81 Foundation** — Arquitectura Informática Experimental e Investigación de Sistemas

[Licencia: Apache 2.0](LICENSE) · [GitHub](https://github.com/t81dev/t81-foundation)
