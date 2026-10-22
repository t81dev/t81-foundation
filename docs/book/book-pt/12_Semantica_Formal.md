# Capítulo 12: Semântica Formal do TISC e T81VM

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Capítulo 12: Semântica Formal do TISC e T81VM](#capítulo-12-semântica-formal-do-tisc-e-t81vm)
  - [12.1 Semântica Operacional](#121-semântica-operacional)
    - [12.1.1 A Função de Transição $\delta$](#1211-a-função-de-transição-$\delta$)
    - [12.1.2 Regras de Inferência](#1212-regras-de-inferência)
      - [**Regra: Busca-Decodificação**](#**regra-busca-decodificação**)
      - [**Regra: Aritmética (Add)**](#**regra-aritmética-add**)
      - [**Regra: Verificação de Política (Axion)**](#**regra-verificação-de-política-axion**)
  - [12.2 Função de Transição Algébrica](#122-função-de-transição-algébrica)
  - [12.3 Sistema de Reescrita de Canonicalização](#123-sistema-de-reescrita-de-canonicalização)
    - [12.3.1 Formas Normais](#1231-formas-normais)
  - [12.4 Esboços de Prova de Determinismo](#124-esboços-de-prova-de-determinismo)
    - [Teorema 1: Independência de Hardware do `dmath`](#teorema-1-independência-de-hardware-do-`dmath`)
    - [Teorema 2: Segurança Axion](#teorema-2-segurança-axion)
  - [12.5 Equivalência Intérprete vs Trace-JIT](#125-equivalência-intérprete-vs-trace-jit)

<!-- T81-TOC:END -->


## 12.1 Semântica Operacional

**Status: Formalizando**

A Máquina Virtual T81 (T81VM) é formalmente definida como um **Autômato Finito Determinístico com Memória Infinita** (DFA-IM). Sua semântica é especificada usando **Semântica Operacional de Passo Pequeno** (SSOS).

### 12.1.1 A Função de Transição $\delta$

O núcleo da VM é a função de transição $\delta$, que mapeia um estado $S$ para um estado sucessor $S'$:
$$
\delta: S \to S'
$$
onde $S = (\mathbf{R}, \mathbf{M}, \mathbf{K}, \mathbf{\Phi}, PC)$ como definido no Capítulo 3.

A execução de um programa é a aplicação repetida de $\delta$ até que um estado terminal seja alcançado:
$$
S_0 \xrightarrow{\delta} S_1 \xrightarrow{\delta} \dots \xrightarrow{\delta} S_n
$$
onde $S_n$ é um **Estado de Parada** (`STOP` ou `TRAP`).

### 12.1.2 Regras de Inferência

Usamos regras de inferência da forma:
$$
\frac{\text{Premissa}}{\text{Conclusão}}
$$

#### **Regra: Busca-Decodificação**
$$
\frac{Code[PC] = \text{Op} \quad \text{Arity}(\text{Op}) = k}{S \to (PC \leftarrow PC + 1 + k, \dots)}
$$

#### **Regra: Aritmética (Add)**
$$
\frac{Code[PC] = \text{Add } r_d, r_a, r_b \quad v_a = R[r_a] \quad v_b = R[r_b] \quad v' = v_a + v_b}{S \to S[R[r_d] \leftarrow v', \Phi \leftarrow \text{Flags}(v')]}
$$
*Invariante*: A adição $+$ é a adição ternária balanceada definida em `dmath`.

#### **Regra: Verificação de Política (Axion)**
Antes de qualquer transição $\delta$ ser aplicada, a função Axion $\alpha$ deve aprovar:
$$
\frac{\alpha(S, \text{Op}) = \text{Deny}}{S \to S_{\text{Trap}}(\text{SecurityFault})}
$$
$$
\frac{\alpha(S, \text{Op}) = \text{Allow}}{S \to \delta_{\text{Op}}(S)}
$$

## 12.2 Função de Transição Algébrica

**Status: Teórico**

Também podemos ver o estado da VM como um elemento de uma estrutura algébrica (um Anel de Estados). Isso nos permite provar propriedades sobre a execução.

Seja $\mathcal{S}$ o conjunto de todos os estados de máquina válidos.
Seja $\mathcal{O}$ o conjunto de todas as sequências de opcodes válidas.
Definimos uma ação de monoide $\cdot : \mathcal{O} \times \mathcal{S} \to \mathcal{S}$.

**Propriedade: Associatividade da Execução**
$$
(Op_1 \cdot Op_2) \cdot S \equiv Op_1 \cdot (Op_2 \cdot S)
$$

**Propriedade: Determinismo**
$$
\forall S \in \mathcal{S}, Op \in \mathcal{O}: \exists! S' \text{ tal que } S \xrightarrow{Op} S'
$$

## 12.3 Sistema de Reescrita de Canonicalização

**Status: Implementado**

O compilador e runtime T81 empregam um **Sistema de Reescrita** para garantir formas canônicas. Isso é crítico para a estabilidade do `CanonHash81`.

### 12.3.1 Formas Normais
Um valor $v$ está na **Forma Normal** se não puder ser reescrito mais pelas regras de redução.

**Regra: Normalização de Float**
$$
\text{Float}(s, m, e) \xrightarrow{} \text{Float}(s, m', e')
$$
onde $m'$ não tem trits zero à esquerda (a menos que $m'=0$).

**Regra: Ordenação Topológica de Grafo**
Dado um grafo $G=(V, E)$, reescrevemos os índices de nó $V \to \{0 \dots |V|-1\}$ tal que:
$$
i < j \implies \text{TopoRank}(v_i) < \text{TopoRank}(v_j)
$$
Isso garante que grafos isomorfos serializem para fluxos de bytes idênticos.

## 12.4 Esboços de Prova de Determinismo

### Teorema 1: Independência de Hardware do `dmath`

**Afirmação**: Para quaisquer entradas `T81Float` $x, y$ e operação $\odot \in \{+, -, *, /\}$, o resultado $z = x \odot y$ é idêntico em bits nas arquiteturas $A_1$ (x86) e $A_2$ (ARM).

**Esboço de Prova**:
1.  `T81Float` é composto de mantissa e expoente `T81Int`.
2.  Operações `T81Int` usam apenas aritmética inteira (add, sub, mul, div, mod).
3.  Aritmética inteira é padronizada pelo padrão C++ (Complemento de Dois) e é isomorfa ao anel inteiro abstrato $\mathbb{Z}_{2^{64}}$.
4.  Como $A_1$ e $A_2$ implementam $\mathbb{Z}_{2^{64}}$ corretamente (verificado por testes de conformidade), e a lógica `dmath` é puramente uma composição dessas operações inteiras, o resultado deve ser idêntico. $\blacksquare$

### Teorema 2: Segurança Axion

**Afirmação**: Se a Política $P$ proíbe opcodes no conjunto $\mathcal{F}$, então nenhum estado alcançável a partir de $S_0$ terá sido produzido por um opcode $op \in \mathcal{F}$.

**Esboço de Prova**:
1.  A função de transição $\delta$ é protegida: $\delta(S) = \text{se } \alpha(S, Op) \text{ então } \dots \text{ senão } \text{Trap}$.
2.  A função $\alpha(S, Op)$ retorna `Deny` se $Op \in \mathcal{F}$ (por definição de aplicação de Política).
3.  Portanto, o ramo "senão" é tomado, levando a um estado Trap, não um estado produzido pela execução de $Op$.
4.  Por indução, nenhuma transição de estado válida usa $Op$. $\blacksquare$

## 12.5 Equivalência Intérprete vs Trace-JIT

**Status: Em Progresso**

A otimização Trace-JIT é uma transformação que preserva a semântica.

Seja $I(S)$ a função do intérprete.
Seja $J(S)$ a função compilada por JIT para um trace $T$.

**Requisito**:
$$
\forall S: J(S) \cong I(S)
$$
onde $\cong$ denota equivalência observacional (mesmos valores de registro, mesmos efeitos de memória, mesmos efeitos colaterais).

**Mecanismo**:
O JIT insere **Guardas** $G_1, \dots, G_k$.
$$
J(S) = \begin{cases}
\text{OptimizedExec}(S) & \text{se } G_1(S) \wedge \dots \wedge G_k(S) \\
\text{Deoptimize}(S) \to I(S) & \text{caso contrário}
\end{cases}
$$
Como o fallback é o próprio intérprete, a equivalência é mantida mesmo se as suposições de otimização falharem.

> **Verificação**: Veja `tests/cpp/jit_trace_equivalence_test.cpp`.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
