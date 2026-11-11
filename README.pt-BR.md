# Fundação T81

[English](README.md) · [简体中文](README.zh-CN.md) · [Español](README.es.md) · [Português (Brasil)](README.pt-BR.md) · [Русский](README.ru.md)

### Pesquisando representações alternativas, execução adaptativa e computação verificável.

**Ternário balanceado · IA/ML com consciência de representação · Execução determinística · Arquitetura experimental**

[Carta de Pesquisa](docs/research/RESEARCH_CHARTER.md) · [Roteiro de Pesquisa](docs/ROADMAP.md) · [Arquitetura](docs/explanation/ARCHITECTURE.md) · [Contexto de Pesquisa em IA](docs/ai-context/) · [Registro de superfície de determinismo](docs/governance/DETERMINISM_SURFACE_REGISTRY.md)

---

## O que é o T81?

**A Fundação T81 é uma arquitetura de computação experimental implementada e um programa de pesquisa científica de código aberto que investiga se representações computacionais alternativas — particularmente o ternário balanceado — podem fornecer vantagens mensuráveis sobre sistemas binários convencionais.**

O T81 é uma **arquitetura implementada e funcional**: ele define o Computador com Conjunto de Instruções Ternárias (TISC, do inglês *Ternary Instruction Set Computer*) como seu contrato de instrução computacional, com execução delimitada entre perfis demonstrada tanto em uma máquina virtual hospedada (T81VM) quanto em um ambiente de microkernel independente em metal (*bare-metal*) (Axion OS / TernaryOS em AArch64 QEMU).

O projeto estuda a computação em múltiplos domínios: representação matemática, algoritmos, execução de software, armazenamento, arquitetura de hardware e comportamento verificável do sistema. Sua arquitetura voltada para a pesquisa originou-se em matemática discreta, CWEB/Hanoi e investigações de sistemas numéricos alternativos antes de se expandir para subsistemas de linguagem, máquinas virtuais, instruções, políticas e garantia de pesquisa.

Uma segunda direção explicitamente experimental é a **execução adaptativa consciente da representação**: selecionar representações de dados, algoritmos e caminhos de execução para cargas de trabalho específicas, especialmente IA/ML, enquanto mede os custos totais de seleção, conversão, computação e precisão. Esta é uma hipótese arquitetônica, **não** uma vantagem de desempenho estabelecida do T81.

Sua pergunta central é:

> Sob quais condições a computação ternária pode superar, simplificar ou de outra forma aprimorar uma abordagem binária equivalente?

A resposta não é presumida.

Alguns experimentos demonstraram propriedades matemáticas e de representação úteis. Outros mostraram que as implementações binárias convencionais permanecem mais rápidas ou mais eficientes. Várias questões fundamentais permanecem em aberto.

O T81 existe para investigar esses limites sistematicamente.

**Esta é uma plataforma de pesquisa experimental, não uma afirmação de que um computador ternário superior já tenha sido construído.**

---

## Por que ternário?

A computação moderna utiliza predominantemente estados binários: 0 e 1.

A computação ternária utiliza três estados. No ternário balanceado, estes são representados como:

\[
T = \{-1,0,+1\}
\]

Isso cria possibilidades distintas para aritmética com sinal, representação numérica, codificação de estados, sistemas de decisão e arquitetura computacional.

Por exemplo:

| Propriedade | Relação matemática |
|---|---|
| Um trit | Três estados possíveis |
| Quatro trits | \(3^4 = 81\) estados |
| Seis trits | \(3^6 = 729\) estados |
| Informação por trit | \(\log_2 3 \approx 1.585\) bits |
| Dígito base-81 | Quatro trits agrupados |

Essas relações são fatos matemáticos.

Elas **não** comprovam que processadores ternários sejam mais rápidos, que a memória ternária seja fisicamente mais densa ou que algoritmos ternários consumam menos energia.

Essas são questões de engenharia separadas que exigem comparações rigorosas e pareadas em relação a alternativas binárias otimizadas.

O T81 investiga essa distinção.

---

## O que aprendemos?

A pesquisa é valiosa quando identifica tanto oportunidades quanto limitações.

**Resultado principal — uma comparação desfavorável com binário otimizado:** No experimento histórico de matriz-vetor AVX2 detalhado abaixo, o INT8 nativo atingiu **17.15 GFLOP/s**, contra **12.91 GFLOP/s** para o ternário empacotado de 2 bits (aproximadamente **32.8% a mais de rendimento** para a implementação nativa INT8). Este é um resultado específico para essa carga de trabalho, não uma classificação universal de bases; o T81 o publica com destaque porque um resultado desfavorável é evidência, não um defeito a ser ocultado.

A história experimental do T81 estabeleceu várias conclusões importantes.

