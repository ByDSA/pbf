#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include "tga.h"


void initCube();
void initPoints(int);
void drawPoints(int);

void loadSource(GLuint &shaderID, std::string name);
void printCompileInfoLog(GLuint shadID);
void printLinkInfoLog(GLuint programID);
void validateProgram(GLuint programID);

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

char* computeFilename[] = {"pbf_first.glsl", "pbf_updateGrid.glsl", "pbf_updateNeighbors.glsl", "pbf_calculateLambda.glsl", "pbf_calculateDelta.glsl", "pbf_update.glsl", "pbf_applyViscosity.glsl", "pbf_applyVorticity.glsl", "pbf_debug.glsl", "pbf_debug.glsl"};

 
float xrot = 24.0f;
float yrot = -44.2f;
float xdiff = 0.0f;
float ydiff = 0.0f;

int g_Width = 1280;                          // Ancho inicial de la ventana
int g_Height = 720;                         // Altura incial de la ventana

GLuint cubeVAOHandle, pointsVAOHandle;
GLuint graphicProgramID;
GLuint computeProgramID[COMPUTE_SHADERS];
GLuint locUniformMVPM, locUniformMVM, locUniformPM;
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

inline float ranf( float min = 0.0f, float max = 1.0f )
{
	return ((max - min) * rand() / RAND_MAX + min);
}

// BEGIN: Carga shaders ////////////////////////////////////////////////////////////////////////////////////////////

void loadSource(GLuint &shaderID, std::string name) 
{
	std::ifstream f(name.c_str());
	if (!f.is_open()) 
	{
		std::cerr << "File not found " << name.c_str() << std::endl;
		system("pause");
		exit(EXIT_FAILURE);
	}
	// now read in the data
	std::string *source;
	source = new std::string( std::istreambuf_iterator<char>(f),   
						std::istreambuf_iterator<char>() );
	f.close();
   
	// add a null to the string
	*source += "\0";
	const GLchar * data = source->c_str();
	glShaderSource(shaderID, 1, &data, NULL);
	delete source;
}

void printCompileInfoLog(GLuint shadID) 
{
GLint compiled;
	glGetShaderiv( shadID, GL_COMPILE_STATUS, &compiled );
	if (compiled == GL_FALSE)
	{
		GLint infoLength = 0;
		glGetShaderiv( shadID, GL_INFO_LOG_LENGTH, &infoLength );

		GLchar *infoLog = new GLchar[infoLength];
		GLint chsWritten = 0;
		glGetShaderInfoLog( shadID, infoLength, &chsWritten, infoLog );

		std::cerr << "Shader compiling failed:" << infoLog << std::endl;
		system("pause");
		delete [] infoLog;

		exit(EXIT_FAILURE);
	}
}

void printLinkInfoLog(GLuint programID)
{
GLint linked;
	glGetProgramiv( programID, GL_LINK_STATUS, &linked );
	if(! linked)
	{
		GLint infoLength = 0;
		glGetProgramiv( programID, GL_INFO_LOG_LENGTH, &infoLength );

		GLchar *infoLog = new GLchar[infoLength];
		GLint chsWritten = 0;
		glGetProgramInfoLog( programID, infoLength, &chsWritten, infoLog );

		std::cerr << "Shader linking failed:" << infoLog << std::endl;
		system("pause");
		delete [] infoLog;

		exit(EXIT_FAILURE);
	}
}

void validateProgram(GLuint programID)
{
GLint status;
    glValidateProgram( programID );
    glGetProgramiv( programID, GL_VALIDATE_STATUS, &status );

    if( status == GL_FALSE ) 
	{
		GLint infoLength = 0;
		glGetProgramiv( programID, GL_INFO_LOG_LENGTH, &infoLength );

        if( infoLength > 0 ) 
		{
			GLchar *infoLog = new GLchar[infoLength];
			GLint chsWritten = 0;
            glGetProgramInfoLog( programID, infoLength, &chsWritten, infoLog );
			std::cerr << "Program validating failed:" << infoLog << std::endl;
			system("pause");
            delete [] infoLog;

			exit(EXIT_FAILURE);
		}
    }
}

// END:   Carga shaders ////////////////////////////////////////////////////////////////////////////////////////////

// BEGIN: Inicializa primitivas ////////////////////////////////////////////////////////////////////////////////////

