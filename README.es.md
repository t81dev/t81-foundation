# T81 Foundation

### Investigando representaciones alternativas, ejecución adaptativa y computación verificable.

**Ternario equilibrado · IA/ML consciente de la representación · Ejecución determinista · Arquitectura experimental**

[Carta de Investigación](https://www.google.com/search?q=docs/research/RESEARCH_CHARTER.md) · [Hoja de Ruta](https://www.google.com/search?q=docs/ROADMAP.md) · [Arquitectura](https://www.google.com/search?q=docs/explanation/ARCHITECTURE.md) · [Contexto de Investigación en IA](https://www.google.com/search?q=docs/ai-context/)

---

## ¿Qué es T81?

**T81 Foundation es una arquitectura informática experimental implementada y un programa de investigación científica de código abierto que investiga si las representaciones computacionales alternativas —en particular el ternario equilibrado— pueden ofrecer ventajas medibles sobre los sistemas binarios convencionales.**

T81 es una **arquitectura implementada y funcional**: define el Computador de Conjunto de Instrucciones Ternarias (TISC, por sus siglas en inglés) como su contrato de instrucciones computacionales, con una ejecución acotada entre perfiles demostrada tanto en una máquina virtual alojada (T81VM) como en un entorno de micronúcleo aislado *bare-metal* (Axion OS / TernaryOS en AArch64 QEMU).

El proyecto estudia la computación en varios niveles: representación matemática, algoritmos, ejecución de software, almacenamiento, arquitectura de hardware y comportamiento verificable del sistema. Su arquitectura orientada a la investigación se originó en las matemáticas discretas, CWEB/Hanoi y la investigación de sistemas numéricos alternativos antes de expandirse a los subsistemas de lenguajes, máquinas virtuales, instrucciones, políticas y garantía de investigación.

Una segunda dirección, explícitamente experimental, es la **ejecución adaptativa consciente de la representación**: seleccionar representaciones de datos, algoritmos y rutas de ejecución para cargas de trabajo particulares, especialmente en IA/ML, midiendo al mismo tiempo los costos completos de selección, conversión, computación y precisión. Esta es una hipótesis arquitectónica, **no** una ventaja de rendimiento establecida de T81.

Su pregunta central es:

> ¿Bajo qué condiciones la computación ternaria puede superar, simplificar o mejorar de otro modo un enfoque binario equivalente?

La respuesta no se da por sentada.

Algunos experimentos han demostrado propiedades matemáticas y de representación útiles. Otros han mostrado que las implementaciones binarias convencionales siguen siendo más rápidas o eficientes. Varias preguntas importantes siguen sin resolverse.

T81 existe para investigar esos límites de forma sistemática.

**Esta es una plataforma de investigación, no una afirmación de que ya se haya construido un computador ternario superior.**

---

## ¿Por qué el ternario?

La informática moderna utiliza abrumadoramente estados binarios: 0 y 1.

La computación ternaria utiliza tres estados. En el ternario equilibrado, estos se representan como:

$$T = \{-1,0,+1\}$$

Esto crea posibilidades interesantes para la aritmética con signo, la representación numérica, la codificación de estados, los sistemas de decisión y la arquitectura computacional.

Por ejemplo:

| Propiedad | Relación matemática |
| --- | --- |
| Un trit | Tres estados posibles |
| Cuatro trits | $3^4 = 81$ estados |
| Seis trits | $3^6 = 729$ estados |
| Información por trit | $\log_2 3 \approx 1.585$ bits |
| Dígito en base 81 | Cuatro trits agrupados |

Estas relaciones son hechos matemáticos.

**No** establecen que los procesadores ternarios sean más rápidos, que la memoria ternaria sea físicamente más densa o que los algoritmos ternarios consuman menos energía.

Esas son cuestiones de ingeniería separadas que requieren comparaciones adecuadas.

T81 investiga esta distinción.

---

## ¿Qué hemos aprendido?

La investigación es valiosa cuando identifica tanto oportunidades como limitaciones.

**Resultado principal — una comparación desfavorable con binario optimizado:** En el experimento histórico de matriz-vector AVX2 que se muestra a continuación, el INT8 nativo alcanzó **17.15 GFLOP/s**, frente a los **12.91 GFLOP/s** del ternario empaquetado de 2 bits (aproximadamente **un 32.8 % más de rendimiento** para la implementación INT8 nativa). Este es un resultado específico de la carga de trabajo, no una clasificación universal de bases numéricas; T81 lo publica de manera destacada porque un resultado desfavorable es evidencia, no un defecto que deba ocultarse.

La trayectoria experimental de T81 ha producido varios hallazgos importantes.

### 1. Resultado negativo en AVX2: la densidad de representación no es velocidad de ejecución

Los datos ternarios empaquetados pueden reducir los requisitos de almacenamiento.

Sin embargo, el software todavía debe decodificar, manipular y ejecutar esas representaciones en hardware binario convencional.

En un experimento histórico de matriz-vector AVX2:

| Implementación | Rendimiento informado |
| --- | --- |
| INT8 nativo AVX2 | 17.15 GFLOP/s |
| Ternario empaquetado de 2 bits AVX2 | 12.91 GFLOP/s |

La representación ternaria compacta no superó a la implementación INT8 convencional en esa carga de trabajo.

Este es un resultado experimental acotado, no una prueba de que la computación ternaria sea universalmente más lenta.

[Explorar la investigación sobre ternario empaquetado](https://www.google.com/search?q=docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md)

### 2. Las representaciones adaptativas pueden tener ventajas condicionales

La investigación sobre la jerarquización adaptativa de representaciones ha identificado configuraciones en las que una representación ternaria tiene un costo modelado menor que una alternativa binaria equivalente.

Esas ventajas dependen de las suposiciones de la carga de trabajo, los metadatos, los costos de conversión y los límites de representación.

La investigación corregida H04-R02 preserva estos hallazgos condicionales mientras deja explícitamente sin resolver el rendimiento físico.

[Leer los hallazgos corregidos de H04-R02](https://www.google.com/search?q=experiments/h04_r02_adaptive_representation/RESULTS.md)

### 3. La corrección no establece la optimización

Una investigación experimental reciente, T81-AIOPT-001, examinó múltiples representaciones de coincidencia de patrones de cuatro trits.

Su implementación M1 superó amplias comprobaciones de corrección local, pero la carga de trabajo experimental se pudo reducir a una simple comparación ordinal.

El resultado demuestra por qué una prueba de corrección exhaustiva aún puede ser insuficiente para evaluar la ventaja computacional.

La siguiente pregunta no es simplemente si otra representación funciona.

Es si la carga de trabajo elegida puede distinguir diferencias significativas entre algoritmos competidores.

[Explorar T81-AIOPT-001](https://www.google.com/search?q=docs/research/aiopt/)

### 4. Instantánea del estado de H04-R03 (2026-11-02): la corrección antes que el rendimiento

El programa de códecs emparejados H04-R03 compara alternativas acotadas de representación y acceso, incluido un candidato B729-µ5. Su protocolo pre-registrado y la implementación/revisión del Candidato A se rastrean en el [PR #677](https://github.com/t81dev/t81-foundation/pull/677), [PR #679](https://github.com/t81dev/t81-foundation/pull/679) y [PR #680](https://github.com/t81dev/t81-foundation/pull/680). Los informes de revisión independiente certifican únicamente la corrección y la aceptación del protocolo; a fecha del 2 de noviembre de 2026, los PRs #679 y #680 permanecen abiertos, y la medición de rendimiento de la Fase C no está autorizada ni reportada. El estado del PR debe actualizarse antes de confiar en esta instantánea.

La igualdad en la longitud de las cargas útiles es una propiedad matemática, no una latencia demostrada, ni un beneficio energético o de extremo a extremo. No se reivindica ninguna ventaja de rendimiento en H04-R03.

### 5. Demostración de ejecución acotada entre perfiles (CPA-004/CPA-005)

**Clasificación de la evidencia: determinismo acotado y portabilidad, no rendimiento comparativo ni madurez de todo el sistema.** Los resultados a continuación se refieren a un programa congelado y a entornos de ejecución especificados; no califican instrucciones no probadas, cargas de trabajo, hardware físico ni la arquitectura general.

Investigaciones recientes en el programa de Arquitectura Entre Perfiles (CPA) demostraron que el núcleo computacional de T81 se ejecuta de manera consistente a través de entornos de ejecución divergentes:

* **Artefacto de Programa Canónico:** Un programa canónico congelado de 351 bytes que comprende 27 registros canónicos de 13 bytes (SHA-256 `b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b`) ejerció un subconjunto admitido de 17 instrucciones TISC.
* **Equivalencia entre Perfiles:** Ejecuciones recientes a través de tres procesos de T81VM alojados y tres arranques limpios *bare-metal* en AArch64 QEMU (después de UEFI `ExitBootServices`) coincidieron con un oráculo matemático derivado de forma independiente en **los 25 límites de observación** (estado inicial + 24 pasos de instrucción).
* **Coincidencia Integral de Límites:** Se verificó la equivalencia en los 81 valores de registros y etiquetas, 1,563 palabras de memoria lógica y etiquetas, indicadores y registros del sistema (R75–R80), deteniéndose de manera determinista en PC=27, SP=283.
* **Alcance e Instantánea:** Probado en la instantánea base del repositorio `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4` (con evaluador independiente e informe de aceptación comprometidos en la rama `origin/experiment/cpa004-freestanding` en el commit `b803bf916a1b1e44129f3a07abf96fa6103c51fc`, y registros de continuidad locales CPA-005).
* **Distinción:** Demostrar la corrección de la ejecución entre perfiles prueba que la arquitectura computacional funciona según lo diseñado; **no** es evidencia de rendimiento ternario, energía o superioridad de hardware sobre las líneas base binarias.

### 6. La computación ternaria física sigue siendo una pregunta abierta

La computación ternaria emulada por software no es equivalente a la computación en hardware ternario nativo.

La evaluación de dispositivos físicos requiere evidencia emparejada sobre área, energía, latencia, fiabilidad, detección, codificación y costos periféricos.

La evaluación física actual sigue bloqueada allí donde no se dispone de evidencia adecuada sobre la caracterización de los dispositivos.

T81 no interpreta la falta de evidencia como prueba de imposibilidad.

---

## La arquitectura de investigación de T81

El repositorio contiene varios subsistemas experimentales desarrollados para investigar las representaciones computacionales y la ejecución controlada.

```text
                     T81 FOUNDATION
                Arquitectura e Investigación Implementadas
                             |
          +------------------+------------------+
          |                  |                  |
    REPRESENTACIÓN       EJECUCIÓN          GARANTÍA
          |                  |                  |
    Ternario equilibrado TISC ISA (Contrato) Axion Policy
    Base-81 / Base-243   +-- T81VM (Alojado)  CanonFS
    Datos empaquetados   +-- Axion (Baremetal) Evidence
    Jerarquía adaptativa T81Lang Frontend    Provenance
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

* **TISC (Ternary Instruction Set Computer):** Sirve como el contrato central de instrucciones computacionales, definiendo la semántica de los opcodes, vectores de estado de 64 bits y reglas de transición deterministas.
* **T81VM Alojada:** Sirve como perfil de ejecución alojado para el desarrollo en espacio de usuario, herramientas, pruebas y reproducción determinista en sistemas operativos anfitriones convencionales (Linux, macOS, Windows).
* **Axion OS / TernaryOS Aislado (*Freestanding*):** Sirve como perfil de ejecución *bare-metal* que se ejecuta directamente sobre hardware o hipervisores (por ejemplo, AArch64 QEMU), ejecutando TISC sin dependencias de un SO en espacio de usuario.
* **DAIOS (Deterministic AI OS):** Permanece como una dirección de investigación experimental independiente y orientada al futuro para la gobernanza del tiempo de ejecución de agentes autónomos de alta IA, distinta del motor de ejecución TISC de bajo nivel implementado.

### Componentes principales

| Componente | Rol de investigación y arquitectónico |
| --- | --- |
| **TISC** | El contrato de instrucciones computacionales que rige la semántica de ejecución |
| **T81VM** | Perfil de ejecución alojado para la ejecución en máquina virtual y determinismo (acotado por el [Registro de Superficie de Determinismo](https://www.google.com/search?q=docs/governance/DETERMINISM_SURFACE_REGISTRY.md)) |
| **Axion / TernaryOS** | Perfil de ejecución aislado (micronúcleo HAL / bare-metal AArch64) y autorización de políticas previa a los efectos secundarios |
| **T81Lang** | Interfaz de lenguaje e infraestructura de compilación orientada a TISC |
| **T81 Core** | Tipos numéricos, contenedores y primitivas de representación |
| **CanonFS** | Artefactos direccionados por contenido y mecanismos de integridad |
| **DAIOS** | Propuesta de arquitectura de destino independiente para sistemas operativos de IA deterministas |
| **Experimentos de investigación** | Pruebas de hipótesis, validación de corrección y mediciones comparativas |

Estos componentes tienen diferentes niveles de madurez. Una implementación funcional no constituye automáticamente una ventaja de rendimiento validada.

---

## Ejecución adaptativa consciente de la representación: una línea de investigación abierta

Los manuscritos históricos de T81 describen una ambición integrada: tipos de datos numéricos, semántica de ejecución TISC, T81Lang/T81VM y decisiones mediadas por Axion sobre el uso de recursos. La [colección histórica de PDFs](https://www.google.com/search?q=docs/pdf/) preserva estas propuestas; las cifras de rendimiento reportadas en ellos son **afirmaciones históricas no verificadas**, no resultados experimentales actuales.

Una versión falsificable moderna de esta ambición se pregunta si un sistema puede seleccionar entre representaciones exactas, de punto flotante, de baja cantidad de bits, ternarias empaquetadas y binarias convencionales para una **carga de trabajo real**, logrando un beneficio neto después de contabilizar la precisión, la codificación/decodificación, el tráfico de memoria, la planificación y la sobrecarga de decisiones. El proyecto debe distinguir:

* **T — mecanismo específico del ternario:** una ventaja atribuible a la semántica ternaria o a un sustrato ternario físico, no meramente a una codificación diferente.
* **S — elección de representación por software:** una ventaja derivada de seleccionar una representación, potencialmente reproducible con codificaciones binarias.
* **A — encaminamiento de algoritmos:** una ventaja derivada de elegir un mejor algoritmo o ruta de ejecución, independientemente de la base numérica.

Los resultados positivos de S o A son útiles pero **no** establecen T. T81-AIOPT-001 M2 sigue siendo un problema de diseño de investigación más que una prueba autorizada de la superioridad ternaria adaptativa. Para consultar el alcance autorizado y los límites, acuda al [contexto de investigación de IA](https://www.google.com/search?q=docs/ai-context/) y a los protocolos subyacentes de los experimentos.

## Tres líneas de investigación

T81 organiza su trabajo en tres líneas complementarias.

### Línea R — Investigación computacional

Investigar si las representaciones alternativas o las arquitecturas computacionales producen ventajas útiles.

Los temas incluyen:

* Aritmética y representación en ternario equilibrado.
* Base-81 y agrupamiento de orden superior.
* Selección adaptativa de representación.
* Computación empaquetada de bajo número de bits.
* Costos de memoria y movimiento de datos.
* Hipótesis de hardware ternario nativo.
* Inferencia de IA ternaria y representaciones de modelos.

**El éxito significa establecer un resultado defendible, incluido un resultado negativo.**

### Línea F — Ingeniería de cimientos

Desarrollar las herramientas necesarias para realizar experimentos confiables y evaluar la computación determinista.

Esto incluye máquinas virtuales, herramientas de lenguaje, identidad de artefactos, autorización, reproducción e infraestructura de verificación.

Estas capacidades pueden ser útiles independientemente de la computación ternaria.

Su valor de ingeniería debe evaluarse por separado de las afirmaciones de superioridad específica de la base numérica.

### Línea M — Memoria de investigación

Preservar la evidencia detrás de cada conclusión.

Esto incluye el historial de fuentes, protocolos experimentales, intentos fallidos, correcciones, registros de reproducibilidad y preguntas sin resolver.

Un resultado superado sigue formando parte del registro histórico.

Un modelo matemático no se promueve silenciosamente a medición física.

Una evaluación generada por IA no constituye automáticamente una verificación independiente.

[Leer la Carta de Continuidad Maestra](https://www.google.com/search?q=docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md)

---

## Metodología de investigación

T81 sigue un enfoque basado primero en la evidencia.

Cada experimento comparativo debe responder a cinco preguntas:

1. **¿Cuál es la hipótesis?** Definir una proposición falsificable.
2. **¿Cuál es la línea base?** Comparar con una alternativa convencional funcionalmente equivalente y optimizada de manera adecuada.
3. **¿Qué se mide?** Separar la corrección, el almacenamiento, la latencia, el rendimiento, la energía y otros costos relevantes.
4. **¿Cuáles son las limitaciones?** Identificar suposiciones, hardware, alcance de la carga de trabajo, incertidumbre y dependencias sin resolver.
5. **¿Qué conclusión está justificada?** Distinguir resultados respaldados, no respaldados, inconclusos, bloqueados y no probados.

El proyecto también distingue entre:

**Implementado → Probado → Medido → Establecido comparativamente**

Estos son diferentes niveles de evidencia, no etiquetas intercambiables.

Un resultado experimental no debe afirmar más de lo que su evidencia respalda.

---

## La visión original y los manuscritos históricos

El [archivo histórico de PDFs](https://www.google.com/search?q=docs/pdf/) contiene exploraciones tempranas de Base-81, tipos de datos, T81Lang, TISC, Axion, IA/ML y aplicaciones propuestas. Estos documentos preservan el linaje intelectual y las hipótesis de diseño; **no** son especificaciones actuales ni informes de pruebas comparativas verificados. Una revisión futura debería conciliar las afirmaciones con los experimentos principales sin sobrescribir los originales.

### Hanoi

La historia de T81 es anterior al actual repositorio de la Fundación.

Su precursor conocido más antiguo, **Hanoi**, exploró la computación recursiva, las estructuras de máquinas virtuales ternarias, el despacho simbólico y los niveles de representación dependientes de la carga de trabajo utilizando la programación literaria CWEB.

El proyecto surgió de investigaciones en matemáticas discretas, algoritmos, modelos computacionales y el trabajo de Donald Knuth.

Con el tiempo, la investigación se expandió hacia bibliotecas aritméticas, herramientas de lenguaje, máquinas virtuales, computación de IA con pocos bits, ejecución determinista, gobernanza y hardware experimental.

Algunos diseños históricos estaban incompletos. Algunas suposiciones resultaron ser incorrectas. Otras siguen sin explorarse.

El propósito de preservar este linaje no es validar cada propuesta anterior.

Es recuperar las preguntas que las motivaron y poner a prueba esas preguntas con mejores herramientas.

[Explorar el repositorio original de Hanoi](https://github.com/t81dev/Hanoi) · [Archivo de continuidad de investigación](https://www.google.com/search?q=docs/research/continuity/)

---

## Prioridades actuales de investigación

Las prioridades inmediatas del programa son:

**1. Establecer líneas base experimentales confiables.**

Verificar una semántica equivalente y un comportamiento reproducible de forma independiente antes de interpretar las diferencias de rendimiento.

**2. Identificar cargas de trabajo discriminatorias.**

Seleccionar experimentos en los que las representaciones alternativas tengan una consecuencia plausible y medible más allá de los cambios ordinarios de codificación.

**3. Investigar ventajas condicionales y cargas de trabajo de IA/ML.**

Estudiar las circunstancias bajo las cuales la densidad de representación, los patrones de acceso, los costos de conversión, la precisión del modelo o las estrategias adaptativas cambian el resultado. Comparar con líneas base binarias e de enrutamiento de algoritmos igualmente optimizadas.

**4. Preservar las preguntas sobre el hardware físico.**

Separar las limitaciones de la emulación por software del costo y la viabilidad de los dispositivos multivaluados nativos.

**5. Mantener la base experimental.**

Mantener las herramientas, especificaciones, pruebas, registros de procedencia y mecanismos de gobernanza lo suficientemente confiables como para respaldar la investigación.

Las propuestas activas y su estado de aceptación pertenecen a los registros de investigación y a las solicitudes de extracción (*pull requests*). Su aparición en el repositorio no implica su aprobación formal.

[Leer la hoja de ruta para colaboradores](https://www.google.com/search?q=docs/ROADMAP.md)

---

## Explora el repositorio

| Punto de partida | Qué encontrarás |
| --- | --- |
| [`docs/research/`](https://www.google.com/search?q=docs/research/) | Investigaciones científicas y conclusiones experimentales |
| [`experiments/`](https://www.google.com/search?q=experiments/) | Implementaciones experimentales y artefactos de reproducibilidad |
| [`spec/`](https://www.google.com/search?q=spec/) | Especificaciones del conjunto de instrucciones y RFCs |
| [`include/t81/`](https://www.google.com/search?q=include/t81/) | Bibliotecas centrales e interfaces públicas |
| [`vm/`](https://www.google.com/search?q=vm/) | Implementación de la máquina virtual |
| [`lang/`](https://www.google.com/search?q=lang/) | Herramientas de lenguaje |
| [`kernel/axion/`](https://www.google.com/search?q=kernel/axion/) | Componentes de autorización y políticas |
| [`fs/`](https://www.google.com/search?q=fs/) | Identidad de artefactos y mecanismos de almacenamiento |
| [`docs/ai-context/`](https://www.google.com/search?q=docs/ai-context/) | Material de continuidad para colaboradores de investigación en IA |
| [`docs/pdf/`](https://www.google.com/search?q=docs/pdf/) | Manuscritos históricos y sus hipótesis de investigación originales |

Para instrucciones de compilación, dependencias de desarrollo y objetivos de prueba disponibles, consulta la documentación para colaboradores del repositorio y la configuración de CMake. Los resultados de la investigación deben reproducirse utilizando el entorno y los comandos especificados por sus respectivos protocolos individuales.

---

## Contribuir

T81 agradece las contribuciones que mejoren la comprensión científica, la corrección de la implementación, la reproducibilidad o la documentación.

Entre las contribuciones especialmente valiosas se incluyen:

* Reproducción independiente de experimentos existentes.
* Líneas base binarias convencionales más sólidas.
* Contraejemplos matemáticos y correcciones de pruebas.
* Protocolos experimentales más pequeños y discriminatorios.
* Validación de corrección multiplataforma.
* Correcciones respaldadas por evidencia a afirmaciones históricas.
* Mejoras con alcance claro en la infraestructura de investigación.

Antes de proponer un nuevo subsistema, identifica a qué pregunta de investigación responde y qué evidencia establecería su utilidad.

Lee la [hoja de ruta para colaboradores](https://www.google.com/search?q=docs/ROADMAP.md), las instrucciones de gobernanza del repositorio y los protocolos de experimentos relevantes antes de modificar los artefactos de investigación.

---

## Lo que T81 no afirma

Actualmente, T81 no afirma:

* La superioridad universal de la computación ternaria sobre la binaria.
* La superioridad demostrada del silicio ternario nativo.
* Una ventaja general de rendimiento para la arquitectura integrada T81.
* Un optimizador de IA adaptativo validado que supere a las líneas base sólidas de política fija o de selección de algoritmos.
* Que las cifras de los casos de estudio de los PDFs históricos constituyan mediciones reproducidas independientemente.
* Que los pesos de los modelos ternarios aceleren inherentemente la inferencia de IA.
* Que la ejecución determinista o la aplicación de políticas requiera una máquina virtual ternaria.
* Que un experimento generado por IA esté validado de forma independiente meramente porque sus pruebas pasen.

Estos límites son esenciales para la investigación.

Dejan espacio para futuros descubrimientos sin tergiversar lo que ya se ha establecido.

---

## La pregunta sigue abierta

La computación binaria domina la infraestructura física y de software actual por razones de ingeniería convincentes.

Dicho dominio no elimina el valor de investigar representaciones alternativas, modelos computacionales o futuros sustratos de hardware.

El propósito de T81 es determinar dónde son útiles esas alternativas, y dónde no lo son.

**No estamos construyendo una conclusión y buscando evidencia para respaldarla.**

**Estamos construyendo la base experimental necesaria para descubrir la conclusión.**

---

**T81 Foundation** — Arquitectura Informática Experimental e Investigación de Sistemas

[Licencia: Apache 2.0](https://www.google.com/search?q=LICENSE) · [GitHub](https://github.com/t81dev/t81-foundation)
