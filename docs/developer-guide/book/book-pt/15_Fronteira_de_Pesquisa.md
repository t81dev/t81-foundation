# Capítulo 15: Fronteira de Pesquisa

## 15.1 Aceleração de Hardware Ternário

**Status: Pesquisa**

Embora o T81 rode eficientemente em hardware binário via emulação (armazenamento `packed-trit`), hardware ternário nativo poderia oferecer vantagens significativas.
*   **Densidade de Informação**: Trits carregam $\approx 1.58$ bits de informação. Uma palavra de 27 trits cabe em 64 bits, mas representa $7.6 \times 10^{12}$ valores, excedendo em muito $2^{32}$.
*   **Eficiência Aritmética**: A adição ternária balanceada reduz o número médio de operações de transporte (carry) comparado ao binário.
*   **Eficiência Energética**: Pesquisas sugerem que portas lógicas ternárias podem ser mais eficientes em energia para certas cargas de trabalho de IA (ex: Redes Neurais Esparsas).

**Caminho a Seguir**:
1.  **Emulação FPGA**: Portar o núcleo TISC para Verilog/VHDL visando Xilinx Artix-7, implementando ALUs ternárias nativas.
2.  **Design ASIC**: Colaborar com projetos de silício open-source (OpenROAD) para fazer o tape-out de um coprocessador ternário de prova de conceito.

## 15.2 Caminhos de Verificação Formal

**Status: Pesquisa**

Atualmente, o T81 conta com **Testes Baseados em Propriedades** (estilo QuickCheck) e **Fuzzing** para garantir a correção. O próximo passo são **Provas Formais**.
*   **Coq / Isabelle**: Definir a semântica formal do TISC em um assistente de prova.
*   **Compilação Certificada**: Provar que o Compilador T81 preserva a semântica de Fonte $\to$ AST $\to$ IR $\to$ Bytecode.
*   **Correção JIT**: Provar que os passes de otimização de trace (Dobra de Constantes, Eliminação de Código Morto) são transformações que preservam a semântica.

## 15.3 CanonFS como Substrato Merkle

**Status: Conceito**

O CanonFS atualmente lida com blobs estáticos (pesos, código). Pesquisas futuras visam torná-lo um sistema de arquivos totalmente **Mutável-via-Imutável**, similar ao Git ou IPFS, mas otimizado para cargas de trabalho de IA.
*   **Modelos Versionados**: `model:v1` é um ponteiro para `hash1`. `model:v2` é um ponteiro para `hash2`.
*   **Deduplicação**: Deduplicar camadas automaticamente entre diferentes redes neurais.
*   **Carregamento Preguiçoso**: Transmitir fatias de tensor sob demanda pela rede, verificadas por provas Merkle.

## 15.4 Inferência de IA Determinística em Escala

**Status: Desenvolvimento Ativo**

O objetivo final do T81 é **IA Soberana**: rodar grandes modelos de linguagem (LLMs) deterministicamente.
*   **Problema**: GPUs atuais (CUDA) são não-determinísticas devido à ordem de redução paralela e peculiaridades de hardware.
*   **Solução**: A biblioteca `dmath` e de tensores do T81 fornece uma implementação de referência lenta, mas correta.
*   **Otimização**: Implementar algoritmos de redução paralela determinísticos (ex: soma baseada em árvore) para permitir execução multi-core sem sacrificar a exatidão bit-exact.
*   **Aplicação**: Redes de IA descentralizadas onde nós devem alcançar consenso sobre a saída de um prompt LLM.

---
**Canonical Source**: /book (English)
**Source Version**: Phase 1 Expansion
**Last Synced**: 2026-02-23
**Translation Status**: Needs Update
---
