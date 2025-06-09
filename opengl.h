#ifndef MYOPENGL_H
#define MYOPENGL_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include "gl/glut.h"
#include <windows.h> // somente no Windows
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

typedef struct Node Node;

struct Node {
  char name[20];      // nome
  float offset[3];    // offset (deslocamento)
  int channels;       // qtd de canais (3 ou 6)
  float *channelData; // vetor com os dados dos canais
  int numChildren;    // qtd de filhos
  Node *parent;       // ponteiro para o pai
  Node *children;     // ponteiro para o primeiro filho (ou NULL)
  Node *next;         // ponteiro para o próximo filho (ou NULL)
};

void renderBone(float x0, float y0, float z0, float x1, float y1, float z1);
void drawNode(Node *node);
void drawSkeleton();
void drawFloor();
void mouse(int button, int state, int x, int y);
void move(int x, int y);
void posUser();
void reshape(int w, int h);
void display();
void keyboard(unsigned char key, int x, int y);
void arrow_keys(int a_keys, int x, int y);
void init();

#endif

