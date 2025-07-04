// *************************************************************************************************************************************************
// JOGO DE CAÇA PALAVRAS EM C, DESENVOLVIDO POR LUIZ EDUARDO JELONSCHEK E JOÃO PEDRO GEHLEN DURANTE O 2 SEMESTRE DE ENGENHARIA DE SOFTWARE NA UTFPR.
// *************************************************************************************************************************************************
#define _CRT_SECURE_NO_WARNINGS // Visual Studio tava me impedindo de rodar o codigo por conta dos printf e scanf não seguros, ai coloquei isso

// Bibliotecas
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Variaveis Globais que Serão Usadas no Jogo
#define MAX_PALAVRA_LEN 50
#define MIN_DIMENSAO 7
#define MAX_DIMENSAO 9
#define MIN_PALAVRAS_ARQUIVO 10
#define NUM_PALAVRAS_JOGO 7

// Structs (necessarios pq a atividade pede)
typedef struct {
    char texto[MAX_PALAVRA_LEN];
    int encontrada;
} Palavra;

typedef struct {
    char** matriz;
    char** orig_matriz;
    int linhas;
    int colunas;
    Palavra* palavras_jogo[NUM_PALAVRAS_JOGO];
    int tentativas;
    int palavras_restantes;
} Jogo;


// "Sumario" das funções, para facilitar a leitura e entendimento do código, não precisaria para todas as funçoes (executadas antes do main), mas achei legal colocar.
// Funcoes de manipulacao de arquivo
void salvar_palavra(const Palavra* p);
void carregar_palavras(Palavra** lista_palavras, int* num_palavras);
void atualizar_palavra(const char* palavra_antiga, const Palavra* nova_palavra);
void apagar_palavra(const char* palavra_apagar);
void mostrar_palavras();

// Funcoes de jogo
void inicializar_jogo(Jogo* jogo);
void liberar_jogo(Jogo* jogo);
void gerar_matriz(Jogo* jogo);
void sortear_palavras_jogo(Jogo* jogo, Palavra* lista_todas_palavras, int num_todas_palavras);
void exibir_matriz(const Jogo* jogo);
void exibir_status_jogo(const Jogo* jogo);
int verificar_palavra(Jogo* jogo, int r1, int c1, int r2, int c2);
void preencher_espacos_aleatorios(Jogo* jogo);
void inserir_palavras_na_matriz(Jogo* jogo);

// Funcoes auxiliares
int obter_dimensao(const char* tipo);
int palavra_existe(const char* palavra);
void limpar_buffer();
void menu_principal();
int menu_pos_jogo();
int is_valid(int r, int c, int linhas, int colunas);

// Funcoes de manipulacao de arquivo

// Função salvar: Cria um ponteiro do tipo FILE, que será utilizado para gravar novas palavras no arquivo binario "palavras.bin", utiliza o método "ab", pois "a" de ADD e "b" de "BINARY".
// Se o arquivo for igual a NULL, ou seja, algum erro ocorreu, ele da return na função e exibe uma mensagem de erro.
// Se o arquivo for aberto com sucesso, ele utiliza a função fwrite para escrever a palavra (recebida na função) no arquivo, e depois fecha o mesmo.
void salvar_palavra(const Palavra* p) {
    FILE* arquivo = fopen("palavras.bin", "ab");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo para salvar.\n");
        return;
    }
    fwrite(p, sizeof(Palavra), 1, arquivo);
    fclose(arquivo);
    printf("Palavra '%s' salva com sucesso!\n", p->texto);
}