### 1. Resultado negativo em AVX2: densidade de representação não é velocidade de execução

Dados ternários empacotados podem reduzir os requisitos de armazenamento.

No entanto, o software ainda precisa decodificar, manipular e executar essas representações em hardware binário convencional.

Em um experimento histórico de matriz-vetor AVX2:

| Implementação | Rendimento reportado |
|---|---:|
| INT8 nativo AVX2 | 17.15 GFLOP/s |
| Ternário empacotado de 2 bits AVX2 | 12.91 GFLOP/s |

A representação ternária compacta não superou a implementação convencional INT8 nessa carga de trabalho.

Este é um resultado experimental delimitado, não uma prova de que a computação ternária seja universalmente mais lenta.

[Explorar a pesquisa sobre ternário empacotado](docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md)

### 2. Representações adaptativas têm vantagens condicionais

A pesquisa sobre níveis de representação adaptativa identificou configurações em que uma representação ternária tem um custo modelado menor do que uma alternativa binária correspondente.

Essas vantagens dependem de premissas de carga de trabalho, sobrecarga de metadados, custos de conversão e limites de representação.

A pesquisa corrigida H04-R02 preserva essas descobertas condicionais enquanto deixa o desempenho físico explicitamente não resolvido.

[Ler as descobertas corrigidas de H04-R02](experiments/h04_r02_adaptive_representation/RESULTS.md)

### 3. Correção não estabelece otimização

A investigação experimental T81-AIOPT-001 examinou múltiplas representações de correspondência de padrões de quatro trits.

Sua implementação M1 passou em extensos testes de correção locais, mas a carga de trabalho experimental pôde ser reduzida a uma simples comparação ordinal.

O resultado demonstra por que um teste de correção exaustivo ainda pode ser insuficiente para avaliar uma vantagem computacional.

A questão relevante não é apenas se outra representação funciona, mas se a carga de trabalho escolhida pode distinguir diferenças significativas entre algoritmos concorrentes.

[Explorar T81-AIOPT-001](docs/research/aiopt/)

### 4. Execução delimitada entre perfis demonstrada (CPA-004/CPA-005)

**Classificação de evidência: determinismo e portabilidade delimitados, não desempenho comparativo ou maturidade de todo o sistema.** As garantias de determinismo são estritamente delimitadas pelo [Registro de superfície de determinismo](docs/governance/DETERMINISM_SURFACE_REGISTRY.md).

Pesquisas recentes no programa de Arquitetura Entre Perfis (CPA) demonstraram que o núcleo computacional do T81 executa de forma idêntica em ambientes de execução divergentes:

- **Artefato de Programa Canônico:** Um programa canônico congelado de 351 bytes composto por 27 registros canônicos de 13 bytes (SHA-256 `b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b`) executou um subconjunto admitido de 17 instruções TISC.
- **Equivalência entre Perfis:** Execuções limpas em três processos T81VM hospedados e três inicializações bare-metal limpas em AArch64 QEMU (após `ExitBootServices` do UEFI) coincidiram com um oráculo matemático derivado independentemente em **todos os 25 limites de observação** (estado inicial + 24 passos de instrução).
- **Correspondência Abrangente de Limites:** A equivalência foi verificada em todos os 81 valores e tags de registradores, 1.563 palavras e tags de memória lógica, flags e registradores de sistema (R75–R80), parando deterministicamente em PC=27, SP=283.
- **Distinção:** Demonstrar a correção da execução entre perfis prova que a arquitetura computacional funciona como especificado; **não** é evidência de superioridade ternária em desempenho, energia ou hardware sobre linhas de base binárias.

### 5. Programa de pesquisa ativo SCG-08: propagação de transporte e trabalho aritmético

O programa **SCG-08** investiga a propagação de transporte em ternário balanceado e trabalho aritmético de precisão múltipla sob uma métrica de custo ponderada congelada:

- **Carta de Pesquisa Fase 0:** Aceita e integrada (`SCG08_PHASE0_RESEARCH_CHARTER.md`).
- **Harness Aritmético Fase 1 Portão 1:** Implementado e verificado no ambiente isolado `experiments/scg08_arithmetic_work/`. Estabelece correção matemática através de 5 larguras de bits de operandos padronizadas (64, 128, 256, 512, 1024 bits) em relação a um oráculo desacoplado de inteiros exatos de precisão arbitrária em base $10^9$, reprodução determinística, contabilidade exata de operações primitivas e cumprimento estrito do orçamento de ensaios ($\le 1.000$ ensaios; 179 executados na suíte smoke).
- **Status do Portão 2 de Benchmarking:** **ESTRITAMENTE RETIDO.** Campanhas empíricas em larga escala e avaliações de hipóteses estatísticas permanecem não autorizadas até uma revisão explícita e autorização da governança. O código integrado do harness não constitui autorização experimental.

