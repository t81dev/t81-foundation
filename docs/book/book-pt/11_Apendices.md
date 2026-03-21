# Capítulo 11: Apêndices

## 11.1 O Que Ainda Não Foi Implementado

**Status: Acompanhamento**

Embora o núcleo da T81VM e da TISC ISA sejam estáveis, vários recursos avançados permanecem em fases de desenvolvimento ativo ou pesquisa.

### 11.1.1 Fase 2: Cobertura Completa de `dmath`
Atualmente, o `dmath` fornece implementações determinísticas para aritmética básica (`+`, `-`, `*`, `/`) e principais transcendentais (`sin`, `cos`, `exp`, `log`).
*   **Faltando**: Funções trigonométricas inversas (`asin`, `acos`, `atan`) atualmente dependem da `libc` do hospedeiro (a menos que `T81_DETERMINISTIC` esteja definido, o que causa um trap).
*   **Faltando**: Funções hiperbólicas (`sinh`, `cosh`, `tanh`) são parciais.
*   **Plano**: Implementar expansões completas de séries de Taylor/Maclaurin para todas as funções matemáticas padrão em `include/t81/core/detail/dmath.hpp` para remover todas as dependências de `libm`.

### 11.1.2 Fase 3: Consenso Distribuído (Nível 4)
Opcodes de Nível 4 (`Gossip`, `Merge`) são especificados, mas a pilha de rede P2P subjacente é experimental.
*   **Faltando**: Descoberta robusta de pares (DHT).
*   **Faltando**: Mecanismo de resistência a Sybil (espaço reservado para Prova de Trabalho/Participação).
*   **Plano**: Integrar uma camada de rede endereçável por conteúdo (ex: libp2p ou Kademlia personalizado) para suportar fusão de estado descentralizada.

### 11.1.3 Fase 4: Formas Infinitas Completas (Nível 5)
O Nível 5 suporta colapso básico de Séries Geométricas.
*   **Faltando**: Continuação analítica geral para séries não geométricas.
*   **Faltando**: Soma simbólica de funções geradoras mais complexas.
*   **Plano**: Expandir `InfCollapse` para lidar com uma classe mais ampla de funções meromorfas.

## 11.2 Glossário

| Termo | Definição |
| :--- | :--- |
| **Axion** | O kernel de segurança do T81, responsável pela aplicação de políticas e registro de auditoria. |
| **CanonRef** | Uma referência canônica (hash SHA3-256) apontando para um objeto imutável no CanonFS. |
| **Nível Cognitivo** | Um nível de capacidade computacional (1=Simbólico a 5=Infinito). |
| **Portão de Determinismo** | O processo de CI (`t81lang_repro_gate`) que verifica a reprodutibilidade bit-exact do compilador. |
| **dmath** | Biblioteca de Matemática Determinística; uma implementação de software de aritmética de ponto flutuante. |
| **T81Float** | Um número de ponto flutuante ternário balanceado $(s, m, e)$. |
| **T81Int** | Um inteiro ternário balanceado de precisão arbitrária. |
| **TISC** | Computador de Conjunto de Instruções Ternárias; a linguagem de bytecode da T81VM. |
| **Trit** | Um dígito de base-3 $\{-1, 0, 1\}$. |
| **Tryte** | Uma sequência de trits (geralmente 4). |
| **Honestidade Estrutural** | O princípio de que um sistema não deve sintetizar informações ou ocultar aproximação. |

## 11.3 Links Úteis

*   **Repositório**: [github.com/t81-foundation/t81](https://github.com/t81-foundation/t81)
*   **Especificação**: diretório `spec/` no repositório.
*   **Painel de CI**: Aba GitHub Actions.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
