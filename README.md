# bvhviewer ☘️

Relatório das Funções;

1. trim_prefix
Descrição:
Essa função remove espaços em branco iniciais de uma string. É usada para garantir que o processamento subsequente de cada linha do arquivo BVH seja feito corretamente, sem interferências causadas por indentação ou formatação de espaços.
Funcionamento:
- Localiza o primeiro caractere não-espaço usando a função isspace.
- Se o início da string contiver espaços, move os caracteres relevantes para o início
da string original, utilizando memmove.
Contexto de Uso: Essencial durante o parsing do arquivo BVH, em que espaços antes de palavras-chave (como ROOT ou JOINT) devem ser ignorados.

2. parseRoot
Descrição:
Essa função interpreta e processa o nó raiz da hierarquia do esqueleto (definido pela palavra-chave ROOT no arquivo BVH). Ele lê o nome do nó raiz e o cria usando a função createNode.
Funcionamento:
- Lê uma linha do arquivo que deve conter a palavra-chave ROOT.
- Remove espaços extras da linha com trim_prefix.
- Verifica se a linha começa com ROOT e extrai o nome associado.
- Cria o nó raiz com 6 canais (representando graus de liberdade: 3 rotações + 3
translações) e deslocamentos iniciais definidos como zero.
Contexto de Uso: É o ponto de partida para construir a hierarquia do esqueleto lido do arquivo BVH.

3. parseBody
Descrição:
Essa função interpreta o corpo da hierarquia, incluindo os nós filhos (definidos por JOINT ou End Site) e suas respectivas propriedades, como deslocamento (OFFSET) e canais (CHANNELS).
Funcionamento:
- Mantém controle da hierarquia usando o nível de chaves (braceLevel), iniciando com o brace do nó pai.
- Analisa cada linha do arquivo:
-Identifica deslocamentos com OFFSET e armazena os valores no nó correspondente.
-Define os canais de transformação do nó com CHANNELS e aloca memória para seus dados.
-Lida com a criação de nós filhos (JOINT) ou nós finais (End Site) chamando recursivamente parseBody.
- Finaliza quando todas as chaves do nó atual são fechadas.
Contexto de Uso: Constroi de forma recursiva a hierarquia do esqueleto, conectando todos os nós pais, filhos e terminais.

4. parseHierarchy
Descrição:
É a função principal para interpretar a seção de hierarquia do arquivo BVH. Ela verifica a integridade inicial do arquivo e chama as funções parseRoot e parseBody para construir toda a hierarquia.
Funcionamento:
- Abre o arquivo BVH e verifica se a palavra-chave HIERARCHY está presente na primeira linha.
- Chama parseRoot para processar o nó raiz.
- Chama parseBody para construir a hierarquia completa, conectando o nó raiz aos
nós filhos.
Contexto de Uso: Representa o ponto de entrada do processo de parsing da hierarquia. Combina as funções auxiliares para retornar o nó raiz com toda a hierarquia carregada.

5. parseMotion
Descrição
A função parseMotion é responsável por interpretar a seção de movimento em um arquivo BVH, extraindo os valores de movimento (keyframes) e armazenando-os em um vetor de números de ponto flutuante (float). Esta função lê os frames descritos no arquivo, processa cada valor de movimento associado aos nós da hierarquia e retorna um ponteiro para um vetor dinâmico contendo todos os dados extraídos.

Funcionamento;
1. Verificação Inicial: Verifica se o ponteiro para o arquivo é válido. Caso contrário, exibe uma mensagem de erro utilizando perrore retorna NULL.
2. Localização da Seção MOTION: Realiza uma busca pela palavra-chave MOTION, que sinaliza o início da seção de movimento. Em seguida, procura as palavras-chave Frames: (para determinar o número total de frames) e Frame Time:(tempo entre frames), ignorando o valor do intervalo de tempo.
3. Alocação Dinâmica do Vetor: Inicializa um vetor dinâmico com uma estimativa inicial de tamanho para armazenar os valores de movimento. A alocação inicial presume até 100 valores por frame (ajustável conforme o arquivo).
4. Processamento dos Valores: Processa os valores de movimento, linha por linha, dividindo cada linha em valores separados utilizando a função strtok. Converte cada valor de string para float com atof e armazena no vetor.
5. Ajuste do Tamanho do Vetor: Após a leitura de todos os dados, ajusta dinamicamente o tamanho do vetor com realloc para corresponder ao número exato de valores armazenados.
6. Encerramento: Fecha o arquivo e retorna o ponteiro para o vetor contendo os valores de movimento.
Contexto de Uso
A função parseMotion é usada após o processamento da hierarquia do arquivo BVH. Ela complementa o parsing da hierarquia com os valores de movimento necessários para animar o esqueleto descrito. Os valores extraídos são organizados em um vetor linear, permitindo fácil acesso e manipulação dos dados durante a reprodução ou análise da animação.

Arquivo test.c:
Um arquivo com nome de test.c foi criado como uma ferramenta de depuração para facilitar o desenvolvimento e a validação do código em um ambiente controlado. Ele foi projetado para testar a funcionalidade completa do parser, incluindo a leitura de um arquivo BVH de teste. Por meio deste arquivo, foi possível verificar o funcionamento correto de todas as funções implementadas e garantir que o parser esteja processando o arquivo BVH da forma esperada. Assim, o uso do test.cassegura que o código final esteja robusto e confiável, com base em testes realizados diretamente em um cenário prático.