// Função carregar: Cria um ponteiro do tipo FILE, que será utilizado para ler as palavras do arquivo binario "palavras.bin".
// Ela recebe dois argumentos, o lista_palavras e num_palavras, que viram como NULL, abre o arquivo com o formato "rb" onde r = read e b = binary.
// Depois verifica se deu certo a abertura e utiliza fseek para mover o cursos para o final do arquivo para saber o tamanho dele, depois move o cursor para o inicio novametne com o segundo fseek.
// Após isso ele pega o tamanho do arquivo e divide pelo tamanho da palavra, e chega no resultado.
// Por final ele aloca a memória necessaria para essas palavras e verifica se deu certo a alocação, se deu certo ele usa o fread para ler o arquivo e por ultimo ele fecha o arquivo.
void carregar_palavras(Palavra** lista_palavras, int* num_palavras) {
    FILE* arquivo = fopen("palavras.bin", "rb");
    if (arquivo == NULL) {
        *num_palavras = 0;
        *lista_palavras = NULL;
        return;
    }

    fseek(arquivo, 0, SEEK_END);
    long tamanho_arquivo = ftell(arquivo);
    fseek(arquivo, 0, SEEK_SET);

    *num_palavras = tamanho_arquivo / sizeof(Palavra);
    *lista_palavras = (Palavra*)malloc(*num_palavras * sizeof(Palavra));
    if (*lista_palavras == NULL) {
        printf("Erro de alocacao de memoria para carregar palavras.\n");
        *num_palavras = 0;
        fclose(arquivo);
        return;
    }

    fread(*lista_palavras, sizeof(Palavra), *num_palavras, arquivo);
    fclose(arquivo);
}

// Função atualizar_palavra: Começa buscando todas as palavras salvas no arquivo "palavras.bin" utilizando a função carregar_palavras, depois faz um if e valida se encontrou a palavra digitada pelo usuario.
// Se encontrou, abre o arquivo com o modo wb, copia o conteudo e atualiza a palavra, senaão só mostra uma mensagem de aviso opr usuario.
void atualizar_palavra(const char* palavra_antiga, const Palavra* nova_palavra) {
    Palavra* lista_todas_palavras = NULL;
    int num_todas_palavras = 0;
    carregar_palavras(&lista_todas_palavras, &num_todas_palavras);

    int encontrada = 0;
    for (int i = 0; i < num_todas_palavras; i++) {
        if (strcmp(lista_todas_palavras[i].texto, palavra_antiga) == 0) {
            strcpy(lista_todas_palavras[i].texto, nova_palavra->texto);
            encontrada = 1;
            break;
        }
    }

    if (encontrada) {
        FILE* arquivo = fopen("palavras.bin", "wb"); // Abre para sobrescrever
        if (arquivo == NULL) {
            printf("Erro ao abrir o arquivo para atualizar.\n");
            free(lista_todas_palavras);
            return;
        }
        fwrite(lista_todas_palavras, sizeof(Palavra), num_todas_palavras, arquivo);
        fclose(arquivo);
        printf("Palavra '%s' atualizada para '%s' com sucesso!\n", palavra_antiga, nova_palavra->texto);
    }
    else {
        printf("Palavra '%s' nao encontrada para atualizacao.\n", palavra_antiga);
    }
    free(lista_todas_palavras);
}

void apagar_palavra(const char* palavra_apagar) {
    Palavra* lista_todas_palavras = NULL;
    int num_todas_palavras = 0;
    carregar_palavras(&lista_todas_palavras, &num_todas_palavras);

    // Verifica se a palavra digitada pelo usuario existe no arquivo.
    int indice_apagar = -1;
    for (int i = 0; i < num_todas_palavras; i++) {
        if (strcmp(lista_todas_palavras[i].texto, palavra_apagar) == 0) {
            indice_apagar = i;
            break;
        }
    }

    // Se existir entra nesse if que vai pegar o arquivo com o modo write, ou seja, vai apagar o conteudo daquele espaço, como funciona demostrado ->
    // Pega o arquivo palavras.bin inteiro, como abriu com o modo write vai apagar todo o conteudo, entra no for e recoloca todas as palavras antigas, menos a digitada pelo usuário, no ifnal da um free para liberar a memória.
    if (indice_apagar != -1) {
        FILE* arquivo = fopen("palavras.bin", "wb"); 
        if (arquivo == NULL) {
            printf("Erro ao abrir o arquivo para apagar.\n");
            free(lista_todas_palavras);
            return;
        }
        for (int i = 0; i < num_todas_palavras; i++) {
            if (i != indice_apagar) {
                fwrite(&lista_todas_palavras[i], sizeof(Palavra), 1, arquivo);
            }
        }
        fclose(arquivo);
        printf("Palavra '%s' apagada com sucesso!\n", palavra_apagar);
    }
    else {
        printf("Palavra '%s' nao encontrada para apagar.\n", palavra_apagar);
    }
    free(lista_todas_palavras);
}