// TODO: no generar los buffers cada vez que se llama a la función
void initPoints() 
{
	float* position = new float[NUM_PARTICLES * 4];
	float* colors = new float[NUM_PARTICLES * 4];
	float* velocs = new float[NUM_PARTICLES * 4];

    float ox = (LIM_X_SUP - LIM_X_INF - side) / 2.0f;
    float oy = (LIM_Y_SUP - LIM_Y_INF - side) / 4.0f;
    float oz = (LIM_Z_SUP - LIM_Z_INF - side) / 2.0f;

    int n = 0;
    for(int i = 0; i < ppedge; i++)
        for(int j = 0; j < ppedge; j++)
            for(int k = 0; k < ppedge; k++) {

                position[4 * n + 0] = ox + i*inc; // x
		        position[4 * n + 1] = oy + j*inc; // y
		        position[4 * n + 2] = oz + k*inc; // z

                colors[4 * n + 0] = ((float)i) / ppedge; // r
		        colors[4 * n + 1] = ((float)j) / ppedge; // g
		        colors[4 * n + 2] = ((float)k) / ppedge; // b
		        colors[4 * n + 3] = 1.0f;

		        velocs[4 * n + 0] = 0; // x
		        velocs[4 * n + 1] = 0; // y
		        velocs[4 * n + 2] = 0; // z

                n++;
            }


enum {
    POS_SSBO, VEL_SSBO, COL_SSBO, POSPRED_SSBO, VORTICITY_SSBO, PRESSURE_SSBO, LAMBDA_SSBO, CELL_SSBO, GRIDCELLS_SSBO, NEIGHBORS_SSBO, SSBO_NUMBER
};

	GLuint ssbo[SSBO_NUMBER];

	glGenBuffers( SSBO_NUMBER, ssbo);

	// Tarea por hacer: Crear SSBO en lugar de VBO
	// Tarea por hacer: Activarlos para ser indexados dentro del compute shader
	// Tarea por hacer: Dentro del VAO, utilizar los SSBO como VBO (no será necesario volver a pasarle los datos)
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo[POS_SSBO] );
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(float)*4, position, GL_STATIC_DRAW );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo[VEL_SSBO] );
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(float)*4, velocs, GL_STATIC_DRAW );
	glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo[COL_SSBO] );
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(float)*4, colors, GL_STATIC_DRAW );

	glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo[POSPRED_SSBO] );
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(float)*4, NULL, GL_STATIC_DRAW );

	glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo[VORTICITY_SSBO] );
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(float)*4, NULL, GL_STATIC_DRAW );

	glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo[PRESSURE_SSBO] );
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(float),	NULL, GL_STATIC_DRAW );

	glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo[LAMBDA_SSBO] );
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(float)*4,	NULL, GL_STATIC_DRAW );

	glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo[CELL_SSBO] );
	glBufferData( GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(unsigned)*4, NULL, GL_STATIC_DRAW );

	glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo[NEIGHBORS_SSBO] );
	glBufferData( GL_SHADER_STORAGE_BUFFER, (MAX_PART_NEIGHBORS+1) * NUM_PARTICLES * sizeof(unsigned)*4, NULL, GL_STATIC_DRAW );

	glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo[GRIDCELLS_SSBO] );
	glBufferData( GL_SHADER_STORAGE_BUFFER, CELL_NUMBER * (MAX_PART_PER_CELL + 1) * sizeof(unsigned)*4, NULL, GL_STATIC_DRAW );



    for(int i = 0; i < SSBO_NUMBER; i++) {
		//std::cout << i << std::endl;
	    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, i, ssbo[i] );
	}


	glGenVertexArrays (1, &pointsVAOHandle);
	glBindVertexArray (pointsVAOHandle);

	glBindBuffer(GL_ARRAY_BUFFER, ssbo[POS_SSBO]);    
	GLuint loc = glGetAttribLocation(graphicProgramID, "aPosition");   
	glEnableVertexAttribArray(loc); 
	glVertexAttribPointer( loc, 4, GL_FLOAT, GL_FALSE, 0, (char *)NULL + 0 ); 

	glBindBuffer(GL_ARRAY_BUFFER, ssbo[COL_SSBO]);    
	GLuint loc2 = glGetAttribLocation(graphicProgramID, "aColor"); 
	glEnableVertexAttribArray(loc2); 
	glVertexAttribPointer( loc2, 4, GL_FLOAT, GL_FALSE, 0, (char *)NULL + 0 );

	glBindVertexArray (0);

	delete []position;
	delete []colors;
	delete []velocs;
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
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(g_Width, g_Height);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("Programa Ejemplo");
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
	srand (time(NULL));

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
 
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearDepth(1.0f);

	glShadeModel(GL_SMOOTH);

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Tarea por hacer: Crear el objeto programa para el compute shader
	for(int i = 0; i < COMPUTE_SHADERS; i++) {
		computeProgramID[i] = glCreateProgram();

		GLuint computeShaderID = glCreateShader(GL_COMPUTE_SHADER);
		loadSource(computeShaderID, computeFilename[i]);
		std::cout << "Compiling Compute Shader " << computeFilename[i] << std::endl;
		glCompileShader(computeShaderID);
		printCompileInfoLog(computeShaderID);
		glAttachShader(computeProgramID[i], computeShaderID);

		glLinkProgram(computeProgramID[i]);
		printLinkInfoLog(computeProgramID[i]);
		validateProgram(computeProgramID[i]);
	}
	
	// Graphic shaders program
	graphicProgramID = glCreateProgram();

	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	loadSource(vertexShaderID, "pbf.vert");
	std::cout << "Compiling Vertex Shader" << std::endl;
	glCompileShader(vertexShaderID);
	printCompileInfoLog(vertexShaderID);
	glAttachShader(graphicProgramID, vertexShaderID);

	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	loadSource(fragmentShaderID, "pbf.frag");
	std::cout << "Compiling Fragment Shader" << std::endl;
	glCompileShader(fragmentShaderID);
	printCompileInfoLog(fragmentShaderID);
	glAttachShader(graphicProgramID, fragmentShaderID);

	GLuint geometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
	loadSource(geometryShaderID, "oriented_sprite.geom");
	std::cout << "Compiling Geometry Shader" << std::endl;
	glCompileShader(geometryShaderID);
	printCompileInfoLog(geometryShaderID);
	glAttachShader(graphicProgramID, geometryShaderID);

	// Tarea por hacer: Realizar la misma tarea para el geometry shader (que para el vertex y el fragment shader)

	glLinkProgram(graphicProgramID);
	printLinkInfoLog(graphicProgramID);
	validateProgram(graphicProgramID);


	TGAFILE tgaImage;
	GLuint textId;
	glGenTextures (1, &textId);
	if ( LoadTGAFile("white_sphere.tga", &tgaImage) )
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tgaImage.imageWidth, tgaImage.imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, tgaImage.imageData);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	initPoints();
	locUniformMVPM = glGetUniformLocation(graphicProgramID, "uModelViewProjMatrix");
	locUniformMVM = glGetUniformLocation(graphicProgramID, "uModelViewMatrix");
	locUniformPM = glGetUniformLocation(graphicProgramID, "uProjectionMatrix");
	locUniformSpriteTex = glGetUniformLocation(graphicProgramID, "uSpriteTex");
	return true;
}

