# Fundación T81 — Monografía Técnica Definitiva

## Prefacio

Hay dos formas de construir sistemas.

Una es optimizar por conveniencia — moverse rápido, aproximar, aceptar que el último bit puede variar, que la deriva del punto flotante es tolerable, que los compiladores pueden reordenar, que el hardware decidirá qué significa "lo suficientemente cerca".

La otra es insistir en que la computación no es una sugerencia, sino una declaración.

T81 pertenece al segundo camino.

En su núcleo, este proyecto no trata sobre aritmética ternaria, máquinas virtuales o motores de políticas — aunque contiene todo esto. Trata sobre la **integridad de la ejecución**. Trata sobre trazar un límite alrededor de un proceso computacional y decir: dentro de este límite, el comportamiento no es incidental.

El determinismo a menudo se trata como un compromiso de rendimiento o una conveniencia de depuración. Aquí se trata como una restricción civilizacional. Si dos máquinas no pueden ponerse de acuerdo sobre el resultado del mismo programa, entonces la computación nunca fue verdaderamente definida — simplemente fue realizada.

Ternario balanceado, serialización canónica, matemáticas definidas por software, registro de trazas, cumplimiento de políticas — estas no son opciones estéticas. Son instrumentos en un solo argumento:

> Una computación debe ser reproducible, auditable y estructuralmente honesta.

Los sistemas modernos están estratificados con abstracciones que ocultan las transiciones de estado detrás de optimizadores, ejecución especulativa, peculiaridades del punto flotante y efectos secundarios implícitos. T81 intenta algo diferente: hacer que cada transición sea explícita, cada representación canónica, cada ejecución rastreable.

Es un experimento arquitectónico en restricción.

El sistema no asume hardware benevolente.
No asume bibliotecas de punto flotante idénticas.
No asume que los compiladores se comporten igual en diferentes arquitecturas.
No asume que la ejecución sin registro sea aceptable.

En cambio, codifica reglas:

* Las transiciones de estado deben ser definibles.
* Los datos deben tener una única forma canónica.
* El consumo de recursos debe ser responsable.
* Las políticas deben ser aplicables.
* El comportamiento debe ser reproducible.

El resultado no es la máquina más rápida.
No es el entorno más flexible.
No está diseñado para reemplazar ecosistemas de scripting de propósito general.

Está diseñado para responder a una pregunta más estrecha pero más exigente:

**¿Se puede construir un sistema de software tal que su comportamiento sea demostrablemente invariante a través del espacio y el tiempo?**

Este libro existe para documentar ese intento.

No como mitología.
No como marketing.
Sino como un libro mayor.

Cada subsistema descrito aquí — T81Lang, TISC, la T81VM, Axion, CanonFS, las puertas de determinismo, los niveles cognitivos — es parte de una estructura estratificada construida alrededor de un invariante:

> Entradas idénticas deben producir salidas idénticas, bajo reglas explícitamente definidas.

Si esta arquitectura se adopta ampliamente es secundario. Lo que importa es que se ha hecho concreta, implementada, probada y descrita con suficiente precisión para que pueda ser entendida, verificada o desafiada por otros.

Este volumen es, por lo tanto, tanto técnico como filosófico.

Es técnico porque describe un sistema en funcionamiento.
Es filosófico porque afirma que la reproducibilidad no es opcional en ciertos dominios.

Si el repositorio evoluciona, este libro debería evolucionar con él.
Si el proyecto cesa, este documento debería seguir siendo suficiente para reconstruir lo que se intentó y por qué.

Al final, T81 no es una afirmación de perfección.

Es un compromiso con la restricción.

Y la restricción, cuando se aplica deliberadamente, es una forma de claridad.

---

## Cómo Leer Este Libro

* **¿Nuevo en T81?** → Comience con la Parte I, luego la Parte II.
* **¿Implementador?** → Concéntrese en las Partes II y III.
* **¿Auditor?** → Lea las Partes III y IV cuidadosamente.
* **¿Investigador?** → Enfatice las Partes IV y V.
* **¿Mantenedor a largo plazo?** → Las Partes IV y V son críticas.

---

## Navegación

<details open>
<summary><strong>Parte I — Fundamentos</strong></summary>

