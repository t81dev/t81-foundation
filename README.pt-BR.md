# Fundação T81

### Pesquisa de representações alternativas, execução adaptativa e computação verificável.

**Ternário balanceado · IA/ML com consciência de representação · Execução determinística · Arquitetura experimental**

[Carta de Pesquisa](https://www.google.com/search?q=docs/research/RESEARCH_CHARTER.md) · [Roteiro de Pesquisa](https://www.google.com/search?q=docs/ROADMAP.md) · [Arquitetura](https://www.google.com/search?q=docs/explanation/ARCHITECTURE.md) · [Contexto de Pesquisa em IA](https://www.google.com/search?q=docs/ai-context/)

---

## O que é o T81?

**A Fundação T81 é uma arquitetura de computação experimental implementada e um programa de pesquisa científica de código aberto que investiga se representações computacionais alternativas — particularmente o ternário balanceado — podem fornecer vantagens mensuráveis sobre sistemas binários convencionais.**

O T81 é uma **arquitetura implementada e funcional**: ele define o Computador com Conjunto de Instruções Ternárias (TISC, do inglês *Ternary Instruction Set Computer*) como seu contrato de instrução computacional, com execução delimitada e multiplataforma demonstrada tanto em uma máquina virtual hospedada (T81VM) quanto em um ambiente de microkernel independente em metal *bare-metal* (Axion OS / TernaryOS em AArch64 QEMU).

O projeto estuda a computação em vários níveis: representação matemática, algoritmos, execução de software, armazenamento, arquitetura de hardware e comportamento de sistemas verificáveis. Sua arquitetura voltada para a pesquisa originou-se em matemática discreta, CWEB/Hanoi e investigações de sistemas numéricos alternativos antes de se expandir para subsistemas de linguagem, máquinas virtuais, instruções, políticas e garantia de pesquisa.

Uma segunda direção explicitamente experimental é a **execução adaptativa consciente da representação**: selecionar representações de dados, algoritmos e caminhos de execução para cargas de trabalho específicas, especialmente IA/ML, enquanto mede os custos totais de seleção, conversão, computação e precisão. Esta é uma hipótese arquitetônica, **não** uma vantagem de desempenho do T81 já estabelecida.

Sua pergunta central é:

> Sob quais condições a computação ternária pode superar, simplificar ou de outra forma aprimorar uma abordagem binária equivalente?

A resposta não é pressuposta.

Alguns experimentos demonstraram propriedades matemáticas e representacionais úteis. Outros mostraram que implementações binárias convencionais continuam mais rápidas ou eficientes. Várias questões importantes permanecem em aberto.

O T81 existe para investigar sistematicamente esses limites.

**Esta é uma plataforma de pesquisa, não uma afirmação de que um computador ternário superior já foi construído.**

---

## Por que o ternário?

A computação moderna utiliza predominantemente estados binários: 0 e 1.

A computação ternária utiliza três estados. No ternário balanceado, estes são representados como:

$$T = \{-1,0,+1\}$$

Isso cria possibilidades interessantes para aritmética com sinal, representação numérica, codificação de estados, sistemas de decisão e arquitetura computacional.

Por exemplo:

| Propriedade | Relação matemática |
| --- | --- |
| Um trit | Três estados possíveis |
| Quatro trits | $3^4 = 81$ estados |
| Seis trits | $3^6 = 729$ estados |
| Informação por trit | $\log_2 3 \approx 1,585$ bits |
| Dígito em base-81 | Quatro trits agrupados |

Essas relações são fatos matemáticos.

Elas **não** estabelecem que os processadores ternários são mais rápidos, que a memória ternária é fisicamente mais densa ou que os algoritmos ternários consomem menos energia.

Essas são questões de engenharia separadas que exigem comparações adequadas.

O T81 investiga essa distinção.

---

## O que aprendemos?

A pesquisa é valiosa quando identifica tanto oportunidades quanto limitações.

**Principal resultado — uma comparação desfavorável de binário otimizado:** No experimento histórico de matriz-vetor AVX2 abaixo, o INT8 nativo alcançou **17,15 GFLOP/s**, contra **12,91 GFLOP/s** para o ternário compactado de 2 bits (aproximadamente **32,8% a mais de vazão** para a implementação INT8 nativa). Este é um resultado específico para a carga de trabalho, e não um ranking universal de bases numéricas; o T81 o publica de forma destacada porque um resultado desfavorável é evidência, e não um defeito a ser ocultado.

O histórico experimental do T81 produziu várias descobertas importantes.

### 1. Resultado negativo do AVX2: densidade de representação não é velocidade de execução

Dados ternários compactados podem reduzir os requisitos de armazenamento.

No entanto, o software ainda precisa decodificar, manipular e executar essas representações em hardware binário convencional.

Em um experimento histórico de matriz-vetor AVX2:

| Implementação | Vazão relatada |
| --- | --- |
| INT8 nativo AVX2 | 17,15 GFLOP/s |
| Ternário compactado de 2 bits AVX2 | 12,91 GFLOP/s |

A representação ternária compacta não superou a implementação INT8 convencional nessa carga de trabalho.

Este é um resultado experimental delimitado — e não uma prova de que a computação ternária é universalmente mais lenta.

[Explore a pesquisa de ternário compactado](https://www.google.com/search?q=docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md)

### 2. Representações adaptativas podem ter vantagens condicionais

A pesquisa sobre hierarquização de representação adaptativa identificou configurações em que uma representação ternária possui um custo modelado menor do que uma alternativa binária equivalente.

Essas vantagens dependem de premissas de carga de trabalho, metadados, custos de conversão e limites de representação.

A pesquisa H04-R02 corrigida preserva essas descobertas condicionais, enquanto deixa explicitamente o desempenho físico em aberto.

[Leia as descobertas corrigidas de H04-R02](https://www.google.com/search?q=experiments/h04_r02_adaptive_representation/RESULTS.md)

### 3. A correção não estabelece otimização

Uma investigação experimental recente, T81-AIOPT-001, examinou múltiplas representações de correspondência de padrões de quatro trits.

Sua implementação M1 passou por extensas verificações de correção local, mas a carga de trabalho experimental pôde ser reduzida a uma simples comparação ordinal.

O resultado demonstra por que um teste exaustivo de correção ainda pode ser insuficiente para avaliar a vantagem computacional.

A próxima pergunta não é simplesmente se outra representação funciona.

É se a carga de trabalho escolhida consegue distinguir diferenças significativas entre algoritmos concorrentes.

[Explore o T81-AIOPT-001](https://www.google.com/search?q=docs/research/aiopt/)

### 4. Panorama de status H04-R03 (02/11/2026): correção antes do desempenho

O programa de codec correspondente H04-R03 compara alternativas de representação e acesso delimitadas, incluindo um candidato B729-µ5. Seu protocolo pré-registrado e a implementação/revisão do Candidato A são rastreados nos [PR #677](https://github.com/t81dev/t81-foundation/pull/677), [PR #679](https://github.com/t81dev/t81-foundation/pull/679) e [PR #680](https://github.com/t81dev/t81-foundation/pull/680). Os relatórios de revisão independente tratam apenas da correção e aceitação do protocolo; a partir de 02/11/2026, os PRs #679 e #680 permanecem abertos, e a medição de desempenho da Fase C não está autorizada nem relatada. O status do PR deve ser atualizado antes de confiar neste panorama.

A igualdade nos comprimentos de carga útil é uma propriedade matemática, e não uma latência, energia ou benefício ponta a ponta demonstrado. Nenhuma vantagem de desempenho do H04-R03 é reivindicada.

### 5. Execução multiplataforma delimitada demonstrada (CPA-004/CPA-005)

**Classificação de evidência: determinismo e portabilidade delimitados, e não desempenho comparativo ou maturidade de todo o sistema.** Os resultados abaixo dizem respeito a um programa congelado e ambientes de execução especificados; eles não qualificam instruções não testadas, cargas de trabalho, hardware físico ou a arquitetura mais ampla.

Pesquisas recentes no programa de Arquitetura Multiplataforma (CPA, do inglês *Cross-Profile Architecture*) demonstraram que o núcleo computacional do T81 é executado de forma consistente em ambientes de execução divergentes:

* **Artefato de Programa Canônico:** Um programa canônico congelado de 351 bytes compreendendo 27 registros canônicos de 13 bytes (SHA-256 `b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b`) exerceu um subconjunto admitido de 17 instruções TISC.
* **Equivalência Multiplataforma:** Execuções novas em três processos hospedados do T81VM e três inicializações autônomas (*bare-metal*) de AArch64 QEMU (após o UEFI `ExitBootServices`) corresponderam a um oráculo matemático derivado de forma independente em **todos os 25 limites de observação** (estado inicial + 24 etapas de instrução).
* **Correspondência Abrangente de Limites:** A equivalência foi verificada em todos os 81 valores e tags de registradores, 1.563 palavras e tags de memória lógica, flags e registradores de sistema (R75–R80), parando deterministicamente em PC=27, SP=283.
* **Escopo e Panorama:** Testado no snapshot base do repositório `803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4` (com avaliador independente e relatório de aceitação confirmados no branch `origin/experiment/cpa004-freestanding` no commit `b803bf916a1b1e44129f3a07abf96fa6103c51fc`, e registros de continuidade locais do CPA-005).
* **Distinção:** Demonstrar a correção da execução multiplataforma prova que a arquitetura computacional funciona conforme projetado; **não** é evidência de desempenho ternário, energia ou superioridade de hardware sobre as bases binárias.

### 6. A computação ternária física permanece uma questão em aberto

A computação ternária emulada por software não é equivalente à computação em hardware ternário nativo.

A avaliação de dispositivos físicos exige evidências correspondentes para área, energia, latência, confiabilidade, sensoriamento, codificação e custos de periféricos.

A avaliação física atual permanece bloqueada onde faltam evidências adequadas de caracterização de dispositivos.

O T81 não interpreta a falta de evidências como prova de impossibilidade.

---

## A arquitetura de pesquisa do T81

O repositório contém vários subsistemas experimentais desenvolvidos para investigar representações computacionais e execução controlada.

```text
                     FUNDAÇÃO T81
           Arquitetura Implementada e Pesquisa
                             |
          +------------------+------------------+
          |                  |                  |
    REPRESENTAÇÃO       EXECUÇÃO          GARANTIA
          |                  |                  |
    Ternário balanceado  TISC ISA (Contrato)  Política Axion
    Base-81 / Base-243   +-- T81VM (Hospedado) CanonFS
    Dados compactados    +-- Axion (Baremetal)  Evidência
    Hierarquia adaptativa Front-end T81Lang   Proveniência
          |                  |                  |
          +------------------+------------------+
                             |
               EXPERIMENTOS CONTROLADOS
                             |
               Bases binárias convencionais
                             |
               Verificação e comparação
                             |
                  Conclusões de Pesquisa

```

### Contrato Computacional e Perfis de Execução

Na arquitetura do T81:

* **TISC (Ternary Instruction Set Computer):** Serve como o contrato de instrução computacional central, definindo semântica de opcode, vetores de estado de 64 bits e regras de transição determinísticas.
* **T81VM Hospedado:** Serve como o perfil de execução hospedado para desenvolvimento no espaço do usuário, ferramentas, testes e reprodução determinística em sistemas operacionais hospedeiros convencionais (Linux, macOS, Windows).
* **Axion OS / TernaryOS Autônomo:** Serve como o perfil de execução em metal *bare-metal* rodando diretamente em hardware ou hipervisores (ex.: AArch64 QEMU), executando o TISC sem dependências de SO no espaço do usuário.
* **DAIOS (Deterministic AI OS):** Continua sendo uma direção de pesquisa experimental independente e voltada para o futuro para governança de agentes autônomos de alto nível e tempo de execução de IA, distinta do motor de execução TISC de baixo nível implementado.

### Componentes principais

| Componente | Função de pesquisa e arquitetônica |
| --- | --- |
| **TISC** | O contrato de instrução computacional que rege a semântica de execução |
| **T81VM** | Perfil de execução hospedado para execução em máquina virtual e determinismo (delimitado pelo [Registro de Superfície de Determinismo](https://www.google.com/search?q=docs/governance/DETERMINISM_SURFACE_REGISTRY.md)) |
| **Axion / TernaryOS** | Perfil de execução autônomo (*bare-metal* AArch64 microkernel/HAL) e autorização de política pré-efeito colateral |
| **T81Lang** | Front-end de linguagem e infraestrutura de compilação direcionada ao TISC |
| **T81 Core** | Tipos numéricos, contêineres e primitivas de representação |
| **CanonFS** | Artefatos endereçados por conteúdo e mecanismos de integridade |
| **DAIOS** | Proposta de arquitetura de destino independente para sistemas operacionais de IA determinísticos |
| **Experimentos de pesquisa** | Testes de hipóteses, validação de correção e medições comparativas |

Esses componentes têm diferentes níveis de maturidade. Uma implementação funcional não se traduz automaticamente em uma vantagem de desempenho validada.

---

## Execução adaptativa consciente da representação: uma direção de pesquisa em aberto

Os manuscritos históricos do T81 descrevem uma ambição integrada: tipos de dados numéricos, semântica de execução TISC, T81Lang/T81VM e decisões mediadas pelo Axion sobre o uso de recursos. A [coleção histórica de PDFs](https://www.google.com/search?q=docs/pdf/) preserva essas propostas; seus números de desempenho relatados são **afirmações históricas e não verificadas**, e não resultados experimentais atuais.

Uma versão falsificável moderna dessa ambição questiona se um sistema pode selecionar entre representações exatas, de ponto flutuante, de baixo bit, ternárias compactadas e binárias convencionais para uma **carga de trabalho real**, alcançando um benefício líquido após contabilizar precisão, codificação/decodificação, tráfego de memória, escalonamento e custo operacional de decisão. O projeto deve distinguir:

* **T — mecanismo específico do ternário:** uma vantagem atribuível à semântica ternária ou a um substrato ternário físico, e não apenas a uma codificação diferente.
* **S — escolha de representação de software:** uma vantagem decorrente da seleção de uma representação, potencialmente reproduzível com codificações binárias.
* **A — roteamento de algoritmos:** uma vantagem decorrente da escolha de um algoritmo ou caminho de execução melhor, independentemente da base numérica (radix).

Resultados positivos de S ou A são úteis, mas **não** estabelecem T. O T81-AIOPT-001 M2 permanece um problema de design de pesquisa em vez de uma prova autorizada de superioridade ternária adaptativa. Para escopo autorizado e portões, consulte o [contexto de pesquisa de IA](https://www.google.com/search?q=docs/ai-context/) e os protocolos de experimentos subjacentes.

## Três linhas de pesquisa

O T81 organiza seu trabalho em três frentes complementares.

### Linha R — Pesquisa computacional

Investigar se representações alternativas ou arquiteturas computacionais produzem vantagens úteis.

Os assuntos incluem:

* Aritmética e representação ternária balanceada.
* Base-81 e agrupamento de ordem superior.
* Seleção de representação adaptativa.
* Computação compactada de baixo bit.
* Custos de memória e movimentação de dados.
* Hipóteses de hardware ternário nativo.
* Inferência de IA ternária e representações de modelos.

**O sucesso significa estabelecer um resultado defensável, incluindo um resultado negativo.**

### Linha F — Engenharia de base

Desenvolver as ferramentas necessárias para conduzir experimentos confiáveis e avaliar a computação determinística.

Isso inclui máquinas virtuais, ferramentas de linguagem, identidade de artefatos, autorização, reprodução e infraestrutura de verificação.

Essas capacidades podem ser úteis independentemente da computação ternária.

Seu valor de engenharia deve ser avaliado separadamente das reivindicações de superioridade específica de base numérica.

### Linha M — Memória de pesquisa

Preservar as evidências por trás de cada conclusão.

Isso inclui histórico de origem, protocolos experimentais, tentativas frustradas, correções, registros de reprodutibilidade e questões em aberto.

Um resultado substituído permanece parte do registro histórico.

Um modelo matemático não é promovido silenciosamente a uma medição física.

Uma avaliação gerada por IA não é automaticamente uma verificação independente.

[Leia a Carta de Continuidade Mestre](https://www.google.com/search?q=docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md)

---

## Metodologia de pesquisa

O T81 segue uma abordagem centrada em evidências.

Cada experimento comparativo deve responder a cinco perguntas:

1. **Qual é a hipótese?** Definir uma proposição falsificável.
2. **Qual é a linha de base?** Comparar com uma alternativa convencional funcionalmente equivalente e adequadamente otimizada.
3. **O que é medido?** Separar correção, armazenamento, latência, vazão, energia e outros custos relevantes.
4. **Quais são as limitações?** Identificar premissas, hardware, escopo da carga de trabalho, incerteza e dependências não resolvidas.
5. **Qual conclusão é justificada?** Distinguir resultados suportados, não suportados, inconclusivos, bloqueados e não testados.

O projeto também distingue entre:

**Implementado → Testado → Medido → Comparativamente estabelecido**

Estes são níveis probatórios diferentes, e não rótulos intercambiáveis.

Um resultado experimental não deve reivindicar mais do que suas evidências sustentam.

---

## A visão original e os manuscritos históricos

O [arquivo histórico de PDFs](https://www.google.com/search?q=docs/pdf/) contém explorações iniciais da Base-81, tipos de dados, T81Lang, TISC, Axion, IA/ML e aplicações propostas. Esses documentos preservam a linhagem intelectual e hipóteses de design; **não** são especificações atuais ou relatórios de benchmark verificados. Uma revisão futura deve reconciliar as reivindicações com experimentos primários sem sobrescrever os originais.

### Hanoi

A história do T81 é anterior ao repositório atual da Fundação.

Seu precursor mais conhecido, **Hanoi**, explorou computação recursiva, estruturas de máquinas virtuais ternárias, despacho simbólico e níveis de representação dependentes da carga de trabalho usando a programação literata CWEB.

O projeto emergiu de investigações em matemática discreta, algoritmos, modelos computacionais e do trabalho de Donald Knuth.

Com o tempo, a pesquisa se expandiu para bibliotecas aritméticas, ferramentas de linguagem, máquinas virtuais, computação de IA de baixo bit, execução determinística, governança e hardware experimental.

Alguns projetos históricos estavam incompletos. Algumas premissas provaram estar incorretas. Outras permanecem inexploradas.

O propósito de preservar essa linhagem não é validar todas as propostas anteriores.

É recuperar as perguntas que as motivaram e testar essas perguntas com ferramentas melhores.

[Explore o repositório original do Hanoi](https://github.com/t81dev/Hanoi) · [Arquivo de continuidade de pesquisa](https://www.google.com/search?q=docs/research/continuity/)

---

## Prioridades atuais de pesquisa

As prioridades imediatas do programa são:

**1. Estabelecer linhas de base experimentais confiáveis.**

Verificar semântica equivalente e comportamento reprodutível de forma independente antes de interpretar diferenças de desempenho.

**2. Identificar cargas de trabalho discriminantes.**

Selecionar experimentos nos quais representações alternativas tenham uma consequência plausível e mensurável além de simples alterações de codificação.

**3. Investigar vantagens condicionais e cargas de trabalho de IA/ML.**

Estudar as circunstâncias sob as quais a densidade de representação, padrões de acesso, custos de conversão, precisão do modelo ou estratégias adaptativas alteram o resultado. Comparar com linhas de base binárias e de roteamento de algoritmos igualmente otimizadas.

**4. Preservar questões de hardware físico.**

Separar as limitações da emulação de software do custo e viabilidade de dispositivos multivalorados nativos.

**5. Manter a fundação experimental.**

Manter as ferramentas, especificações, testes, registros de proveniência e mecanismos de governança confiáveis o suficiente para apoiar a pesquisa.

Propostas ativas e seu status de aceitação pertencem aos registros de pesquisa e solicitações de pull request (*pull requests*). Sua aparição no repositório não implica aprovação formal.

[Leia o roteiro de colaboradores](https://www.google.com/search?q=docs/ROADMAP.md)

---

## Explore o repositório

| Ponto de partida | O que você encontrará |
| --- | --- |
| [`docs/research/`](https://www.google.com/search?q=docs/research/) | Investigações científicas e conclusões experimentais |
| [`experiments/`](https://www.google.com/search?q=experiments/) | Implementações experimentais e artefatos de reprodutibilidade |
| [`spec/`](https://www.google.com/search?q=spec/) | Especificações de conjuntos de instruções e RFCs |
| [`include/t81/`](https://www.google.com/search?q=include/t81/) | Bibliotecas principais e interfaces públicas |
| [`vm/`](https://www.google.com/search?q=vm/) | Implementação da máquina virtual |
| [`lang/`](https://www.google.com/search?q=lang/) | Ferramentas de linguagem |
| [`kernel/axion/`](https://www.google.com/search?q=kernel/axion/) | Componentes de autorização e política |
| [`fs/`](https://www.google.com/search?q=fs/) | Identidade de artefatos e mecanismos de armazenamento |
| [`docs/ai-context/`](https://www.google.com/search?q=docs/ai-context/) | Material de continuidade para colaboradores de pesquisa em IA |
| [`docs/pdf/`](https://www.google.com/search?q=docs/pdf/) | Manuscritos históricos e suas hipóteses de pesquisa originais |

Para instruções de compilação, dependências de desenvolvimento e alvos de teste disponíveis, consulte a documentação de colaboradores do repositório e a configuração do CMake. Os resultados da pesquisa devem ser reproduzidos usando o ambiente e os comandos especificados por seus respectivos protocolos individuais.

---

## Contribuindo

O T81 aceita contribuições que aprimorem a compreensão científica, a correção da implementação, a reprodutibilidade ou a documentação.

Contribuições especialmente valiosas incluem:

* Reprodução independente de experimentos existentes.
* Linhas de base binárias convencionais mais fortes.
* Contra-exemplos matemáticos e correções de provas.
* Protocolos experimentais menores e mais discriminantes.
* Validação de correção multiplataforma.
* Correções baseadas em evidências para reivindicações históricas.
* Melhorias com escopo bem definido na infraestrutura de pesquisa.

Antes de propor um novo subsistema, identifique a qual questão de pesquisa ele responde e quais evidências estabeleceriam sua utilidade.

Leia o [roteiro de colaboradores](https://www.google.com/search?q=docs/ROADMAP.md), as instruções de governança do repositório e os protocolos de experimentos relevantes antes de modificar os artefatos de pesquisa.

---

## O que o T81 não afirma

O T81 atualmente não afirma:

* A superioridade universal da computação ternária sobre a computação binária.
* Superioridade demonstrada de silício ternário nativo.
* Uma vantagem geral de desempenho para a arquitetura integrada do T81.
* Um otimizador de IA adaptativo validado que supere linhas de base fortes de política fixa ou de seleção de algoritmos.
* Que os dados de estudo de caso em PDF históricos constituem medições reproduzidas de forma independente.
* Que os pesos de modelos ternários aceleram inerentemente a inferência de IA.
* Que a execução determinística ou a aplicação de políticas exija uma máquina virtual ternária.
* Que um experimento gerado por IA seja validado independentemente apenas porque seus testes passam.

Esses limites são essenciais para a pesquisa.

Eles deixam espaço para descobertas futuras sem distorcer o que já foi estabelecido.

---

## A questão permanece em aberto

A computação binária domina a infraestrutura física e de software atual por motivos convincentes de engenharia.

Esse domínio não elimina o valor de investigar representações alternativas, modelos computacionais ou futuros substratos de hardware.

O propósito do T81 é determinar onde essas alternativas são úteis — e onde não são.

**Não estamos construindo uma conclusão e procurando evidências para apoiá-la.**

**Estamos construindo a fundação experimental necessária para descobrir a conclusão.**

---

**Fundação T81** — Arquitetura de Computação Experimental e Pesquisa de Sistemas

[Licença: Apache 2.0](https://www.google.com/search?q=LICENSE) · [GitHub](https://github.com/t81dev/t81-foundation)