// Função para mostrar todas as palavras salvas, chama a função carregar_palavras para capturar e ler o arquivo,
// e a partir disso ela faz um for que percorre todas as palavras exibindo as mesmas com o id delas e o nome da palavra
// no final de tudo faz um free para liberar a memória alocada
void mostrar_palavras() {
    Palavra* lista_todas_palavras = NULL;
    int num_todas_palavras = 0;
    carregar_palavras(&lista_todas_palavras, &num_todas_palavras);

    if (num_todas_palavras == 0) {
        printf("Nenhuma palavra cadastrada.\n");
    }
    else {
        printf("\n--- Palavras Cadastradas ---\n");
        for (int i = 0; i < num_todas_palavras; i++) {
            printf("%d. %s\n", i + 1, lista_todas_palavras[i].texto);
        }
        printf("----------------------------\n");
    }
    free(lista_todas_palavras);
}

// Funcoes de jogo
// Inicializa todas as variaveis com os valores zerados para não haver lixo de memoria.
void inicializar_jogo(Jogo* jogo) {
    jogo->matriz = NULL; // Previne tentar usar ou liberar algo que ainda não existe
    jogo->orig_matriz = NULL;
    jogo->linhas = 0; // Previne divisão por zero ou uso indevido
    jogo->colunas = 0;
    jogo->tentativas = 0;
    jogo->palavras_restantes = 0;
    for (int i = 0; i < NUM_PALAVRAS_JOGO; i++) {
        jogo->palavras_jogo[i] = NULL;
    }
}
// Serve para liberar a memória usada pela matriz do jogo, evitando desperdício de memória.
void liberar_jogo(Jogo* jogo) {
    if (jogo->matriz != NULL) {
        for (int i = 0; i < jogo->linhas; i++) {
            free(jogo->matriz[i]);
        }
        free(jogo->matriz);
        jogo->matriz = NULL;
    }

    if (jogo->orig_matriz) {
        for (int i = 0; i < jogo->linhas; i++) {
            free(jogo->orig_matriz[i]);
        }
        free(jogo->orig_matriz);
    }
    jogo->matriz = jogo->orig_matriz = NULL;
}

// Inicialmente libera a memória para caso tenha outra matriz aberta, depois pega o tamanho da linha e da coluna com o jogador, aloca a memoria necessaria para as linhas e valida se a alocação deu certo.
// Executa um for inicial para as linhas e dentro outro for para as colunas, alocando a memória necessaria para isso também, ai dentro do for verifica se a alocação das colunas deu certo, caso de errado da um exit.
void gerar_matriz(Jogo* jogo) {
    liberar_jogo(jogo); 

    // Pega o tamanho das linhas e colunas de matriz, sendo no minimo 7 e no maximo 9.
    jogo->linhas = obter_dimensao("linhas");
    jogo->colunas = obter_dimensao("colunas");

    // Aloca memória para as linhas da matriz.
    jogo->matriz = (char**)malloc(jogo->linhas * sizeof(char*));
    if (jogo->matriz == NULL) {
        printf("Erro de alocacao de memoria para linhas da matriz.\n");
        exit(1);
    }

    // A coluna tem um for pq ao contrario das linhas que são um vetor de ponteiros as colunas são o conteudo de cada linhas, por isso precisa do for.
    for (int i = 0; i < jogo->linhas; i++) {
        jogo->matriz[i] = (char*)malloc(jogo->colunas * sizeof(char));
        if (jogo->matriz[i] == NULL) {
            printf("Erro de alocacao de memoria para colunas da matriz.\n");
            exit(1);
        }
    }
}

