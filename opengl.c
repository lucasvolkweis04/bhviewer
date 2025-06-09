#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opengl.h"

#ifdef WIN32
#include "gl/glut.h"
#include <windows.h> // somente no Windows
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

// Raiz da hierarquia
extern Node *root;

// Total de frames
extern int totalFrames;

// Frame atual
extern int curFrame;

// Funcoes para liberacao de memoria da hierarquia
void freeTree();
void freeNode(Node *node);
void apply();

// Variaveis globais para manipulacao da visualizacao 3D
int width, height;
float deltax = 0, deltay = 0;
GLfloat angle = 60, fAspect = 1.0;
GLfloat rotX = 0, rotY = 0, rotX_ini = 0, rotY_ini = 0;
GLfloat ratio;
GLfloat angY, angX;
int x_ini = 0, y_ini = 0, bot = 0;
float Obs[3] = {0, 0, -500};
float Alvo[3];
float ObsIni[3];

// Desenha um segmento do esqueleto (bone)
void renderBone(float x0, float y0, float z0, float x1, float y1, float z1) {
  GLdouble dir_x = x1 - x0;
  GLdouble dir_y = y1 - y0;
  GLdouble dir_z = z1 - z0;
  GLdouble bone_length = sqrt(dir_x * dir_x + dir_y * dir_y + dir_z * dir_z);

  static GLUquadricObj *quad_obj = NULL;
  if (quad_obj == NULL)
    quad_obj = gluNewQuadric();
  gluQuadricDrawStyle(quad_obj, GLU_FILL);
  gluQuadricNormals(quad_obj, GLU_SMOOTH);

  glPushMatrix();

  glTranslated(x0, y0, z0);

  double length;
  length = sqrt(dir_x * dir_x + dir_y * dir_y + dir_z * dir_z);
  if (length < 0.0001) {
    dir_x = 0.0;
    dir_y = 0.0;
    dir_z = 1.0;
    length = 1.0;
  }
  dir_x /= length;
  dir_y /= length;
  dir_z /= length;

  GLdouble up_x, up_y, up_z;
  up_x = 0.0;
  up_y = 1.0;
  up_z = 0.0;

  double side_x, side_y, side_z;
  side_x = up_y * dir_z - up_z * dir_y;
  side_y = up_z * dir_x - up_x * dir_z;
  side_z = up_x * dir_y - up_y * dir_x;

  length = sqrt(side_x * side_x + side_y * side_y + side_z * side_z);
  if (length < 0.0001) {
    side_x = 1.0;
    side_y = 0.0;
    side_z = 0.0;
    length = 1.0;
  }
  side_x /= length;
  side_y /= length;
  side_z /= length;

  up_x = dir_y * side_z - dir_z * side_y;
  up_y = dir_z * side_x - dir_x * side_z;
  up_z = dir_x * side_y - dir_y * side_x;

  GLdouble m[16] = {side_x, side_y, side_z, 0.0, up_x, up_y, up_z, 0.0,
                    dir_x,  dir_y,  dir_z,  0.0, 0.0,  0.0,  0.0,  1.0};
  glMultMatrixd(m);

  GLdouble radius = 3;   // raio
  GLdouble slices = 8.0; // fatias horizontais
  GLdouble stack = 3.0;  // fatias verticais

  // Desenha como cilindros
  gluCylinder(quad_obj, radius, radius, bone_length, slices, stack);

  glPopMatrix();
}

// Desenha um nodo da hierarquia (chamada recursiva)
void drawNode(Node *node) {
  int c = 0;
  glPushMatrix();

  if (node->channels == 6) {
    glTranslatef(node->channelData[0], node->channelData[1],
                 node->channelData[2]);
    c = 3;
  } else
    glTranslatef(node->offset[0], node->offset[1], node->offset[2]);
  glRotatef(node->channelData[c++], 0, 0, 1);
  glRotatef(node->channelData[c++], 1, 0, 0);
  glRotatef(node->channelData[c++], 0, 1, 0);

  if (node->numChildren == 0)
    renderBone(0, 0, 0, node->offset[0], node->offset[1], node->offset[2]);
  else if (node->numChildren == 1) {
    Node *child = node->children;
    renderBone(0, 0, 0, child->offset[0], child->offset[1], child->offset[2]);
  } else {
    int nc = 0;
    float center[3] = {0.0f, 0.0f, 0.0f};
    Node* child = node->children;
    while(child) {
      nc++;
      center[0] += child->offset[0];
      center[1] += child->offset[1];
      center[2] += child->offset[2];
      child = child->next;
    }
    center[0] /= nc + 1;
    center[1] /= nc + 1;
    center[2] /= nc + 1;

    renderBone(0.0f, 0.0f, 0.0f, center[0], center[1], center[2]);

    child = node->children;
    while(child) {
      renderBone(center[0], center[1], center[2], child->offset[0],
                 child->offset[1], child->offset[2]);
      child = child->next;
    }
  }

  Node* child = node->children;
  while(child) {
    drawNode(child);
    child = child->next;
  }
  glPopMatrix();
}

void drawSkeleton() { drawNode(root); }

