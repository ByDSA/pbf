#include <iostream>
#include <fstream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include "tga.h"

#include "ShaderProgram.h"

#include <vector>

#include "BufferObject.h"


void initPoints(int);
void drawPoints(int);

bool init();
void display();
void resize(int, int);
void idle();
void keyboard(unsigned char, int, int);
void specialKeyboard(int, int, int);
void mouse(int, int, int, int);
void mouseMotion(int, int);


bool fullscreen = false;
bool mouseDown = false;
bool animation = false;
int lighting = 0; 


enum {PBF_FIRST, PBF_UPDATEGRID, PBF_UPDATENEIGHBORS, PBF_CALCULATELAMBDA, PBF_CALCULATEDELTA, PBF_UPDATE, PBF_APPLYVISOSITY, PBF_APPLYVORTICITY, PBF_DEBUG, COMPUTE_SHADERS};

char* computeFilename[] = {"shaders/compute/pbf_first.glsl",
	"shaders/compute/pbf_updateGrid.glsl",
	"shaders/compute/pbf_updateNeighbors.glsl",
	"shaders/compute/pbf_calculateLambda.glsl",
	"shaders/compute/pbf_calculateDelta.glsl",
	"shaders/compute/pbf_update.glsl",
	"shaders/compute/pbf_applyViscosity.glsl",
	"shaders/compute/pbf_applyVorticity.glsl",
	"shaders/compute/pbf_debug.glsl",
	"shaders/compute/pbf_debug.glsl"};

enum {
    POS_SSBO, VEL_SSBO, COL_SSBO, POSPRED_SSBO, VORTICITY_SSBO, LAMBDA_SSBO, CELL_SSBO, GRIDCELLS_SSBO, NEIGHBORS_SSBO, SSBO_NUMBER
};

float xrot = 0.0f;
float yrot = 0.0f;
float xdiff = 0.0f;
float ydiff = 0.0f;

int g_Width = 1280;                          // Ancho inicial de la ventana
int g_Height = 720;                         // Altura incial de la ventana

GLuint cubeVAOHandle, pointsVAOHandle;
ShaderProgram* graphicProgram;
ShaderProgram* computeProgram[COMPUTE_SHADERS];
BufferObject<GL_SHADER_STORAGE_BUFFER>* ssbo[SSBO_NUMBER];
GLuint locUniformSpriteTex;

const unsigned ITERATIONS = 4;

float dt                 = 0.01;
const float density_rest = 998.29f;
const int ppedge    = 30;
const float kernel_part  = 20;
const float UMBRAL       = 1e-5f;
const float CRF          = 0;

const float side         = 4;
const float volume       = side*side*side;
const float inc          = side / ppedge;
const int numPart        = ppedge*ppedge*ppedge;
const float mass         = density_rest * volume / numPart;

// Radio kernel
const int WORK_GROUP_SIZE = 100;

const int NUM_PARTICLES = ppedge*ppedge*ppedge;

const float h  = pow( (3*volume*kernel_part) / ( 4 * glm::pi<float>() * NUM_PARTICLES ) , 1.0f/3);
const float h2 = h * h;
const float h3 = h2 * h;
const float h6 = h2 * h2 * h2;
const float h9 = h3 * h3 * h3;

// Recipiente
const float LIM_X_INF = 0;
const float LIM_X_SUP = 6;
const float LIM_Y_INF = 0;
const float LIM_Y_SUP = 6;
const float LIM_Z_INF = 0;
const float LIM_Z_SUP = 6;

// Cantidad de celdas
const unsigned CELL_WIDTH  = (unsigned)( (LIM_X_SUP - LIM_X_INF) / h ) + 1;
const unsigned CELL_HEIGHT = (unsigned)( (LIM_Y_SUP - LIM_Y_INF) / h ) + 1;
const unsigned CELL_LARGE  = (unsigned)( (LIM_Z_SUP - LIM_Z_INF) / h ) + 1;
const unsigned CELL_NUMBER = CELL_WIDTH * CELL_HEIGHT * CELL_LARGE;

const int MAX_PART_PER_CELL = 20;                      // Máximo número de partículas por celda
const int CELL_NEIGHBORS     = 27;                     // Celdas vecinas (incluyendo propia)
const int MAX_PART_NEIGHBORS = CELL_NEIGHBORS*MAX_PART_PER_CELL; // Máximo número de vecinos por celda

// BEGIN: Inicializa primitivas ////////////////////////////////////////////////////////////////////////////////////

