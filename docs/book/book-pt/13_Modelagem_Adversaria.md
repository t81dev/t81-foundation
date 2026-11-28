# Capítulo 13: Modelagem Adversária e Ataques de Determinismo

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Capítulo 13: Modelagem Adversária e Ataques de Determinismo](#capítulo-13-modelagem-adversária-e-ataques-de-determinismo)
  - [13.1 Modelo de Ameaça](#131-modelo-de-ameaça)
  - [13.2 Ataques de Nível de Compilador](#132-ataques-de-nível-de-compilador)
  - [13.3 Vetores de Ataque de VM e GC](#133-vetores-de-ataque-de-vm-e-gc)
  - [13.4 Ataques CanonFS e Hash](#134-ataques-canonfs-e-hash)
  - [13.5 Ataque de Viagem no Tempo de Nível Distribuído](#135-ataque-de-viagem-no-tempo-de-nível-distribuído)
  - [13.6 Modelo de Post-Mortem de Violação de Determinismo](#136-modelo-de-post-mortem-de-violação-de-determinismo)

<!-- T81-TOC:END -->


## 13.1 Modelo de Ameaça

**Status: Teórico**

O T81 assume um ambiente hostil. O **Hospedeiro** (SO, Hardware, Operador) é considerado um adversário que pode tentar:
1.  **Introduzir Entropia**: Injetar aleatoriedade na execução determinística.
2.  **Forjar Estados**: Alegar que uma computação atingiu o estado $S'$ quando na verdade atingiu $S$.
3.  **Negar Serviço**: Consumir recursos infinitos.
4.  **Vazar Informação**: Expor dados privados via canais laterais.

## 13.2 Ataques de Nível de Compilador

**Vetor de Ataque**: "Fonte Cavalo de Troia" / Homóglifos.
**Descrição**: Um invasor usa caracteres de controle Unicode (ex: Right-to-Left Override) para fazer o código-fonte parecer diferente para humanos do que para o compilador.
**Mitigação**: O Lexer T81 impõe um subconjunto estrito de UTF-8. Caracteres de controle e não imprimíveis são rejeitados durante a tokenização.

**Vetor de Ataque**: Reordenação de Tokens / Deriva de Otimização.
**Descrição**: Um compilador malicioso pode reordenar instruções de uma maneira que preserve a semântica em uma arquitetura, mas não em outra (ex: devido a diferenças no modelo de memória).
**Mitigação**: O Compilador T81 emite uma **AST Canônica**. A fase de geração de IR é determinística e agnóstica de plataforma. O `t81lang_repro_gate` verifica se a saída do compilador é idêntica bit-a-bit entre execuções.

## 13.3 Vetores de Ataque de VM e GC

**Vetor de Ataque**: Rowhammer / Bit Flips.
**Descrição**: Ataques físicos na DRAM para inverter bits em memória sensível (ex: mudando um veredito `Deny` para `Allow`).
**Mitigação**: O T81 usa **Handles Opacos** e **Segmentação de Memória**. Estruturas críticas do kernel são armazenadas em páginas isoladas (onde possível) e validadas por checksums. No entanto, software não pode mitigar totalmente falhas de hardware sem memória ECC.

**Vetor de Ataque**: Não-determinismo do Coletor de Lixo.
**Descrição**: Se o GC rodar com base no tempo do relógio de parede ou pressão de memória, traces de execução divergirão entre execuções.
**Mitigação**: O GC do T81 é **determinístico**. Ele é acionado unicamente por contagens de alocação (`bytes_allocated > threshold`). Isso garante que pausas do GC aconteçam na mesma instrução exata em cada execução.

**Vetor de Ataque**: Canais Laterais de Tempo.
**Descrição**: Observar o tempo que leva para computar uma função (ex: exponenciação modular) para inferir chaves secretas.
**Mitigação**: O `dmath` visa implementações de tempo constante para primitivas criptográficas, mas aritmética de propósito geral não é garantida como tempo constante. O T81 foca em determinismo *funcional*, não determinismo *temporal* (ciclos constantes).

## 13.4 Ataques CanonFS e Hash

**Vetor de Ataque**: Colisão de Hash / Pré-imagem.
**Descrição**: Encontrar duas entradas diferentes $A \neq B$ tal que $Hash(A) = Hash(B)$.
**Mitigação**: O T81 usa **SHA3-256** (Keccak), que é resistente a ataques de extensão de comprimento e ataques de colisão. As regras de serialização canônica (chaves ordenadas, floats normalizados) minimizam a superfície de ataque reduzindo o espaço de entrada de objetos válidos.

## 13.5 Ataque de Viagem no Tempo de Nível Distribuído

**Vetor de Ataque**: Retenção de Estado / Replay.
**Descrição**: No Nível 4, um nó computa uma transição de estado $S_t \to S_{t+1}$ mas a retém, liberando-a mais tarde para invalidar o trabalho de outros nós (um equivalente de "mineração egoísta").
**Mitigação**:
1.  **Carimbos de Data/Hora Lamport**: Cada transição deve seguir causalmente a anterior.
2.  **Quóruns de Consenso**: Um estado só é finalizado quando assinado por $2/3$ do cluster cognitivo.
3.  **Fusão de Trace**: Se ramos divergirem, a função de fusão determinística resolve conflitos com base no trabalho computacional total (comprimento do trace).

## 13.6 Modelo de Post-Mortem de Violação de Determinismo

**Status: Processo**

Se uma violação de determinismo for detectada (ou seja, `t81lang_repro_gate` falhar), o seguinte procedimento é invocado:

1.  **Isolamento**: Identificar as entradas divergentes e o índice de instrução específico onde o trace $A$ difere do trace $B$.
2.  **Reprodução**: Criar um caso de reprodução mínima (`repro.t81`).
3.  **Análise**:
    *   É um bug do compilador? (Verificar dump da AST)
    *   É um bug da VM? (Verificar implementação `dmath`)
    *   É um problema de biblioteca do hospedeiro? (Verificar ligação `libc`)
4.  **Remediação**:
    *   Corrigir `dmath` para substituir o fallback do hospedeiro.
    *   Atualizar `t81lang_repro_gate` com o novo teste de regressão.
5.  **Divulgação**: Publicar um "Aviso de Determinismo" (se sistemas de produção forem afetados).

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