1. **[Introducción](./01_Introduccion.md)**

   * [1.1 Alcance y Definición](./01_Introduccion.md#11-alcance-y-definición)
   * [1.2 Arquitectura del Sistema](./01_Introduccion.md#12-arquitectura-del-sistema)
   * [1.3 Misión de Cómputo Verificable](./01_Introduccion.md#13-misión-de-cómputo-verificable)
   * [1.4 Terminología](./01_Introduccion.md#14-terminología)
   * [1.5 Lista de Verificación](./01_Introduccion.md#15-lista-de-verificación)

2. **[Principios e Invariantes Centrales](./02_Principios.md)**

   * [2.1 El Invariante de Determinismo](./02_Principios.md#21-el-invariante-de-determinismo)
   * [2.1.1 Superficies de Determinismo y Vectores de Ataque](./02_Principios.md#211-superficies-de-determinismo-y-vectores-de-ataque)
   * [2.2 Lógica Ternaria (Base-3)](./02_Principios.md#22-lógica-ternaria-base-3)
   * [2.3 Auditabilidad y la Traza Axion](./02_Principios.md#23-auditabilidad-y-la-traza-axion)
   * [2.4 Los Nueve Principios (Cumplimiento Ético)](./02_Principios.md#24-los-nueve-principios-cumplimiento-ético)
   * [2.5 Lista de Verificación](./02_Principios.md#25-lista-de-verificación)
   * [2.6 Matriz de Auditoría Formal](./02_Principios.md#26-matriz-de-auditoría-formal)

</details>

<details>
<summary><strong>Parte II — La Máquina Determinista</strong></summary>

3. **[Arquitectura T81VM](./03_Arquitectura.md)**

   * [3.1 Visión General](./03_Arquitectura.md#31-visión-general)
   * [3.2 El Límite del Runtime](./03_Arquitectura.md#32-el-límite-del-runtime)
   * [3.3 Modelo de Memoria](./03_Arquitectura.md#33-modelo-de-memoria)
   * [3.4 El Conjunto de Instrucciones (TISC)](./03_Arquitectura.md#34-el-conjunto-de-instrucciones-tisc)
   * [3.5 Compilación JIT (Trace-JIT)](./03_Arquitectura.md#35-compilación-jit-trace-jit)

4. **[Tipos de Datos y Serialización](./04_Tipos_de_Datos_y_Serializacion.md)**

   * [4.1 Tipos Primitivos](./04_Tipos_de_Datos_y_Serializacion.md#41-tipos-primitivos)
   * [4.2 T81Float y dmath](./04_Tipos_de_Datos_y_Serializacion.md#42-t81float-y-dmath)
   * [4.3 Tensores y Diseños Canónicos](./04_Tipos_de_Datos_y_Serializacion.md#43-tensores-y-diseños-canónicos)
   * [4.4 Reglas de Serialización Canónica](./04_Tipos_de_Datos_y_Serializacion.md#44-reglas-de-serialización-canónica)

5. **[Instalación y Verificación de Construcción](./05_Instalacion.md)**

   * [5.1 Requisitos Previos](./05_Instalacion.md#51-requisitos-previos)
   * [5.2 Construcción desde la Fuente](./05_Instalacion.md#52-construcción-desde-la-fuente)
   * [5.3 Verificando la Construcción](./05_Instalacion.md#53-verificando-la-construcción)
   * [5.4 Solución de Problemas](./05_Instalacion.md#54-solución-de-problemas)

6. **[Uso de CLI y API](./06_Uso.md)**

   * [6.1 La Interfaz de Línea de Comandos T81](./06_Uso.md#61-la-interfaz-de-línea-de-comandos-t81)
   * [6.2 Embebiendo T81 (API C++)](./06_Uso.md#62-embebiendo-t81-api-c)
   * [6.3 Embebiendo T81 (API Python)](./06_Uso.md#63-embebiendo-t81-api-python)
   * [6.4 Depuración](./06_Uso.md#64-depuración)

</details>

<details>
<summary><strong>Parte III — Gobernanza y Verificación</strong></summary>


7. **[Programación en T81Lang](./07_Programacion_en_T81Lang.md)**

   * [7.1 Filosofía de Diseño](./07_Programacion_en_T81Lang.md#71-filosofia-de-diseno)
   * [7.2 Conceptos Básicos de Sintaxis](./07_Programacion_en_T81Lang.md#72-conceptos-basicos-de-sintaxis)
   * [7.3 Tipos de Datos](./07_Programacion_en_T81Lang.md#73-tipos-de-datos)
   * [7.4 Flujo de Control](./07_Programacion_en_T81Lang.md#74-flujo-de-control)
   * [7.5 Funciones](./07_Programacion_en_T81Lang.md#75-funciones)
   * [7.6 Integración con Axion](./07_Programacion_en_T81Lang.md#76-integracion-con-axion)
   * [7.7 Ejemplos](./07_Programacion_en_T81Lang.md#77-ejemplos)
8. **[Verificación y Auditoría](./08_Verificacion_y_Auditoria.md)**

   * [8.1 Metodología de Verificación Formal](./08_Verificacion_y_Auditoria.md#81-metodología-de-verificación-formal)
   * [8.2 La Matriz de Auditoría Formal](./08_Verificacion_y_Auditoria.md#82-la-matriz-de-auditoría-formal)
   * [8.3 Pruebas Basadas en Propiedades](./08_Verificacion_y_Auditoria.md#83-pruebas-basadas-en-propiedades)
   * [8.4 La Puerta de Determinismo](./08_Verificacion_y_Auditoria.md#84-la-puerta-de-determinismo)

9. **[El Kernel de Seguridad Axion](./09_El_Kernel_Axion.md)**

   * [9.1 Definición Formal](./09_El_Kernel_Axion.md#91-definición-formal)
   * [9.2 El Modelo de Políticas](./09_El_Kernel_Axion.md#92-el-modelo-de-políticas)
   * [9.3 Intercepción de Instrucciones](./09_El_Kernel_Axion.md#93-intercepción-de-instrucciones)
   * [9.4 El Registro de Auditoría (Traza)](./09_El_Kernel_Axion.md#94-el-registro-de-auditoría-traza)
   * [9.5 Promoción Cognitiva](./09_El_Kernel_Axion.md#95-promoción-cognitiva)

10. **[Niveles Cognitivos y Cómputo Distribuido](./10_Niveles_Cognitivos_y_Computo_Distribuido.md)**

   * [10.1 El Modelo de Niveles Cognitivos](./10_Niveles_Cognitivos_y_Computo_Distribuido.md#101-el-modelo-de-niveles-cognitivos)
   * [10.2 Cómputo Distribuido (Nivel 4)](./10_Niveles_Cognitivos_y_Computo_Distribuido.md#102-cómputo-distribuido-nivel-4)
   * [10.3 Compilación JIT Basada en Trazas](./10_Niveles_Cognitivos_y_Computo_Distribuido.md#103-compilación-jit-basada-en-trazas)
   * [10.4 Formas Infinitas (Nivel 5)](./10_Niveles_Cognitivos_y_Computo_Distribuido.md#104-formas-infinitas-nivel-5)

11. **[Apéndices](./11_Apendices.md)**

   * [11.1 Lo Que Aún No Está Implementado](./11_Apendices.md#111-lo-que-aún-no-está-implementado)
   * [11.2 Glosario](./11_Apendices.md#112-glosario)
   * [11.3 Enlaces Útiles](./11_Apendices.md#113-enlaces-útiles)

</details>

<details>
<summary><strong>Parte IV — Formalización y Endurecimiento Estructural</strong></summary>

12. **[Semántica Formal de TISC y T81VM](./12_Semantica_Formal.md)**

   * [12.1 Semántica Operacional](./12_Semantica_Formal.md#121-semántica-operacional)
   * [12.2 Función de Transición Algebraica](./12_Semantica_Formal.md#122-función-de-transición-algebraica)
   * [12.3 Sistema de Reescritura de Canonicalización](./12_Semantica_Formal.md#123-sistema-de-reescritura-de-canonicalización)
   * [12.4 Bocetos de Prueba de Determinismo](./12_Semantica_Formal.md#124-bocetos-de-prueba-de-determinismo)
   * [12.5 Equivalencia Intérprete vs Trace-JIT](./12_Semantica_Formal.md#125-equivalencia-intérprete-vs-trace-jit)

13. **[Modelado Adversarial y Ataques de Determinismo](./13_Modelado_Adversarial.md)**

   * [13.1 Modelo de Amenazas](./13_Modelado_Adversarial.md#131-modelo-de-amenazas)
   * [13.2 Ataques a Nivel de Compilador](./13_Modelado_Adversarial.md#132-ataques-a-nivel-de-compilador)
   * [13.3 Vectores de Ataque de VM y GC](./13_Modelado_Adversarial.md#133-vectores-de-ataque-de-vm-y-gc)
   * [13.4 Ataques a CanonFS y Hash](./13_Modelado_Adversarial.md#134-ataques-a-canonfs-y-hash)
   * [13.5 Ataque de Viaje en el Tiempo de Nivel Distribuido](./13_Modelado_Adversarial.md#135-ataque-de-viaje-en-el-tiempo-de-nivel-distribuido)
   * [13.6 Plantilla Post-Mortem de Brecha de Determinismo](./13_Modelado_Adversarial.md#136-plantilla-post-mortem-de-brecha-de-determinismo)

</details>

<details>
<summary><strong>Parte V — Continuidad y Horizonte de Investigación</strong></summary>

14. **[Continuidad y Resiliencia](./14_Continuidad_Resiliencia.md)**

   * [14.1 El Protocolo de Sala Limpia](./14_Continuidad_Resiliencia.md#141-el-protocolo-de-sala-limpia)
   * [14.2 Puntos Únicos de Fallo](./14_Continuidad_Resiliencia.md#142-puntos-únicos-de-fallo)
   * [14.3 Manifiesto de Continuidad](./14_Continuidad_Resiliencia.md#143-manifiesto-de-continuidad)
   * [14.4 Invariantes Formales Inmutables](./14_Continuidad_Resiliencia.md#144-invariantes-formales-inmutables)

15. **[Frontera de Investigación](./15_Frontera_de_Investigacion.md)**

   * [15.1 Aceleración de Hardware Ternario](./15_Frontera_de_Investigacion.md#151-aceleración-de-hardware-ternario)
   * [15.2 Rutas de Verificación Formal](./15_Frontera_de_Investigacion.md#152-rutas-de-verificación-formal)
   * [15.3 CanonFS como Sustrato Merkle](./15_Frontera_de_Investigacion.md#153-canonfs-como-sustrato-merkle)
   * [15.4 Inferencia de IA Determinista a Escala](./15_Frontera_de_Investigacion.md#154-inferencia-de-ia-determinista-a-escala)

</details>