// Receve a lista com todas as palavras e a quantidade de palavras totais, ai coloca todas as palavras como NULL para garantir que não há lixo de memória.
// Posteriormente aloca a memoria necessaria para as palavras e valida se a alocação foi um sucesso,
// Depois far um for para adicionar um valor aos indices sorteados baseado na quantidade de palavras
// E por fim, embaralha as palavras usando a função rand()
void sortear_palavras_jogo(Jogo* jogo, Palavra* lista_todas_palavras, int num_todas_palavras) {

    for (int i = 0; i < NUM_PALAVRAS_JOGO; i++) {
        jogo->palavras_jogo[i] = NULL;
    }

    int* indices_sorteados = (int*)malloc(num_todas_palavras * sizeof(int));
    if (indices_sorteados == NULL) {
        printf("Erro de alocacao de memoria para indices sorteados.\n");
        exit(1);
    }
    for (int i = 0; i < num_todas_palavras; i++) {
        indices_sorteados[i] = i;
    }

    for (int i = num_todas_palavras - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = indices_sorteados[i];
        indices_sorteados[i] = indices_sorteados[j];
        indices_sorteados[j] = temp;
    }

    // Valida se a palavra cabe na MATRIZ (O usuario não pode digitar uma palavra maior que 9 caracteres, mas pode acontecer dele jogar com uma matriz de 7 x 7 e ter palavras com 9 caracteres, ai precisei validar isso)
    jogo->palavras_restantes = 0;
    for (int i = 0; i < NUM_PALAVRAS_JOGO; i++) {
        if (strlen(lista_todas_palavras[indices_sorteados[i]].texto) <= jogo->linhas &&
            strlen(lista_todas_palavras[indices_sorteados[i]].texto) <= jogo->colunas) {
            jogo->palavras_jogo[jogo->palavras_restantes] = &lista_todas_palavras[indices_sorteados[i]];
            jogo->palavras_jogo[jogo->palavras_restantes]->encontrada = 0; 
            jogo->palavras_restantes++;
        }
        else {
            printf("Palavra '%s' muito grande para a matriz. Nao sera usada neste jogo.\n", lista_todas_palavras[indices_sorteados[i]].texto);
        }
    }
    free(indices_sorteados);

    if (jogo->palavras_restantes == 0) {
        printf("Nenhuma palavra sorteada que caiba na matriz. Por favor, tente novamente com dimensoes maiores ou palavras menores.\n");
    }
}

// Não tem mto segredo, mostra a matriz do jogo na tela com os índices. Letras normais aparecem como estão,
// e se alguma letra já foi encontrada (virou número), ela aparece como número mesmo.
void exibir_matriz(const Jogo* jogo) {
    printf("\n   ");
    for (int j = 0; j < jogo->colunas; j++) {
        printf("%2d ", j);
    }
    printf("\n");
    for (int i = 0; i < jogo->linhas; i++) {
        printf("%2d ", i);
        for (int j = 0; j < jogo->colunas; j++) {
            if (jogo->matriz[i][j] >= '0' && jogo->matriz[i][j] <= '9') {
                printf(" %c ", jogo->matriz[i][j]); // Exibe o numero encontrado
            }
            else {
                printf(" %c ", jogo->matriz[i][j]);
            }
        }
        printf("\n");
    }
}