// TODO: no generar los buffers cada vez que se llama a la función
void initPoints() {
	std::vector<glm::vec4> position(NUM_PARTICLES);
	std::vector<glm::vec4> colors(NUM_PARTICLES);
	std::vector<glm::vec4> velocs(NUM_PARTICLES);

    float ox = (LIM_X_SUP - LIM_X_INF - side) / 2.0f;
    float oy = (LIM_Y_SUP - LIM_Y_INF - side) / 2.0f;
    float oz = (LIM_Z_SUP - LIM_Z_INF - side) / 2.0f;

    int n = 0;
    for(int i = 0; i < ppedge; i++)
        for(int j = 0; j < ppedge; j++)
            for(int k = 0; k < ppedge; k++) {
                position[n].x = ox + i*inc;
		        position[n].y = oy + j*inc;
		        position[n].z = oz + k*inc;

                colors[n].r = i / (GLfloat)ppedge;
		        colors[n].g = j / (GLfloat)ppedge;
		        colors[n].b = k / (GLfloat)ppedge;
		        colors[n].a = 1.0f;

		        velocs[n].x = 0;
		        velocs[n].y = 0;
		        velocs[n].z = 0;

                n++;
            }
	
	// Transferir/reservar memoria
	ssbo[POS_SSBO]      ->transfer(position, GL_STATIC_DRAW);
	ssbo[VEL_SSBO]      ->transfer(velocs, GL_STATIC_DRAW);
	ssbo[COL_SSBO]      ->transfer(colors, GL_STATIC_DRAW);
	ssbo[POSPRED_SSBO]  ->allocate(NUM_PARTICLES * sizeof(GLfloat)*4, GL_STATIC_DRAW);
	ssbo[VORTICITY_SSBO]->allocate(NUM_PARTICLES * sizeof(GLfloat)*4, GL_STATIC_DRAW);
	ssbo[LAMBDA_SSBO]   ->allocate(NUM_PARTICLES * sizeof(GLfloat)*4, GL_STATIC_DRAW);
	ssbo[CELL_SSBO]     ->allocate(NUM_PARTICLES * sizeof(unsigned), GL_STATIC_DRAW);
	ssbo[NEIGHBORS_SSBO]->allocate((MAX_PART_NEIGHBORS+1) * NUM_PARTICLES * sizeof(unsigned), GL_STATIC_DRAW);
	ssbo[GRIDCELLS_SSBO]->allocate(CELL_NUMBER * (MAX_PART_PER_CELL + 1) * sizeof(unsigned), GL_STATIC_DRAW );

	glGenVertexArrays (1, &pointsVAOHandle);
	glBindVertexArray (pointsVAOHandle);

	graphicProgram->vertexAttribPointer(ssbo[POS_SSBO], "aPosition", 4, GL_FLOAT, GL_FALSE, 0, (char *)NULL + 0);
	graphicProgram->vertexAttribPointer(ssbo[COL_SSBO], "aColor", 4, GL_FLOAT, GL_FALSE, 0, (char *)NULL + 0);

	glBindVertexArray (0);
}

// END: Inicializa primitivas ////////////////////////////////////////////////////////////////////////////////////

// BEGIN: Funciones de dibujo ////////////////////////////////////////////////////////////////////////////////////

void drawPoints(int numPoints) {
	glBindVertexArray(pointsVAOHandle);
    glDrawArrays(GL_POINTS, 0, numPoints);
	glBindVertexArray(0);
}

// END: Funciones de dibujo ////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	glutInit(&argc, argv); 
	glutInitWindowPosition(240, 480);
	glutInitWindowSize(g_Width, g_Height);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("Position Based Fluids");
	GLenum err = glewInit();
	if (GLEW_OK != err) {
	  /* Problem: glewInit failed, something is seriously wrong. */
	  std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
	  system("pause");
	  exit(-1);
	}
	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKeyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMotion);
	glutReshapeFunc(resize);
	glutIdleFunc(idle);
 
	glutMainLoop();
 
	return EXIT_SUCCESS;
}

