# Capítulo 8: Verificação e Auditoria

## 8.1 Metodologia de Verificação Formal

**Status: Implementado**

O T81 emprega uma estratégia de verificação multicamadas, movendo-se de testes unitários para verificação formal de propriedades.

### 8.1.1 A Pilha de Verificação

1.  **Testes Unitários (L1)**: Verificam funções e classes individuais.
    *   Exemplo: `tests/cpp/test_T81Int.cpp` verifica `T81Int::add(1, 1) == 1T`.
2.  **Testes Baseados em Propriedades (L2)**: Geram entradas aleatórias para verificar invariantes (estilo QuickCheck).
    *   Exemplo: `tests/cpp/test_property_invariants.cpp` verifica $\forall a, b: a + b = b + a$.
3.  **Fuzz Testing (L3)**: Fornece entradas malformadas ao parser e VM para garantir resiliência a falhas.
    *   Exemplo: `tests/cpp/frontend_fuzz_test.cpp` usa `libFuzzer`.
4.  **Testes de Integração (L4)**: Executam programas completos end-to-end e verificam a saída.
    *   Exemplo: `tests/cpp/e2e_arithmetic_test.cpp`.
5.  **Portões de Determinismo (L5)**: Verificam reprodutibilidade bit-exact em todos os ambientes.

## 8.2 A Matriz de Auditoria Formal

**Status: Auditável**

Esta matriz mapeia os requisitos do sistema para seus artefatos de verificação específicos.

| Requisito | ID | Artefato / Teste | Status |
| :--- | :--- | :--- | :--- |
| **Determinismo Estrito** | REQ-001 | `scripts/ci/t81lang_repro_gate.py` | PASS |
| **Aritmética Ternária** | REQ-002 | `tests/cpp/ternary_arith_test.cpp` | PASS |
| **Aplicação de Política** | REQ-003 | `tests/cpp/test_ethics.cpp` | PASS |
| **Segurança de Memória** | REQ-004 | `tests/cpp/vm_bounds_test.cpp` | PASS |
| **Integridade de Trace** | REQ-005 | `tests/cpp/axion_log_determinism_test.cpp` | PASS |
| **Hash Canônico** | REQ-006 | `tests/cpp/canonfs_driver_test.cpp` | PASS |

## 8.3 Testes Baseados em Propriedades

**Status: Implementado**

Usamos **Testes Baseados em Propriedades** para provar que as leis algébricas se mantêm para nossos tipos numéricos personalizados.

### 8.3.1 Os Axiomas do Anel
O arquivo `tests/cpp/test_property_invariants.cpp` verifica programaticamente que `T81Int` e `T81Float` satisfazem os axiomas de um Anel:
1.  **Associatividade**: $(a + b) + c = a + (b + c)$
2.  **Comutatividade**: $a + b = b + a$
3.  **Identidade**: $a + 0 = a$
4.  **Inverso**: $a + (-a) = 0$
5.  **Distributividade**: $a \times (b + c) = (a \times b) + (a \times c)$

Esses testes executam milhões de iterações com entradas aleatórias (incluindo casos extremos como `MaxInt`, `MinInt`, `Zero`) para fornecer alta confiança estatística na correção.

## 8.4 O Portão de Determinismo (Determinism Gate)

**Status: Ativo**

O **Portão de Determinismo** (`scripts/ci/t81lang_repro_gate.py`) é a verificação final antes de qualquer código ser mesclado.

### 8.4.1 Mecânica
1.  **Consistência de Compilação**: Compila um conjunto de 5+ programas de teste canônicos (`*.t81`) duas vezes. Ele compara a saída binária do Passe A e Passe B. Qualquer diferença de bit causa falha imediata.
2.  **Hash Agregado**: Computa um hash SHA-256 do bytecode gerado para todos os testes.
3.  **Verificação de Baseline**: Este hash agregado é comparado com um "Golden Hash" registrado. Se a lógica do compilador mudar (resultando em bytecode diferente), o hash mudará e o build falhará. Isso garante que mudanças no compilador sejam intencionais e auditadas.

### 8.4.2 Rastreabilidade
Quando o portão passa, ele exibe:
```text
[PASS] T81Lang gates passed: fixtures=5 hash=a7f92b...
```
Este hash é registrado nos logs de CI, fornecendo um registro imutável do estado do compilador para aquele release.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
