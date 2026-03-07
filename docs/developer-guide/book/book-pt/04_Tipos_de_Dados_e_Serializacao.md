# Capítulo 4: Tipos de Dados e Serialização

## 4.1 Tipos Primitivos

**Status: Implementado e Testado**

A arquitetura T81 é construída sobre uma fundação de primitivas ternárias balanceadas. Esses tipos são projetados para serem eficientemente simulados em hardware binário enquanto mantêm as propriedades matemáticas da lógica de base-3.

### 4.1.1 Trits e Trytes

*   **Trit**: O átomo fundamental de informação, assumindo valores $\{-1, 0, 1\}$.
    *   Também denotado como $\{T, 0, 1\}$ ou $\{-, 0, +\}$.
    *   Conteúdo de informação: $\log_2 3 \approx 1.58$ bits.
*   **Tryte**: Uma sequência de trits. O "equivalente a Byte" padrão é o Tryte de 4 trits ($3^4 = 81$ valores).
*   **Word**: Uma palavra de máquina T81 padrão é de 27 trits ($3^{27} \approx 7.6 \times 10^{12}$), que cabe confortavelmente em um inteiro de 64 bits.

**Representação Empacotada (T3_K)**:
Para armazenamento eficiente, trits são frequentemente empacotados usando um código de 2 bits:
*   `00` $\to$ 0
*   `01` $\to$ 1
*   `10` $\to$ -1 (T)
*   `11` $\to$ Não utilizado / Preenchimento

### 4.1.2 T81Int (Inteiro de Precisão Arbitrária)
`T81Int` é um tipo inteiro de largura variável. Diferente de inteiros binários que usam Complemento de Dois para valores negativos, inteiros ternários balanceados são simétricos.

*   **Intervalo**: Intervalo simétrico $[-\frac{3^N-1}{2}, +\frac{3^N-1}{2}]$.
*   **Normalização**: Zeros à esquerda são estritamente proibidos na forma serializada canônica. A única representação válida para Zero é um único trit `0`.

## 4.2 T81Float e dmath

**Status: Implementado (Core)**

A aritmética de ponto flutuante é a principal fonte de não-determinismo na computação multiplataforma. O T81 substitui floats de hardware IEEE-754 por um formato totalmente definido por software: **`T81Float`**.

### 4.2.1 Definição Canônica
Um `T81Float<M, E>` é uma tupla $(s, m, e)$ representando o valor:
$$
V = s \times m \times 3^{e - \text{bias}}
$$
onde:
*   $s \in \{-1, 1\}$ é o sinal (armazenado como um trit).
*   $m$ é a mantissa, um inteiro de $M$-trits normalizado tal que o trit mais significativo seja não-zero (a menos que $V=0$).
*   $e$ é o expoente, um inteiro de $E$-trits.
*   $\text{bias} = \frac{3^E - 1}{2}$.

**Valores Especiais**:
*   **Zero**: $e = 0, m = 0$.
*   **Infinito**: $e = e_{\max}, m = 0$.
*   **NaE (Não é uma Entidade)**: $e = e_{\max}, m \neq 0$. (Equivalente a NaN).

### 4.2.2 O Backend dmath
Para garantir resultados bit-exact em x86, ARM e RISC-V, o T81 implementa **`dmath`** (Matemática Determinística).
*   **Aritmética**: `Add`, `Sub`, `Mul`, `Div` são implementados usando matemática inteira nas mantissas, com regras de arredondamento precisas (ties-to-even) aplicadas em software.
*   **Transcendentais**: Funções como `sin`, `cos`, `exp`, `log` são computadas usando **expansões de Série de Taylor** com um número fixo de iterações e precisão constante fixa. Isso elimina a dependência da `libm` do SO hospedeiro, que varia entre glibc, musl e MSVC.

## 4.3 Tensores e Layouts Canônicos

**Status: Implementado e Testado**

Tensores (`T81Tensor`) são os cavalos de batalha dos níveis cognitivos. Para suportar execução eficiente e hash canônico, eles seguem um layout estrito.

### 4.3.1 Layout de Memória
Tensores são armazenados em ordem **Row-Major** (linha principal, estilo C), não Column-Major (estilo Fortran).
*   **Forma (Shape)**: Um vetor de dimensões $(d_0, d_1, \dots, d_n)$.
*   **Stride**: Calculado como $s_i = \prod_{j=i+1}^n d_j$.
*   **Alinhamento**: Dados de tensor são alinhados a limites de 64 bytes no segmento de memória `Tensor` para facilitar carregamento SIMD (AVX-512 / NEON) onde seguro.

### 4.3.2 Serialização (.t81w)
O formato `.t81w` (Pesos T81) é o container padrão para persistir modelos de tensor. É projetado para ser **amigável ao mmap** e **canônico**.

**Estrutura Binária (Versão 2)**:
1.  **Cabeçalho Mágico**: `0x54383157` ("T81W").
2.  **Versão**: `0x02`.
3.  **Tabela de Conteúdos**: Uma lista de tuplas `(Hash, Offset, Length)`, ordenada por Hash.
4.  **Dados de Blob**: Dados de tensor contíguos, preenchidos para alinhamento de 64 bytes.

### 4.3.3 Quantização (T3_K)
O T81 suporta um formato de quantização ternária nativa chamado **T3_K**.
*   **Tamanho do Bloco**: $K$ trits (tipicamente 64 ou 128).
*   **Representação**: Cada valor é quantizado para $\{-1, 0, 1\}$.
*   **Escala**: Cada bloco tem um fator de escala (T81Float) para aproximar a magnitude original.

## 4.4 Regras de Serialização Canônica

**Status: Implementado**

Para garantir hash consistente (para `CanonRef`), todos os dados devem ser normalizados antes da serialização. O serializador impõe um **Mapeamento Bijetivo** entre valores abstratos e sequências de bytes.

1.  **Inteiros (T81Int)**:
    *   Remover zeros à esquerda.
    *   Zero é codificado como um único byte `0x00` (assumindo codificação específica).
2.  **Floats (T81Float)**:
    *   Devem ser normalizados (deslocamento máximo à esquerda).
    *   Zero Negativo é estritamente proibido; deve ser convertido para Zero Positivo.
    *   Payloads NaE são zerados (sem diferenciação de bit "sinalização" vs "silencioso").
3.  **Coleções**:
    *   **Mapas/Dicionários**: Chaves devem ser ordenadas lexicograficamente por sua representação binária canônica.
    *   **Conjuntos**: Elementos devem ser ordenados.
4.  **Grafos**:
    *   Nós são reindexados por uma ordenação topológica canônica. Se o grafo tiver ciclos, uma regra de desempate determinística (baseada em pesos de aresta) é aplicada.

> **Verificação**: `tests/cpp/test_property_invariants.cpp` verifica essas propriedades via testes baseados em propriedades (fuzzing), garantindo que $Serialize(Deserialize(Serialize(X))) \equiv Serialize(X)$.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