### 6. Computação física ternária permanece uma questão em aberto

A computação ternária emulada por software em CPUs binárias não é equivalente à computação em hardware ternário nativo.

A avaliação de dispositivos físicos exige evidências pareadas de área, energia, latência, confiabilidade, sensoriamento, codificação e circuitos periféricos.

A avaliação física permanece bloqueada pela indisponibilidade de dados pareados adequados de caracterização de dispositivos (Investigação 10A). O T81 não interpreta a ausência de evidências como prova de impossibilidade, nem trata a eficiência teórica da base como prova de vantagem física.

---

## A arquitetura de pesquisa do T81

O repositório contém vários subsistemas desenvolvidos para investigar representações computacionais e execução controlada.

```text
                     T81 FOUNDATION
               Arquitetura Implementada e Pesquisa
                             |
          +------------------+------------------+
          |                  |                  |
    REPRESENTAÇÃO        EXECUÇÃO           GARANTIA
          |                  |                  |
    Ternário balanceado  TISC ISA (Contrato)  Axion Policy
    Base-81 / Base-243   +-- T81VM (Hospedada)CanonFS
    Dados empacotados    +-- Axion (Baremetal)Evidência
    Níveis adaptativos   T81Lang Frontend     Proveniência
          |                  |                  |
          +------------------+------------------+
                             |
                   EXPERIMENTOS CONTROLADOS
                             |
               Linhas de base binárias convencionais
                             |
                  Verificação e comparação
                             |
                   Conclusões da pesquisa
```

### Contrato Computacional e Perfis de Execução

Na arquitetura T81:
- **TISC (Ternary Instruction Set Computer):** Serve como contrato central de instruções computacionais, definindo a semântica de opcodes, vetores de estado de 64 bits e regras de transição determinísticas.
- **T81VM Hospedada:** Perfil de execução hospedado para desenvolvimento em espaço do usuário, ferramentas, testes e reprodução determinística em sistemas operacionais convencionais (Linux, macOS, Windows).
- **Axion OS / TernaryOS Bare-Metal:** Perfil de execução *bare-metal* executado diretamente sobre hardware ou hipervisores (ex.: AArch64 QEMU), executando TISC sem dependências de SO anfitrião.
- **CanonFS:** Fornece identidade de objetos endereçada por conteúdo, armazenamento imutável de artefatos, proveniência criptográfica e contratos de intercâmbio RFC-00D1.
- **DAIOS (Deterministic AI OS):** Permanece como um conceito arquitetural prospectivo e proposta de arquitetura de destino (RFC-00D2) para governança em tempo de execução de agentes autônomos e IA de alto nível, distinto do motor de execução TISC implementado.

### Classificações de Maturidade de Subsistemas

O projeto distingue diferentes classificações de maturidade entre seus subsistemas. Uma implementação funcional não implica maturidade equivalente ou vantagem validada:

| Componente | Maturidade | Papel |
|---|---|---|
| **TISC** | Frozen | Contrato de instruções computacionais que rege a semântica de execução (v1.9.0) |
| **T81VM** | Stable | Perfil de execução hospedado para execução em máquina virtual e determinismo (delimitado pelo [Registro de superfície de determinismo](docs/governance/DETERMINISM_SURFACE_REGISTRY.md)) |
| **Axion Kernel** | Stable | Perfil de execução independente (*bare-metal* AArch64 microkernel/HAL) e autorização de políticas antes de efeitos colaterais |
| **T81Lang** | Stable | Frontend de linguagem e infraestrutura de compilação direcionada ao TISC |
| **T81 Core** | Stable | Tipos numéricos, contêineres e primitivas de representação |
| **CanonFS** | Stable | Artefatos endereçados por conteúdo e mecanismos de integridade |
| **DAIOS** | Concept | Proposta de arquitetura de destino para sistemas operacionais de IA determinísticos (RFC-00D2) |
| **Research experiments** | Experimental | Teste de hipóteses, validação de correção e medições comparativas |

---

## Primeiros Passos

### Pré-requisitos

A compilação do T81 requer:
- **CMake** 3.24 ou superior
- **Compilador compatível com C++20** (GCC 12+, Clang 15+, ou MSVC 2022)
- **Python** 3.10+ (para scripts de governança, validação e testes)
- *Opcional:* Sistema de compilação Ninja, QEMU AArch64 (para alvos bare-metal)

### Compilando o Projeto

Configurar e compilar o projeto principal:

```bash
# Configurar compilação fora da árvore de fontes
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Compilar alvos principais
cmake --build build --target t81 -j$(nproc)

# Executar a suíte de testes padrão
ctest --test-dir build --output-on-failure
```

### Executando Experimentos (Opcional)

