// **********************************************************************
//	BVHViewer.c
//  Desenha e anima um esqueleto a partir de um arquivo BVH (BioVision)
//  Marcelo Cohen
//  marcelo.cohen@pucrs.br
// **********************************************************************

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "opengl.h"

#define MAX_NAME_LENGTH 20
#define MAX_LINE_LENGTH 256

// Raiz da hierarquia
Node *root;

// Total de frames
int totalFrames;

// Frame atual
int curFrame = 0;

// Funcoes para liberacao de memoria da hierarquia
void freeTree();
void freeNode(Node *node);

// Funcao externa para inicializacao da OpenGL
void init();

// Funcao de teste para criar um esqueleto inicial
void initMaleSkel();

// Funcoes para criacao de nodos e aplicacao dos valores de um frame
Node *createNode(char *name, Node *parent, int numChannels, float ofx,
                 float ofy, float ofz);
void applyData(float data[], Node *n);
void apply();

// **********************************************************************
//  Cria um nodo novo para a hierarquia, fazendo também a ligacao com
//  o seu pai (se houver)
//  Parametros:
//  - name: string com o nome do nodo
//  - parent: ponteiro para o nodo pai (NULL se for a raiz)
//  - numChannels: quantidade de canais de transformacao (3 ou 6)
//  - ofx, ofy, ofz: offset (deslocamento) lido do arquivo
// **********************************************************************
Node *createNode(char *name, Node *parent, int numChannels, float ofx,
                 float ofy, float ofz) {
  Node *aux = malloc(sizeof(Node));
  aux->channels = numChannels;
  aux->channelData = calloc(sizeof(float), numChannels);
  strcpy(aux->name, name);
  aux->offset[0] = ofx;
  aux->offset[1] = ofy;
  aux->offset[2] = ofz;
  aux->numChildren = 0;
  aux->children = NULL;
  aux->parent = parent;
  aux->next = NULL;
  if (parent) {
    if(parent->children == NULL) {
      // printf("First child: %s\n", aux->name);
      parent->children = aux;
    }
    else {
      Node* ptr = parent->children;
      while(ptr->next != NULL)
        ptr = ptr->next;
      // printf("Next child: %s\n", aux->name);
      ptr->next = aux;
    }
    parent->numChildren++;
  }
  printf("Created %s\n", name);
  return aux;
}

//
// DADOS DE EXEMPLO DO PRIMEIRO FRAME
//
// Pos. da aplicacao dos dados
int dataPos;

void applyData(float data[], Node *n) {
  // printf("%s:\n", n->name);
  if (n->numChildren == 0)
    return;
  for (int c = 0; c < n->channels; c++) {
    // printf("   %d -> %f\n", c, data[dataPos]);
    n->channelData[c] = data[dataPos++];
  }
  Node* ptr = n->children;
  while(ptr != NULL) {
    applyData(data, ptr);
    ptr = ptr->next;
  }
}

float* parseMotion(FILE *file) {
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return NULL;
    }

    char line[1024]; 
    int frames = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "MOTION", 6) == 0) {
            break;
        }
    }

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Frames:", 7) == 0) {
            sscanf(line, "Frames: %d", &frames);
            break;
        }
    }

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Frame Time:", 10) == 0) {
            continue;
        }
        break; 
    }

    int max_values = frames * 100; // Supõe até 100 valores por linha
    float *data = (float*) malloc(max_values * sizeof(float));
    if (data == NULL) {
        perror("Erro ao alocar memória");
        fclose(file);
        return NULL;
    }

    int count = 0;

    // Lê os valores 
    do {
        char *token = strtok(line, " ");
        while (token != NULL) {
            data[count++] = atof(token); // Converte string para float e adiciona ao vetor
            token = strtok(NULL, " ");
        }
    } while (fgets(line, sizeof(line), file));

    // Ajusta o tamanho do vetor
    data = (float*) realloc(data, count * sizeof(float));
    if (data == NULL) {
        perror("Erro ao realocar memória");
        fclose(file);
        return NULL;
    }

    fclose(file);
    return data;
}


void trim_prefix(char *str) {
    if (str == NULL) return;
    
    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

Node *parseRoot(FILE *file) {
    char line[MAX_LINE_LENGTH];
    char name[MAX_NAME_LENGTH];
    
    if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "Unexpected end of file while parsing root\n");
        return NULL;
    }
    
    trim_prefix(line);
    if (strncmp(line, "ROOT", 4) != 0) {
        fprintf(stderr, "Expected ROOT, got: %s\n", line);
        return NULL;
    }
    
    if (sscanf(line, "ROOT %19s", name) != 1) {
        fprintf(stderr, "Failed to parse root name\n");
        return NULL;
    }
    
    return createNode(name, NULL, 6, 0, 0, 0);
}

