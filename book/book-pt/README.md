# A Fundação T81 — Monografia Técnica Definitiva

## Prefácio

Existem duas maneiras de construir sistemas.

Uma é otimizar pela conveniência — mover-se rapidamente, aproximar, aceitar que o bit final pode variar, que a deriva de ponto flutuante é tolerável, que compiladores podem reordenar, que o hardware decidirá o que significa "próximo o suficiente".

A outra é insistir que a computação não é sugestão, mas declaração.

O T81 pertence ao segundo caminho.

Em seu núcleo, este projeto não é sobre aritmética ternária, máquinas virtuais ou motores de política — embora contenha todos esses. É sobre **integridade de execução**. É sobre desenhar um limite ao redor de um processo computacional e dizer: dentro deste limite, o comportamento não é incidental.

O determinismo é frequentemente tratado como uma troca de desempenho ou uma conveniência de depuração. Aqui é tratado como uma restrição civilizacional. Se duas máquinas não podem concordar sobre o resultado do mesmo programa, então a computação nunca foi verdadeiramente definida — ela foi apenas executada.

Ternário balanceado, serialização canônica, matemática definida por software, registro de rastreamento, aplicação de políticas — estas não são escolhas estéticas. Elas são instrumentos em um único argumento:

> Uma computação deve ser reprodutível, auditável e estruturalmente honesta.

Sistemas modernos são construídos com camadas de abstração que escondem transições de estado atrás de otimizadores, execução especulativa, peculiaridades de ponto flutuante e efeitos colaterais implícitos. O T81 tenta algo diferente: tornar cada transição explícita, cada representação canônica, cada execução rastreável.

É um experimento arquitetural em restrição.

O sistema não assume hardware benevolente.
Ele não assume bibliotecas de ponto flutuante idênticas.
Ele não assume que compiladores se comportam da mesma forma entre arquiteturas.
Ele não assume que a execução sem registro é aceitável.

Em vez disso, ele codifica regras:

* Transições de estado devem ser definíveis.
* Dados devem ter uma única forma canônica.
* O consumo de recursos deve ser contabilizável.
* Políticas devem ser aplicáveis.
* O comportamento deve ser reproduzível.

O resultado não é a máquina mais rápida.
Não é o ambiente mais flexível.
Não é projetado para substituir ecossistemas de script de propósito geral.

É projetado para responder a uma pergunta mais estreita, mas mais exigente:

**Pode um sistema de software ser construído de tal forma que seu comportamento seja comprovadamente invariante através do espaço e do tempo?**

Este livro existe para documentar essa tentativa.

Não como mitologia.
Não como marketing.
Mas como um livro-razão.

Cada subsistema descrito aqui — T81Lang, TISC, a T81VM, Axion, CanonFS, os portões de determinismo, os níveis cognitivos — é parte de uma estrutura em camadas construída em torno de um invariante:

> Entradas idênticas devem produzir saídas idênticas, sob regras explicitamente definidas.

Se esta arquitetura se tornará amplamente adotada é secundário. O que importa é que ela foi tornada concreta, implementada, testada e descrita com precisão suficiente para que possa ser compreendida, verificada ou desafiada por outros.

Este volume é, portanto, tanto técnico quanto filosófico.

É técnico porque descreve um sistema em funcionamento.
É filosófico porque afirma que a reprodutibilidade não é opcional em certos domínios.

Se o repositório evoluir, este livro deve evoluir com ele.
Se o projeto cessar, este documento deve permanecer suficiente para reconstruir o que foi tentado e por quê.

No final, o T81 não é uma reivindicação de perfeição.

É um compromisso com a restrição.

E a restrição, quando aplicada deliberadamente, é uma forma de clareza.

---

## Como Ler Este Livro

* **Novo no T81?** → Comece pela Parte I, depois Parte II.
* **Implementador?** → Foque nas Partes II e III.
* **Auditor?** → Leia as Partes III e IV cuidadosamente.
* **Pesquisador?** → Dê ênfase às Partes IV e V.
* **Mantenedor de longo prazo?** → As Partes IV e V são críticas.

---

## Navegação

<details open>
<summary><strong>Parte I — Fundamentos</strong></summary>