Experimentos de pesquisa e harnesses (como o SCG-08) estão protegidos por opções de configuração do CMake:

```bash
# Configurar com harnesses experimentais habilitados
cmake -B build -S . -DT81_BUILD_EXPERIMENTS=ON -DCMAKE_BUILD_TYPE=Release

# Compilar e executar a validação aritmética do SCG-08 Portão 1
cmake --build build --target t81_exp_scg08_01_test
ctest --test-dir build -R scg08 --output-on-failure

# Validar o esquema de evidências
python3 scripts/ci/validate_scg08_gate1_evidence.py
```

---

## Metodologia de pesquisa

O T81 adota uma abordagem orientada a evidências.

Todo experimento comparativo deve responder a cinco perguntas:

1. **Qual é a hipótese?** Definir uma proposição falseável.
2. **Qual é a linha de base?** Comparar com uma alternativa convencional funcionalmente equivalente e devidamente otimizada.
3. **O que é medido?** Separar correção, armazenamento, latência, rendimento, energia e tráfego de memória.
4. **Quais são as limitações?** Identificar premissas, hardware hospedeiro, escopo da carga de trabalho e dependências não resolvidas.
5. **Qual conclusão é justificada?** Distinguir resultados suportados, não suportados, inconclusivos, bloqueados e não testados.

O projeto distingue estritamente entre:

**Implementado → Testado → Medido → Comparativamente estabelecido**

Esses são níveis probatórios diferentes, não termos intercambiáveis. Um resultado experimental não deve afirmar mais do que suas evidências comprovam.

---

## Navegação na documentação

| Diretório / Arquivo | Descrição |
|---|---|
| [`docs/research/`](docs/research/) | Investigações científicas principais e conclusões experimentais |
| [`experiments/`](experiments/) | Implementações experimentais delimitadas, harnesses e registros de evidência |
| [`spec/`](spec/) | Especificações de conjuntos de instruções e catálogo de RFCs |
| [`docs/governance/`](docs/governance/) | Registros de superfície de governança, modelo de autoridade e [Registro de superfície de determinismo](docs/governance/DETERMINISM_SURFACE_REGISTRY.md) |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | Roteiro de colaboradores e sequenciamento de pesquisa ativa |
| [`docs/ai-context/`](docs/ai-context/) | Documentação fidedigna de continuidade e revisões de estado para colaboradores de IA |
| [`include/t81/`](include/t81/) | Cabeçalhos C++ públicos e contratos fundamentais |
| [`vm/`](vm/) | Implementação da máquina virtual hospedada (T81VM) |
| [`kernel/axion/`](kernel/axion/) | Motor de políticas do Axion e kernel de governança |
| [`fs/`](fs/) | Driver de armazenamento e intercâmbio endereçado por conteúdo CanonFS |
| [`PROJECT_STRUCTURE.md`](PROJECT_STRUCTURE.md) | Mapa abrangente de diretórios e layout do repositório |
| [`SUPPORT_TAXONOMY.md`](SUPPORT_TAXONOMY.md) | Taxonomia de níveis de suporte, limites de plataformas e perfis |

---

## O que o T81 não afirma

O T81 não afirma:

- Superioridade universal da computação ternária sobre a computação binária.
- Superioridade demonstrada ou viabilidade comercial do silício ternário nativo.
- Uma vantagem de desempenho geral da arquitetura integrada T81 sobre runtimes convencionais otimizados.
- Um otimizador de IA adaptativo validado que supere linhas de base fortes de política fixa ou de roteamento algorítmico.
- Que os números dos estudos de caso dos PDFs históricos constituam medições reproduzidas independentemente.
- Que pesos de modelos ternários acelerem intrinsecamente a inferência de IA.
- Que a execução determinística ou a aplicação de políticas exijam uma máquina virtual ternária.
- Que um experimento gerado por IA seja considerado validado independentemente apenas porque seus testes locais passaram.
- Que o código de harness integrado ou a qualificação de Portão 1 autorizem a realização de benchmarks da Fase 2 sem aprovação explícita de governança.

Esses limites são essenciais para a integridade científica da pesquisa.

---

## A questão permanece em aberto

A computação binária domina a infraestrutura física e de software atual por razões sólidas de engenharia.

Esse domínio não elimina a importância de investigar representações alternativas, modelos computacionais ou novos substratos de hardware.

O propósito do T81 é determinar onde essas alternativas são úteis — e onde não são.

**Não estamos construindo uma conclusão e buscando evidências para apoiá-la.**

**Estamos construindo a base experimental necessária para descobrir a conclusão.**

---

**Fundação T81** — Arquitetura de Computação Experimental e Pesquisa de Sistemas

[Licença: Apache 2.0](LICENSE) · [GitHub](https://github.com/t81dev/t81-foundation)