// Fica exibindo o status do jogo conforme o usuario vai jogando.
void exibir_status_jogo(const Jogo* jogo) {
    printf("\n--- Status do Jogo ---\n");
    printf("Tentativas: %d\n", jogo->tentativas);
    printf("Palavras restantes: %d\n", jogo->palavras_restantes);
    printf("Palavras a encontrar:\n");
    for (int i = 0; i < NUM_PALAVRAS_JOGO; i++) {
        if (jogo->palavras_jogo[i] != NULL && jogo->palavras_jogo[i]->encontrada == 0) {
            printf("- %s\n", jogo->palavras_jogo[i]->texto);
        }
    }
    printf("----------------------\n");
}

int verificar_palavra(Jogo* jogo, int r1, int c1, int r2, int c2) {
    // Extrai a sequencia de letras da matriz entre (r1,c1) e (r2,c2).
    // Depois vai comparar essa sequencia com as palavras em jogo->palavras_jogo.
    // Se encontrar, marcar a palavra como encontrada, substituir na matriz e decrementa palavras_restantes.
    // Retornar 1 se encontrou, 0 caso contrario.

    // Determinar a direcao do movimento
    int dr = 0, dc = 0;
    if (r1 < r2) dr = 1;
    else if (r1 > r2) dr = -1;

    if (c1 < c2) dc = 1;
    else if (c1 > c2) dc = -1;

    // Só pra validar se é um unico ponto
    if (dr == 0 && dc == 0) return 0;

    // Construir a palavra selecionada
    char palavra_selecionada[MAX_PALAVRA_LEN];
    int k = 0;
    int r = r1;
    int c = c1;

    while (is_valid(r, c, jogo->linhas, jogo->colunas)) {
        palavra_selecionada[k++] = jogo->orig_matriz[r][c];
        if (r == r2 && c == c2) break; // Chegou ao fim da selecao
        r += dr;
        c += dc;
    }
    palavra_selecionada[k] = '\0';

    // Verificar se a palavra selecionada corresponde a alguma palavra do jogo
    for (int i = 0; i < NUM_PALAVRAS_JOGO; i++) {
        if (jogo->palavras_jogo[i] != NULL && jogo->palavras_jogo[i]->encontrada == 0) {
            if (strcmp(palavra_selecionada, jogo->palavras_jogo[i]->texto) == 0) {
                // Palavra encontrada! Substituir na matriz e marcar como encontrada
                char num_encontrado_char = '1' + (NUM_PALAVRAS_JOGO - jogo->palavras_restantes);

                r = r1;
                c = c1;
                while (is_valid(r, c, jogo->linhas, jogo->colunas)) {
                    jogo->matriz[r][c] = num_encontrado_char;
                    if (r == r2 && c == c2) break;
                    r += dr;
                    c += dc;
                }

                jogo->palavras_jogo[i]->encontrada = 1;
                jogo->palavras_restantes--;
                printf("Parabens! Voce encontrou a palavra '%s'!\n", palavra_selecionada);
                return 1;
            }
        }
    }
    return 0;
}

void preencher_espacos_aleatorios(Jogo* jogo) {
    // Esta funcao deve ser chamada depois que as palavras forem inseridas na matriz.
    // Ela preenche os espacos vazios com letras aleatorias, basicamente vai preencher tudo oq não foi usado com as palavras de antes.
    for (int i = 0; i < jogo->linhas; i++) {
        for (int j = 0; j < jogo->colunas; j++) {
            // Ele assume que ' ' representa um espaco vazio apos a insercao das palavras
            if (jogo->matriz[i][j] == ' ') {
                jogo->matriz[i][j] = 'a' + (rand() % 26);
            }
        }
    }
}

// Funcoes auxiliares
// Pega o tamanho da matriz que o usuario digitar, usado para as linhas e colunas, depois faz um if que verifica se atende o padrão de tamanho da matriz.
int obter_dimensao(const char* tipo) {
    int dimensao;
    do {
        printf("Digite o numero de %s (entre %d e %d): ", tipo, MIN_DIMENSAO, MAX_DIMENSAO);
        scanf("%d", &dimensao);
        limpar_buffer();
        if (dimensao < MIN_DIMENSAO || dimensao > MAX_DIMENSAO) {
            printf("Dimensao invalida. Tente novamente.\n");
        }
    } while (dimensao < MIN_DIMENSAO || dimensao > MAX_DIMENSAO);
    return dimensao;
}