1. **[Introdução](./01_Introducao.md)**

   * [1.1 Escopo e Definição](./01_Introducao.md#11-escopo-e-definicao)
   * [1.2 Arquitetura do Sistema](./01_Introducao.md#12-arquitetura-do-sistema)
   * [1.3 Missão de Computação Verificável](./01_Introducao.md#13-missao-de-computacao-verificavel)
   * [1.4 Terminologia](./01_Introducao.md#14-terminologia)
   * [1.5 Checklist de Verificação](./01_Introducao.md#15-checklist-de-verificacao)

2. **[Princípios Centrais e Invariantes](./02_Principios.md)**

   * [2.1 O Invariante de Determinismo](./02_Principios.md#21-o-invariante-de-determinismo)
   * [2.1.1 Superfícies de Determinismo e Vetores de Ataque](./02_Principios.md#211-superficies-de-determinismo-e-vetores-de-ataque)
   * [2.2 Lógica Ternária (Base-3)](./02_Principios.md#22-logica-ternaria-base-3)
   * [2.3 Auditabilidade e o Trace Axion](./02_Principios.md#23-auditabilidade-e-o-trace-axion)
   * [2.4 Os Nove Princípios (Aplicação de Ética)](./02_Principios.md#24-os-nove-principios-aplicacao-de-etica)
   * [2.5 Checklist de Verificação](./02_Principios.md#25-checklist-de-verificacao)
   * [2.6 Matriz de Auditoria Formal](./02_Principios.md#26-matriz-de-auditoria-formal)

</details>

<details>
<summary><strong>Parte II — A Máquina Determinística</strong></summary>

3. **[Arquitetura T81VM](./03_Arquitetura.md)**

   * [3.1 Visão Geral](./03_Arquitetura.md#31-visao-geral)
   * [3.2 A Fronteira de Runtime](./03_Arquitetura.md#32-a-fronteira-de-runtime)
   * [3.3 Modelo de Memória](./03_Arquitetura.md#33-modelo-de-memoria)
   * [3.4 O Conjunto de Instruções (TISC)](./03_Arquitetura.md#34-o-conjunto-de-instrucoes-tisc)
   * [3.5 Compilação JIT (Trace-JIT)](./03_Arquitetura.md#35-compilacao-jit-trace-jit)

4. **[Tipos de Dados e Serialização](./04_Tipos_de_Dados_e_Serializacao.md)**

   * [4.1 Tipos Primitivos](./04_Tipos_de_Dados_e_Serializacao.md#41-tipos-primitivos)
   * [4.2 T81Float e dmath](./04_Tipos_de_Dados_e_Serializacao.md#42-t81float-e-dmath)
   * [4.3 Tensores e Layouts Canônicos](./04_Tipos_de_Dados_e_Serializacao.md#43-tensores-e-layouts-canonicos)
   * [4.4 Regras de Serialização Canônica](./04_Tipos_de_Dados_e_Serializacao.md#44-regras-de-serializacao-canonica)

5. **[Instalação e Verificação de Build](./05_Instalacao.md)**

   * [5.1 Pré-requisitos](./05_Instalacao.md#51-pre-requisitos)
   * [5.2 Compilando a partir da Fonte](./05_Instalacao.md#52-compilando-a-partir-da-fonte)
   * [5.3 Verificando o Build](./05_Instalacao.md#53-verificando-o-build)
   * [5.4 Solução de Problemas](./05_Instalacao.md#54-solucao-de-problemas)

6. **[Uso de CLI e API](./06_Uso.md)**

   * [6.1 A Interface de Linha de Comando T81](./06_Uso.md#61-a-interface-de-linha-de-comando-t81)
   * [6.2 Embutindo T81 (API C++)](./06_Uso.md#62-embutindo-t81-api-c)
   * [6.3 Embutindo T81 (API Python)](./06_Uso.md#63-embutindo-t81-api-python)
   * [6.4 Depuração](./06_Uso.md#64-depuracao)

</details>

<details>
<summary><strong>Parte III — Governança e Verificação</strong></summary>


7. **[Programação em T81Lang](./07_Programacao_em_T81Lang.md)**

   * [7.1 Filosofia de Design](./07_Programacao_em_T81Lang.md#71-filosofia-de-design)
   * [7.2 Noções Básicas de Sintaxe](./07_Programacao_em_T81Lang.md#72-nocoes-basicas-de-sintaxe)
   * [7.3 Tipos de Dados](./07_Programacao_em_T81Lang.md#73-tipos-de-dados)
   * [7.4 Controle de Fluxo](./07_Programacao_em_T81Lang.md#74-controle-de-fluxo)
   * [7.5 Funções](./07_Programacao_em_T81Lang.md#75-funcoes)
   * [7.6 Integração com Axion](./07_Programacao_em_T81Lang.md#76-integracao-com-axion)
   * [7.7 Exemplos](./07_Programacao_em_T81Lang.md#77-exemplos)
8. **[Verificação e Auditoria](./08_Verificacao_e_Auditoria.md)**

   * [8.1 Metodologia de Verificação Formal](./08_Verificacao_e_Auditoria.md#81-metodologia-de-verificacao-formal)
   * [8.2 A Matriz de Auditoria Formal](./08_Verificacao_e_Auditoria.md#82-a-matriz-de-auditoria-formal)
   * [8.3 Testes Baseados em Propriedades](./08_Verificacao_e_Auditoria.md#83-testes-baseados-em-propriedades)
   * [8.4 O Portão de Determinismo](./08_Verificacao_e_Auditoria.md#84-o-portao-de-determinismo-determinism-gate)

9. **[O Kernel de Segurança Axion](./09_O_Kernel_Axion.md)**

   * [9.1 Definição Formal](./09_O_Kernel_Axion.md#91-definicao-formal)
   * [9.2 O Modelo de Política](./09_O_Kernel_Axion.md#92-o-modelo-de-politica)
   * [9.3 Interceptação de Instrução](./09_O_Kernel_Axion.md#93-interceptacao-de-instrucao)
   * [9.4 O Log de Auditoria (Trace)](./09_O_Kernel_Axion.md#94-o-log-de-auditoria-trace)
   * [9.5 Promoção Cognitiva](./09_O_Kernel_Axion.md#95-promocao-cognitiva)

10. **[Níveis Cognitivos e Computação Distribuída](./10_Niveis_Cognitivos_e_Computacao_Distribuida.md)**

   * [10.1 O Modelo de Nível Cognitivo](./10_Niveis_Cognitivos_e_Computacao_Distribuida.md#101-o-modelo-de-nivel-cognitivo)
   * [10.2 Computação Distribuída (Nível 4)](./10_Niveis_Cognitivos_e_Computacao_Distribuida.md#102-computacao-distribuida-nivel-4)
   * [10.3 Compilação JIT Baseada em Trace](./10_Niveis_Cognitivos_e_Computacao_Distribuida.md#103-compilacao-jit-baseada-em-trace)
   * [10.4 Formas Infinitas (Nível 5)](./10_Niveis_Cognitivos_e_Computacao_Distribuida.md#104-formas-infinitas-nivel-5)

11. **[Apêndices](./11_Apendices.md)**

   * [11.1 O Que Ainda Não Foi Implementado](./11_Apendices.md#111-o-que-ainda-nao-foi-implementado)
   * [11.2 Glossário](./11_Apendices.md#112-glossario)
   * [11.3 Links Úteis](./11_Apendices.md#113-links-uteis)

</details>

<details>
<summary><strong>Parte IV — Formalização e Endurecimento Estrutural</strong></summary>

12. **[Semântica Formal do TISC e T81VM](./12_Semantica_Formal.md)**

   * [12.1 Semântica Operacional](./12_Semantica_Formal.md#121-semantica-operacional)
   * [12.2 Função de Transição Algébrica](./12_Semantica_Formal.md#122-funcao-de-transicao-algebrica)
   * [12.3 Sistema de Reescrita de Canonicalização](./12_Semantica_Formal.md#123-sistema-de-reescrita-de-canonicalizacao)
   * [12.4 Esboços de Prova de Determinismo](./12_Semantica_Formal.md#124-esbocos-de-prova-de-determinismo)
   * [12.5 Equivalência Intérprete vs Trace-JIT](./12_Semantica_Formal.md#125-equivalencia-interprete-vs-trace-jit)

13. **[Modelagem Adversária e Ataques de Determinismo](./13_Modelagem_Adversaria.md)**

   * [13.1 Modelo de Ameaça](./13_Modelagem_Adversaria.md#131-modelo-de-ameaca)
   * [13.2 Ataques de Nível de Compilador](./13_Modelagem_Adversaria.md#132-ataques-de-nivel-de-compilador)
   * [13.3 Vetores de Ataque de VM e GC](./13_Modelagem_Adversaria.md#133-vetores-de-ataque-de-vm-e-gc)
   * [13.4 Ataques CanonFS e Hash](./13_Modelagem_Adversaria.md#134-ataques-canonfs-e-hash)
   * [13.5 Ataque de Viagem no Tempo de Nível Distribuído](./13_Modelagem_Adversaria.md#135-ataque-de-viagem-no-tempo-de-nivel-distribuido)
   * [13.6 Modelo de Post-Mortem de Violação de Determinismo](./13_Modelagem_Adversaria.md#136-modelo-de-post-mortem-de-violacao-de-determinismo)

</details>

<details>
<summary><strong>Parte V — Continuidade e Horizonte de Pesquisa</strong></summary>

14. **[Continuidade e Resiliência](./14_Continuidade_e_Resiliencia.md)**

   * [14.1 O Protocolo Cleanroom](./14_Continuidade_e_Resiliencia.md#141-o-protocolo-cleanroom)
   * [14.2 Pontos Únicos de Falha](./14_Continuidade_e_Resiliencia.md#142-pontos-unicos-de-falha)
   * [14.3 Manifesto de Continuidade](./14_Continuidade_e_Resiliencia.md#143-manifesto-de-continuidade)
   * [14.4 Invariantes Formais Imutáveis](./14_Continuidade_e_Resiliencia.md#144-invariantes-formais-imutaveis)

15. **[Fronteira de Pesquisa](./15_Fronteira_de_Pesquisa.md)**

   * [15.1 Aceleração de Hardware Ternário](./15_Fronteira_de_Pesquisa.md#151-aceleracao-de-hardware-ternario)
   * [15.2 Caminhos de Verificação Formal](./15_Fronteira_de_Pesquisa.md#152-caminhos-de-verificacao-formal)
   * [15.3 CanonFS como Substrato Merkle](./15_Fronteira_de_Pesquisa.md#153-canonfs-como-substrato-merkle)
   * [15.4 Inferência de IA Determinística em Escala](./15_Fronteira_de_Pesquisa.md#154-inferencia-de-ia-deterministica-em-escala)

</details>