bool init() {
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
 
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearDepth(1.0f);

	glShadeModel(GL_SMOOTH);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Compute shader programs
	for(int i = 0; i < COMPUTE_SHADERS; i++) {
		computeProgram[i] = new ShaderProgram();
		computeProgram[i]->addShader(GL_COMPUTE_SHADER, computeFilename[i]);
		computeProgram[i]->compile();

		computeProgram[i]->setDispatch(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);
	}
	
	// Graphic shaders program
	graphicProgram = new ShaderProgram();
	graphicProgram->addShader(GL_VERTEX_SHADER, "shaders/pbf.vert");
	graphicProgram->addShader(GL_FRAGMENT_SHADER, "shaders/pbf.frag");
	graphicProgram->addShader(GL_GEOMETRY_SHADER, "shaders/oriented_sprite.geom");
	graphicProgram->compile();

	// Sprite points
	TGAFILE tgaImage;
	GLuint textId;
	glGenTextures (1, &textId);
	if ( LoadTGAFile("textures/white_sphere.tga", &tgaImage) ) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tgaImage.imageWidth, tgaImage.imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, tgaImage.imageData);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// Crear buffers
	for(int i = 0; i < SSBO_NUMBER; i++) {
		ssbo[i] = new BufferObject<GL_SHADER_STORAGE_BUFFER>();
	}

	initPoints();
	locUniformSpriteTex = glGetUniformLocation(graphicProgram->id(), "uSpriteTex");
	return true;
}
 
void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 Projection = glm::perspective(45.0f, 1.0f * g_Width / g_Height, 1.0f, 100.0f);

	glm::vec3 cameraPos = glm::vec3(15) * glm::vec3( cos( yrot / 100 ), sin(xrot / 100), sin( yrot / 100 ) * cos(xrot /100)) + glm::vec3(6.0f);
	glm::mat4 View = glm::lookAt(cameraPos, glm::vec3(6.0f, 6.0f, 6.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 ModelCube = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f)), glm::vec3(0.0f, 0.0f, 0.0f));

	glm::mat4 mvp; // Model-view-projection matrix
	glm::mat4 mv;  // Model-view matrix

    if (animation) {
		ShaderProgram::useAtomic(computeProgram[PBF_FIRST]);
        ShaderProgram::useAtomic(computeProgram[PBF_UPDATEGRID]);
        ShaderProgram::useAtomic(computeProgram[PBF_UPDATENEIGHBORS]);

        for(int i = 0; i < ITERATIONS; i++) {
            ShaderProgram::useAtomic(computeProgram[PBF_CALCULATELAMBDA]);
            ShaderProgram::useAtomic(computeProgram[PBF_CALCULATEDELTA]);
        }

        ShaderProgram::useAtomic(computeProgram[PBF_UPDATE]);
        ShaderProgram::useAtomic(computeProgram[PBF_APPLYVISOSITY]);
        ShaderProgram::useAtomic(computeProgram[PBF_APPLYVORTICITY]);

        ShaderProgram::useAtomic(computeProgram[PBF_DEBUG]);
    }

	ShaderProgram::use(graphicProgram);

	// Dibuja Puntos
	mvp = Projection * View * ModelCube;
	mv = View * ModelCube;
	graphicProgram->uniformMatrix4fv( "uModelViewProjMatrix", 1, GL_FALSE, &mvp[0][0] );
	graphicProgram->uniformMatrix4fv( "uModelViewMatrix", 1, GL_FALSE, &mv[0][0] );
	graphicProgram->uniformMatrix4fv( "uProjectionMatrix", 1, GL_FALSE, &Projection[0][0] );

	glUniform1i(locUniformSpriteTex, 0);

	drawPoints(NUM_PARTICLES);

	glUseProgram(0);

	glutSwapBuffers();
}
 
void resize(int w, int h)
{
	g_Width = w;
	g_Height = h;
	glViewport(0, 0, g_Width, g_Height);
}
 
void idle()
{
	/*if (!mouseDown && animation)
	{
		xrot += 0.3f;
		yrot += 0.4f;
	}*/
	glutPostRedisplay();
}
 
void keyboard(unsigned char key, int x, int y)
{
	static bool wireframe = false;
	switch(key)
	{
	case 27 : case 'q': case 'Q':
		exit(1); 
		break;
	case 'a': case 'A':
		animation = !animation;
		std::cout << (animation ? "Animación activada" : "Animación desactivada");
		break;
    case ' ':
        initPoints();
	}
}
 
void specialKeyboard(int key, int x, int y) {
	if (key == GLUT_KEY_F4) {
		fullscreen = !fullscreen;
 
		if (fullscreen)
			glutFullScreen();
		else {
			glutReshapeWindow(g_Width, g_Height);
			glutPositionWindow(50, 50);
		}
	}
}
 
void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		mouseDown = true;
 
		xdiff = x - yrot;
		ydiff = -y + xrot;
	}
	else
		mouseDown = false;
}
 
void mouseMotion(int x, int y)
{
	if (mouseDown)
	{
		yrot = x - xdiff;
		xrot = y + ydiff;
 
		glutPostRedisplay();
	}
}