void atomicProgram(GLuint p) {
    glUseProgram(p);
			
    glDispatchCompute( NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1 );

	glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
}
 
void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 Projection = glm::perspective(45.0f, 1.0f * g_Width / g_Height, 1.0f, 100.0f);
	
	glm::vec3 cameraPos = glm::vec3( -13.93*cos( xrot ), -13.93 * sin(yrot), -13.93 );
	glm::mat4 View = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 ModelCube = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f)), glm::vec3(0.0f, 0.0f, 0.0f));

	glm::mat4 mvp; // Model-view-projection matrix
	glm::mat4 mv;  // Model-view matrix

    if (animation) {
        atomicProgram(computeProgramID[PBF_FIRST]);
        atomicProgram(computeProgramID[PBF_UPDATEGRID]);
        atomicProgram(computeProgramID[PBF_UPDATENEIGHBORS]);

        for(int i = 0; i < ITERATIONS; i++) {
            atomicProgram(computeProgramID[PBF_CALCULATELAMBDA]);
            atomicProgram(computeProgramID[PBF_CALCULATEDELTA]);
        }

        atomicProgram(computeProgramID[PBF_UPDATE]);
        atomicProgram(computeProgramID[PBF_APPLYVISOSITY]);
        atomicProgram(computeProgramID[PBF_APPLYVORTICITY]);
        atomicProgram(computeProgramID[PBF_DEBUG]);
    }

	glUseProgram(graphicProgramID);

	// Dibuja Puntos
	mvp = Projection * View * ModelCube;
	mv = View * ModelCube;
	glUniformMatrix4fv( locUniformMVPM, 1, GL_FALSE, &mvp[0][0] );
	glUniformMatrix4fv( locUniformMVM, 1, GL_FALSE, &mv[0][0] );
	glUniformMatrix4fv( locUniformPM, 1, GL_FALSE, &Projection[0][0] );

	glUniform1i (locUniformSpriteTex, 0);

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
		break;
    case ' ':
        initPoints();
	}
}
 
void specialKeyboard(int key, int x, int y)
{
	if (key == GLUT_KEY_F1)
	{
		fullscreen = !fullscreen;
 
		if (fullscreen)
			glutFullScreen();
		else
		{
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
 
		xdiff = x - yrot/100;
		ydiff = -y + xrot/100;
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