// **********************************************************************
//  Desenha um quadriculado para representar um piso
// **********************************************************************
void drawFloor() {
  float size = 50;
  int num_x = 100, num_z = 100;
  double ox, oz;
  glBegin(GL_QUADS);
  glNormal3d(0.0, 1.0, 0.0);
  ox = -(num_x * size) / 2;
  for (int x = 0; x < num_x; x++, ox += size) {
    oz = -(num_z * size) / 2;
    for (int z = 0; z < num_z; z++, oz += size) {
      if (((x + z) % 2) == 0)
        glColor3f(1.0, 1.0, 1.0);
      else
        glColor3f(0.8, 0.8, 0.8);
      glVertex3d(ox, 0.0, oz);
      glVertex3d(ox, 0.0, oz + size);
      glVertex3d(ox + size, 0.0, oz + size);
      glVertex3d(ox + size, 0.0, oz);
    }
  }
  glEnd();
}

// Função callback para eventos de botões do mouse
void mouse(int button, int state, int x, int y) {
  if (state == GLUT_DOWN) {
    // Salva os parâmetros atuais
    x_ini = x;
    y_ini = y;
    ObsIni[0] = Obs[0];
    ObsIni[1] = Obs[1];
    ObsIni[2] = Obs[2];
    rotX_ini = rotX;
    rotY_ini = rotY;
    bot = button;
  } else
    bot = -1;
}

// Função callback para eventos de movimento do mouse
#define SENS_ROT 5.0
#define SENS_OBS 5.0
void move(int x, int y) {
  // Botão esquerdo ?
  if (bot == GLUT_LEFT_BUTTON) {
    // Calcula diferenças
    int deltax = x_ini - x;
    int deltay = y_ini - y;
    // E modifica ângulos
    rotY = rotY_ini - deltax / SENS_ROT;
    rotX = rotX_ini - deltay / SENS_ROT;
  }
  // Botão direito ?
  else if (bot == GLUT_RIGHT_BUTTON) {
    // Calcula diferença
    int deltaz = y_ini - y;
    // E modifica distância do observador
    // Obs.x = x;
    // Obs.y = y;
    Obs[2] = ObsIni[2] - deltaz / SENS_OBS;
  }
  // PosicionaObservador();
  glutPostRedisplay();
}

// **********************************************************************
//  Posiciona observador
// **********************************************************************
void posUser() {
  // Set the clipping volume
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, ratio, 0.01, 2000);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Especifica posição do observador e do alvo
  glTranslatef(Obs[0], Obs[1], Obs[2]);
  glRotatef(rotX, 1, 0, 0);
  glRotatef(rotY, 0, 1, 0);
}

// **********************************************************************
//  Callback para redimensionamento da janela OpenGL
// **********************************************************************
void reshape(int w, int h) {
  // Prevent a divide by zero, when window is too short
  // (you cant make a window of zero width).
  if (h == 0)
    h = 1;

  ratio = 1.0f * w / h;
  // Reset the coordinate system before modifying
  glMatrixMode(GL_PROJECTION);
  // glLoadIdentity();
  //  Set the viewport to be the entire window
  glViewport(0, 0, w, h);

  posUser();
}

// **********************************************************************
//  Callback para desenho da tela
// **********************************************************************
void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  posUser();

  glMatrixMode(GL_MODELVIEW);

  drawFloor();

  glPushMatrix();
  glColor3f(0.7, 0.0, 0.0); // vermelho
  drawSkeleton();
  glPopMatrix();

  glutSwapBuffers();
}

// **********************************************************************
//  Callback para eventos de teclado
// **********************************************************************
void keyboard(unsigned char key, int x, int y) {
  switch (key) {
  case 27: // Termina o programa qdo
    freeTree();
    exit(0); // a tecla ESC for pressionada
    break;

  default:
    break;
  }
}

// **********************************************************************
//  Callback para eventos de teclas especiais
// **********************************************************************
void arrow_keys(int a_keys, int x, int y) {
  float passo = 3.0;
  switch (a_keys) {
  case GLUT_KEY_RIGHT:
    if (++curFrame >= totalFrames)
      curFrame = 0;
    apply();
    glutPostRedisplay();
    break;
  case GLUT_KEY_LEFT:
    if (--curFrame < 0)
      curFrame = totalFrames - 1;
    apply();
    glutPostRedisplay();
    break;
  case GLUT_KEY_UP:
    //
    glutPostRedisplay();
    break;
  case GLUT_KEY_DOWN:
    //
    glutPostRedisplay();
    break;
  default:
    break;
  }
}

// **********************************************************************
//	Inicializa os parâmetros globais de OpenGL
// **********************************************************************
void init() {
  // Parametros da fonte de luz
  float light0_position[] = {10.0, 100.0, 100.0, 1.0};
  float light0_diffuse[] = {0.8, 0.8, 0.8, 1.0};
  float light0_specular[] = {1.0, 1.0, 1.0, 1.0};
  float light0_ambient[] = {0.1, 0.1, 0.1, 1.0};

  // Ajusta
  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
  glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);

  // Habilita estados necessarios
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_DEPTH_TEST);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);

  glClearColor(0.5, 0.5, 0.8, 0.0);

  angX = 0.0;
  angY = 0.0;
  rotY = 170;
  rotX = 35;
}