int palavra_existe(const char* palavra) {
    Palavra* lista_todas_palavras = NULL;
    int num_todas_palavras = 0;
    carregar_palavras(&lista_todas_palavras, &num_todas_palavras);

    for (int i = 0; i < num_todas_palavras; i++) {
        if (strcmp(lista_todas_palavras[i].texto, palavra) == 0) {
            free(lista_todas_palavras);
            return 1;
        }
    }
    free(lista_todas_palavras);
    return 0;
}
// Serve para limpar o buffer de entrada e garantir que não haja resquicios de enter no scanf, garantindo uma nova entrada de dados limpa.
void limpar_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void menu_principal() {
    int opcao;
    Palavra nova_palavra;
    char palavra_temp[MAX_PALAVRA_LEN];
    char palavra_antiga[MAX_PALAVRA_LEN];
    Jogo jogo_atual;
    inicializar_jogo(&jogo_atual);

    srand(time(NULL)); // Vai ser utilizado para gerar numeros aleatorios *_*

    do {
        printf("\n--- Menu Principal ---\n");
        printf("1. Novo Jogo\n");
        printf("2. Cadastrar palavra\n");
        printf("3. Atualizar palavra\n");
        printf("4. Apagar palavra\n");
        printf("5. Mostrar palavras\n");
        printf("6. Sair\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &opcao);
        limpar_buffer();

        switch (opcao) {
        case 1: {
            Palavra* todas_palavras = NULL;
            int num_todas_palavras = 0;
            // Pega todas as palavras do arquivo binário "palavras.bin".
            carregar_palavras(&todas_palavras, &num_todas_palavras);

            // Valida se o numero de palavras é menor que minimo necessario, como o minimo é sempre 10, foi gravado em uma variavel global.
            if (num_todas_palavras < MIN_PALAVRAS_ARQUIVO) {
                printf("E necessario ter no minimo %d palavras cadastradas para iniciar um novo jogo.\n", MIN_PALAVRAS_ARQUIVO);
                free(todas_palavras);
                break;
            }

            int jogar_novamente = 1;
            while (jogar_novamente == 1) {
                inicializar_jogo(&jogo_atual);
                gerar_matriz(&jogo_atual);
                sortear_palavras_jogo(&jogo_atual, todas_palavras, num_todas_palavras);
                inserir_palavras_na_matriz(&jogo_atual);

                if (jogo_atual.palavras_restantes == 0) {
                    printf("Nao foi possivel iniciar o jogo com as palavras sorteadas e dimensoes da matriz. Tente novamente.\n");
                    liberar_jogo(&jogo_atual);
                    break;
                }

                // Aqui fica a logica do jogo em si.
                while (jogo_atual.palavras_restantes > 0) {
                    exibir_matriz(&jogo_atual);
                    exibir_status_jogo(&jogo_atual);

                    int r1, c1, r2, c2;
                    printf("Digite as coordenadas de inicio (linha coluna) ou -1 para sair: \n");
                    scanf("%d", &r1);
                    if (r1 == -1) break;
                    scanf("%d", &c1);
                    printf("Digite as coordenadas de fim (linha coluna): \n");
                    scanf("%d", &r2);
                    scanf("%d", &c2);
                    limpar_buffer();

                    jogo_atual.tentativas++;
                    if (!verificar_palavra(&jogo_atual, r1, c1, r2, c2)) {
                        printf("Palavra nao encontrada ou selecao invalida. Tente novamente.\n");
                    }
                }

                if (jogo_atual.palavras_restantes == 0) {
                    printf("\nParabens! Voce encontrou todas as palavras em %d tentativas!\n", jogo_atual.tentativas);

                    printf("      ____________\n");
                    printf("     '-._==_==_.-'\n");
                    printf("      .-\\:      /-.\n");
                    printf("     | (|:.     |) |\n");
                    printf("      '-|:.     |-'\n");
                    printf("        \\::.    /\n");
                    printf("         '::. .'\n");
                    printf("           ) (\n");
                    printf("         _.' '._\n");
                    printf("        *\"\"\"\"\"\"\"*\n");
                }

                jogar_novamente = menu_pos_jogo(); // 1 para novo jogo, 0 para voltar
                liberar_jogo(&jogo_atual);
            }
            free(todas_palavras);
            break;
        }
        case 2:
            printf("Digite a nova palavra (min 5 letras): ");
            fgets(nova_palavra.texto, MAX_PALAVRA_LEN, stdin);
            nova_palavra.texto[strcspn(nova_palavra.texto, "\n")] = 0; // Remove newline
            // Faz as validações referente ao tamanho da palavra digitada pelo usuário.
            if (strlen(nova_palavra.texto) < 5) {
                printf("A palavra deve ter no minimo 5 letras.\n");
            }
            else if (strlen(nova_palavra.texto) > 9) {
                printf("A palavra nao pode ter mais de 9 letras");
            }
            else if (palavra_existe(nova_palavra.texto)) {
                printf("A palavra '%s' ja existe no cadastro.\n", nova_palavra.texto);
            }
            else {
                salvar_palavra(&nova_palavra);
            }
            break;
        case 3:
            printf("Digite a palavra antiga a ser atualizada: ");
            fgets(palavra_antiga, MAX_PALAVRA_LEN, stdin);
            palavra_antiga[strcspn(palavra_antiga, "\n")] = 0;

            if (!palavra_existe(palavra_antiga)) {
                printf("Palavra '%s' nao encontrada para atualizacao.\n", palavra_antiga);
                break;
            }

            printf("Digite a nova palavra: ");
            fgets(nova_palavra.texto, MAX_PALAVRA_LEN, stdin);
            nova_palavra.texto[strcspn(nova_palavra.texto, "\n")] = 0;
            if (strlen(nova_palavra.texto) < 5) {
                printf("A nova palavra deve ter no minimo 5 letras.\n");
            }
            else if (palavra_existe(nova_palavra.texto)) {
                printf("A palavra '%s' ja existe no cadastro. Nao eh possivel atualizar para uma palavra existente.\n", nova_palavra.texto);
            }
            else {
                atualizar_palavra(palavra_antiga, &nova_palavra);
            }
            break;
        case 4:
            printf("Digite a palavra a ser apagada: ");
            // Captura a palavra digitada pelo usuário usando fgets para controle de buffer overflow e tamanho da variavel.
            fgets(palavra_temp, MAX_PALAVRA_LEN, stdin);
            // Retira o enter do final da linha, essencial para garantir a funcionalidade do código.
            palavra_temp[strcspn(palavra_temp, "\n")] = 0;
            apagar_palavra(palavra_temp);
            break;
        case 5:
            mostrar_palavras();
            break;
        case 6:
            printf("Saindo do jogo. Ate mais!\n");
            break;
        default:
            printf("Opcao invalida. Tente novamente.\n");
        }
    } while (opcao != 6);
}


/// Menu que vai ser exibido quando o jogo acabar, ou seja, quando o usuário encontrar todas as palavras, pedindo se a pessoa quer começar um novo jogo, sair para o menu inicial ou encerrar o programa.
int menu_pos_jogo() {
    int opcao;
    do {
        printf("\n--- Fim de Jogo ---\n");
        printf("1. Novo Jogo\n");
        printf("2. Menu Inicial\n");
        printf("3. Sair\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &opcao);
        limpar_buffer();

        switch (opcao) {
        case 1:
			return 1; // Retorna 1 para indicar que o jogador quer jogar novamente
        case 2:
            return 0;
        case 3:
            printf("Saindo do jogo. Ate mais!\n");
            exit(0);
        default:
            printf("Opcao invalida. Tente novamente.\n");
        }
    } while (opcao != 1 && opcao != 2 && opcao != 3);
}

// Função incial, onde começa o programa de fato.
int main() {
    menu_principal();
    return 0;
}

// Funcao auxiliar para verificar se uma posicao e valida na matriz.
int is_valid(int r, int c, int linhas, int colunas) {
    return (r >= 0 && r < linhas && c >= 0 && c < colunas);
}

// Funcao para tentar colocar as palavras na matriz.
int tentar_colocar_palavra(Jogo* jogo, const char* palavra_texto, int r_start, int c_start, int dr, int dc) {
    int len = strlen(palavra_texto);
    // Verificar se a palavra cabe na direcao e posicao
    for (int i = 0; i < len; i++) {
        int r = r_start + i * dr;
        int c = c_start + i * dc;
        if (!is_valid(r, c, jogo->linhas, jogo->colunas) || (jogo->matriz[r][c] != ' ' && jogo->matriz[r][c] != palavra_texto[i])) {
            return 0; // Nao cabe ou colide com letra diferente
        }
    }

    // Se cabe, coloca a palavra
    for (int i = 0; i < len; i++) {
        int r = r_start + i * dr;
        int c = c_start + i * dc;
        jogo->matriz[r][c] = palavra_texto[i];
    }
    return 1;
}

void inserir_palavras_na_matriz(Jogo* jogo) {
    // Inicializa a matriz com espacos em branco
    for (int i = 0; i < jogo->linhas; i++) {
        for (int j = 0; j < jogo->colunas; j++) {
            jogo->matriz[i][j] = ' ';
        }
    }

    int direcoes[8][2] = {
        {0, 1}, {0, -1},   // Horizontal (direita, esquerda)
        {1, 0}, {-1, 0},   // Vertical (baixo, cima)
        {1, 1}, {1, -1},   // Diagonal (baixo-direita, baixo-esquerda)
        {-1, 1}, {-1, -1} // Diagonal (cima-direita, cima-esquerda)
    };

// Aqui vai tentar posicionar cada palavra em uma direção e posição aleatória, para garantir que ela caiba e não sobrescreva letras incompatíveis.
// Após isso, preenche os espaços vazios restantes com letras aleatórias.
    for (int p_idx = 0; p_idx < jogo->palavras_restantes; p_idx++) {
        Palavra* palavra = jogo->palavras_jogo[p_idx];
        int palavra_colocada = 0;
        int tentativas_colocacao = 0;
        while (!palavra_colocada && tentativas_colocacao < 1000) { // Limite de tentativas para evitar loop infinito
            int r_start = rand() % jogo->linhas;
            int c_start = rand() % jogo->colunas;
            int dir_idx = rand() % 8;
            int dr = direcoes[dir_idx][0];
            int dc = direcoes[dir_idx][1];

            if (tentar_colocar_palavra(jogo, palavra->texto, r_start, c_start, dr, dc)) {
                palavra_colocada = 1;
            }
            tentativas_colocacao++;
        }
        if (!palavra_colocada) {
            printf("Nao foi possivel colocar a palavra '%s' na matriz. Pode haver problemas de espaco.\n", palavra->texto); // Sem tratamento ainda pq não me deu tempo :/
        }
    }
    preencher_espacos_aleatorios(jogo);

    jogo->orig_matriz = malloc(jogo->linhas * sizeof(char*));
    for (int i = 0; i < jogo->linhas; i++) {
        jogo->orig_matriz[i] = malloc(jogo->colunas * sizeof(char));
        memcpy(jogo->orig_matriz[i], jogo->matriz[i], jogo->colunas * sizeof(char));
    }
}
