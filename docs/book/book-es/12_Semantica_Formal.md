# Capítulo 12: Semántica Formal de TISC y T81VM

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Capítulo 12: Semántica Formal de TISC y T81VM](#capítulo-12-semántica-formal-de-tisc-y-t81vm)
  - [12.1 Semántica Operacional](#121-semántica-operacional)
    - [12.1.1 La Función de Transición $\delta$](#1211-la-función-de-transición-$\delta$)
    - [12.1.2 Reglas de Inferencia](#1212-reglas-de-inferencia)
      - [**Regla: Obtener-Decodificar**](#**regla-obtener-decodificar**)
      - [**Regla: Aritmética (Add)**](#**regla-aritmética-add**)
      - [**Regla: Verificación de Política (Axion)**](#**regla-verificación-de-política-axion**)
  - [12.2 Función de Transición Algebraica](#122-función-de-transición-algebraica)
  - [12.3 Sistema de Reescritura de Canonicalización](#123-sistema-de-reescritura-de-canonicalización)
    - [12.3.1 Formas Normales](#1231-formas-normales)
  - [12.4 Bocetos de Prueba de Determinismo](#124-bocetos-de-prueba-de-determinismo)
    - [Teorema 1: Independencia de Hardware de `dmath`](#teorema-1-independencia-de-hardware-de-`dmath`)
    - [Teorema 2: Seguridad de Axion](#teorema-2-seguridad-de-axion)
  - [12.5 Equivalencia Intérprete vs Trace-JIT](#125-equivalencia-intérprete-vs-trace-jit)

<!-- T81-TOC:END -->


## 12.1 Semántica Operacional

**Estado: Formalizando**

La Máquina Virtual T81 (T81VM) se define formalmente como un **Autómata Finito Determinista con Memoria Infinita** (DFA-IM). Su semántica se especifica utilizando **Semántica Operacional de Paso Pequeño** (SSOS).

### 12.1.1 La Función de Transición $\delta$

El núcleo de la VM es la función de transición $\delta$, que mapea un estado $S$ a un estado sucesor $S'$:
$$
\delta: S \to S'
$$
donde $S = (\mathbf{R}, \mathbf{M}, \mathbf{K}, \mathbf{\Phi}, PC)$ como se define en el Capítulo 3.

La ejecución de un programa es la aplicación repetida de $\delta$ hasta que se alcanza un estado terminal:
$$
S_0 \xrightarrow{\delta} S_1 \xrightarrow{\delta} \dots \xrightarrow{\delta} S_n
$$
donde $S_n$ es un **Estado de Parada** (`STOP` o `TRAP`).

### 12.1.2 Reglas de Inferencia

Usamos reglas de inferencia de la forma:
$$
\frac{\text{Premisa}}{\text{Conclusión}}
$$

#### **Regla: Obtener-Decodificar**
$$
\frac{Code[PC] = \text{Op} \quad \text{Arity}(\text{Op}) = k}{S \to (PC \leftarrow PC + 1 + k, \dots)}
$$

#### **Regla: Aritmética (Add)**
$$
\frac{Code[PC] = \text{Add } r_d, r_a, r_b \quad v_a = R[r_a] \quad v_b = R[r_b] \quad v' = v_a + v_b}{S \to S[R[r_d] \leftarrow v', \Phi \leftarrow \text{Flags}(v')]}
$$
*Invariante*: La suma $+$ es la suma ternaria balanceada definida en `dmath`.

#### **Regla: Verificación de Política (Axion)**
Antes de aplicar cualquier transición $\delta$, la función Axion $\alpha$ debe aprobar:
$$
\frac{\alpha(S, \text{Op}) = \text{Deny}}{S \to S_{\text{Trap}}(\text{SecurityFault})}
$$
$$
\frac{\alpha(S, \text{Op}) = \text{Allow}}{S \to \delta_{\text{Op}}(S)}
$$

## 12.2 Función de Transición Algebraica

**Estado: Teórico**

También podemos ver el estado de la VM como un elemento de una estructura algebraica (un Anillo de Estados). Esto nos permite probar propiedades sobre la ejecución.

Sea $\mathcal{S}$ el conjunto de todos los estados de máquina válidos.
Sea $\mathcal{O}$ el conjunto de todas las secuencias de opcodes válidas.
Definimos una acción de monoide $\cdot : \mathcal{O} \times \mathcal{S} \to \mathcal{S}$.

**Propiedad: Asociatividad de Ejecución**
$$
(Op_1 \cdot Op_2) \cdot S \equiv Op_1 \cdot (Op_2 \cdot S)
$$

**Propiedad: Determinismo**
$$
\forall S \in \mathcal{S}, Op \in \mathcal{O}: \exists! S' \text{ tal que } S \xrightarrow{Op} S'
$$

## 12.3 Sistema de Reescritura de Canonicalización

**Estado: Implementado**

El compilador y runtime de T81 emplean un **Sistema de Reescritura** para asegurar formas canónicas. Esto es crítico para la estabilidad de `CanonHash81`.

### 12.3.1 Formas Normales
Un valor $v$ está en **Forma Normal** si no puede ser reescrito más por las reglas de reducción.

**Regla: Normalización de Flotantes**
$$
\text{Float}(s, m, e) \xrightarrow{} \text{Float}(s, m', e')
$$
donde $m'$ no tiene trits cero a la izquierda (a menos que $m'=0$).

**Regla: Ordenación Topológica de Grafos**
Dado un grafo $G=(V, E)$, reescribimos los índices de nodo $V \to \{0 \dots |V|-1\}$ tal que:
$$
i < j \implies \text{TopoRank}(v_i) < \text{TopoRank}(v_j)
$$
Esto asegura que los grafos isomorfos se serialicen en flujos de bytes idénticos.

## 12.4 Bocetos de Prueba de Determinismo

### Teorema 1: Independencia de Hardware de `dmath`

**Afirmación**: Para cualquier entrada `T81Float` $x, y$ y operación $\odot \in \{+, -, *, /\}$, el resultado $z = x \odot y$ es idéntico bit a bit en las arquitecturas $A_1$ (x86) y $A_2$ (ARM).

**Boceto de Prueba**:
1.  `T81Float` se compone de mantisa y exponente `T81Int`.
2.  Las operaciones `T81Int` usan solo aritmética entera (suma, resta, mult, div, mod).
3.  La aritmética entera está estandarizada por el estándar C++ (Complemento a Dos) y es isomorfa al anillo de enteros abstracto $\mathbb{Z}_{2^{64}}$.
4.  Dado que $A_1$ y $A_2$ implementan $\mathbb{Z}_{2^{64}}$ correctamente (verificado por pruebas de conformidad), y la lógica `dmath` es puramente una composición de estas operaciones enteras, el resultado debe ser idéntico. $\blacksquare$

### Teorema 2: Seguridad de Axion

**Afirmación**: Si la Política $P$ prohíbe opcodes en el conjunto $\mathcal{F}$, entonces ningún estado alcanzable desde $S_0$ habrá sido producido por un opcode $op \in \mathcal{F}$.

**Boceto de Prueba**:
1.  La función de transición $\delta$ está protegida: $\delta(S) = \text{si } \alpha(S, Op) \text{ entonces } \dots \text{ si no } \text{Trap}$.
2.  La función $\alpha(S, Op)$ devuelve `Deny` si $Op \in \mathcal{F}$ (por definición de cumplimiento de Política).
3.  Por lo tanto, se toma la rama "si no", llevando a un estado Trap, no a un estado producido por la ejecución de $Op$.
4.  Por inducción, ninguna transición de estado válida usa $Op$. $\blacksquare$

## 12.5 Equivalencia Intérprete vs Trace-JIT

**Estado: En Progreso**

La optimización Trace-JIT es una transformación que preserva la semántica.

Sea $I(S)$ la función del intérprete.
Sea $J(S)$ la función compilada por JIT para una traza $T$.

**Requisito**:
$$
\forall S: J(S) \cong I(S)
$$
donde $\cong$ denota equivalencia observacional (mismos valores de registro, mismos efectos de memoria, mismos efectos secundarios).

**Mecanismo**:
El JIT inserta **Guardias** $G_1, \dots, G_k$.
$$
J(S) = \begin{cases}
\text{OptimizedExec}(S) & \text{si } G_1(S) \wedge \dots \wedge G_k(S) \\
\text{Deoptimize}(S) \to I(S) & \text{si no}
\end{cases}
$$
Dado que el respaldo es el propio intérprete, la equivalencia se mantiene incluso si fallan las suposiciones de optimización.

> **Verificación**: Ver `tests/cpp/jit_trace_equivalence_test.cpp`.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