Node *parseBody(FILE *file, Node *parent) {
    char line[MAX_LINE_LENGTH];
    char keyword[256];
    char childName[64];
    float ofx, ofy, ofz;
    int numChannels;
    int braceLevel = 1;  // Start with 1 brace level for the parent

    while (fgets(line, sizeof(line), file)) {
        trim_prefix(line);
        
        if (line[0] == '\0' || line[0] == '\n') continue;

        // Count braces to know when we've exited the current hierarchy
        if (strchr(line, '{')) braceLevel++;
        if (strchr(line, '}')) braceLevel--;

        // Exit condition: we've closed all braces for this hierarchy
        if (braceLevel == 0) {
            return parent;
        }

        if (sscanf(line, "%s", keyword) != 1) continue;
        
        if (strcmp(keyword, "OFFSET") == 0) {
            if (sscanf(line, "OFFSET %f %f %f", &ofx, &ofy, &ofz) != 3) {
                fprintf(stderr, "Invalid OFFSET format\n");
                return NULL;
            }
            // Update node's offset 
            parent->offset[0] = ofx;
            parent->offset[1] = ofy;
            parent->offset[2] = ofz;
        } 
        else if (strcmp(keyword, "CHANNELS") == 0) {
            if (sscanf(line, "CHANNELS %d", &numChannels) != 1) {
                fprintf(stderr, "Invalid CHANNELS format\n");
                return NULL;
            }
            parent->channels = numChannels;
            if (parent->channelData) free(parent->channelData);
            parent->channelData = calloc(numChannels, sizeof(float));
        } 
        else if (strcmp(keyword, "JOINT") == 0 || strcmp(keyword, "End") == 0) {
            if (strcmp(keyword, "JOINT") == 0) {
                if (sscanf(line, "JOINT %63s", childName) != 1) {
                    fprintf(stderr, "Invalid JOINT format\n");
                    return NULL;
                }
                Node *child = createNode(childName, parent, 0, 0, 0, 0);
                if (child == NULL) return NULL;
                
                if (parseBody(file, child) == NULL) {
                    return NULL;
                }
            }
            else if (strcmp(keyword, "End") == 0) {
                if (fgets(line, sizeof(line), file) == NULL) {
                    fprintf(stderr, "Unexpected end of file\n");
                    return NULL;
                }
                
                if (fgets(line, sizeof(line), file) == NULL) {
                    fprintf(stderr, "Unexpected end of file\n");
                    return NULL;
                }
                
                if (sscanf(line, " OFFSET %f %f %f", &ofx, &ofy, &ofz) != 3) {
                    fprintf(stderr, "Invalid End Site OFFSET\n");
                    return NULL;
                }
                
                Node *endSite = createNode("End Site", parent, 0, ofx, ofy, ofz);
                return endSite;
            }
        }
    }
    
    return parent;
}

Node *parseHierarchy(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open file");
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    Node *root = NULL;

    if (fgets(line, sizeof(line), file) == NULL || strncmp(line, "HIERARCHY", 9) != 0)
    {
        fprintf(stderr, "Invalid file format: Missing HIERARCHY\n");
        fclose(file);
        return NULL;
    }

    root = parseRoot(file);
    if (root == NULL)
    {
        fclose(file);
        return NULL;
    }

    if (parseBody(file, root) == NULL)
    {
        freeNode(root);
        fclose(file);
        return NULL;
    }

    //até essa parte foi testado debuggando, esta 100% funcional, as funcoes parseRoot e parseBody tb foram testadas debuggando e estao funcionando
    float* data = parseMotion(file);
    fclose(file);                       //nao sabemos se estamos aplicando a func pronta applydata no lugar/momento correto da exec
    return root;
    applyData(data, root);
}
void freeTree() { freeNode(root); }

void freeNode(Node *node) {
  // printf("Freeing %s %p\n", node->name,node->children);
  if (node == NULL)
    return;
  // printf("Freeing children...\n");
  Node* ptr = node->children;
  Node* aux;
  while(ptr != NULL) {
    aux = ptr->next;
    freeNode(ptr);
    ptr = aux;
  }
  // printf("Freeing channel data...\n");
  free(node->channelData);
}

// **********************************************************************
//  Programa principal
// **********************************************************************
int main(int argc, char **argv) {
  root = parseHierarchy("bvh/Male2_G16_DoubleKick.bvh");
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
  glutInitWindowPosition(0, 0);

  // Define o tamanho inicial da janela grafica do programa
  glutInitWindowSize(650, 500);

  // Cria a janela na tela, definindo o nome da
  // que aparecera na barra de título da janela.
  glutCreateWindow("BVH Viewer");

  // executa algumas inicializações
  init();

  // Exemplo: monta manualmente um esqueleto
  // (no trabalho, deve-se ler do arquivo)
  //initMaleSkel();

  // Define que o tratador de evento para
  // o redesenho da tela. A funcao "display"
  // será chamada automaticamente quando
  // for necessário redesenhar a janela
  glutDisplayFunc(display);

  // Define que a função que irá rodar a
  // continuamente. Esta função é usada para fazer animações
  // A funcao "display" será chamada automaticamente
  // sempre que for possível
  // glutIdleFunc ( display );

  // Define que o tratador de evento para
  // o redimensionamento da janela. A funcao "reshape"
  // será chamada automaticamente quando
  // o usuário alterar o tamanho da janela
  glutReshapeFunc(reshape);

  // Define que o tratador de evento para
  // as teclas. A funcao "keyboard"
  // será chamada automaticamente sempre
  // o usuário pressionar uma tecla comum
  glutKeyboardFunc(keyboard);

  // Define que o tratador de evento para
  // as teclas especiais(F1, F2,... ALT-A,
  // ALT-B, Teclas de Seta, ...).
  // A funcao "arrow_keys" será chamada
  // automaticamente sempre o usuário
  // pressionar uma tecla especial
  glutSpecialFunc(arrow_keys);

  // Registra a função callback para eventos de botões do mouse
  glutMouseFunc(mouse);

  // Registra a função callback para eventos de movimento do mouse
  glutMotionFunc(move);

  // inicia o tratamento dos eventos
  glutMainLoop();
}
