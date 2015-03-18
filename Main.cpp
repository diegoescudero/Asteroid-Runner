/*
 *  CPE 474 lab 0 - modern graphics test bed
 *  draws a partial cube using a VBO and IBO 
 *  glut/OpenGL/GLSL application   
 *  Uses glm and local matrix stack
 *  to handle matrix transforms for a view matrix, projection matrix and
 *  model transform matrix
 *
 *  zwood 9/12 
 *  Copyright 2012 Cal Poly. All rights reserved.
 *
 *****************************************************************************/

#ifdef __APPLE__
#include "GLUT/glut.h"
#include <OPENGL/gl.h>
#endif

#ifdef __unix__
#include <GL/glut.h>
#endif

#ifdef _WIN32
#include <GL/glew.h>
#include <GL/glut.h>

#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "freeglut.lib")
#endif

#include <iostream>
#include <fstream>
#include <ios>
#include <string>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#define PI 3.14159

#include "GLSL_helper.h"
#include "MStackHelp.h"

#include "mesh.h"

using namespace std;
using namespace glm;

// Keys
int keys[256];
float highestSpeed = 1000.0;

// Asteroid Variables
int const asteroidRows = 10;
int const asteroidDensity = 2;
float asteroidSpeed = 0.1;
int rowPassedCount = 0;
int hitTimeCount = 0;
vec3 asteroidPos[asteroidRows][asteroidDensity];
vec3 asteroidAxis[asteroidRows][asteroidDensity];
float const asteroidRadius = 0.5;

// Tunnel Object
Mesh *tunnel;
GLuint tunnelVBO, tunnelIBO, tunnelNBO, tunnelUBO;
int tunnelVertCount;
vec3 tunnelcolor;

// Ship Object
Mesh *ship;
GLuint shipVBO, shipIBO, shipNBO, shipUBO;
int shipVertCount;
float horizontal = 0.0;
float vertical = 0.0;
int hit = 0;
float const moveSpeed = 0.15;
float const moveAngle = 35.0;
vec3 aliveColor = vec3(0.15, 0.57, 0.13);
vec3 hurtColor = vec3(0.57, 0.13, 0.14);

// Ship Collision
vec3 shipCollision[8];

// Asteroid Object
Mesh *asteroid;
GLuint asteroidVBO, asteroidIBO, asteroidNBO, asteroidUBO;
int asteroidVertCount;

// Program Variables
float Accumulator;

// Parameters
unsigned int const StepSize = 10;
unsigned int WindowWidth = 1600, WindowHeight = 900;

// View
vec3 eye(0.0, 0.5, 2.5);
vec3 center(0.0, 0.0, 1.0);
vec3 up(0.0, 1.0, 0.0);
float alphaDeg = 0.0;
float betaDeg = 0.0;

// Variable Handles
GLuint aPosition;
GLuint aNormal;
GLuint uModelMatrix;
GLuint uNormalMatrix;
GLuint uViewMatrix;
GLuint uProjMatrix;
GLuint uColor;

// Shader Handle
GLuint ShadeProg;

RenderingHelper ModelTrans;

void SetProjectionMatrix() {
	glm::mat4 Projection = glm::perspective(80.0f,
			((float) WindowWidth) / ((float) WindowHeight), 0.1f, 100.f);
	safe_glUniformMatrix4fv(uProjMatrix, glm::value_ptr(Projection));
}

void SetView() {
	glm::mat4 View = glm::lookAt(eye, center, up);
	safe_glUniformMatrix4fv(uViewMatrix, glm::value_ptr(View));
}

void SetModel() {
	safe_glUniformMatrix4fv(uModelMatrix,
			glm::value_ptr(ModelTrans.modelViewMatrix));
	safe_glUniformMatrix4fv(uNormalMatrix,
			glm::value_ptr(
					glm::transpose(glm::inverse(ModelTrans.modelViewMatrix))));
}

bool InstallShader(std::string const & vShaderName,
		std::string const & fShaderName) {
	GLuint VS; // handles to shader object
	GLuint FS; // handles to frag shader object
	GLint vCompiled, fCompiled, linked; // status of shader

	VS = glCreateShader(GL_VERTEX_SHADER);
	FS = glCreateShader(GL_FRAGMENT_SHADER);

	// load the source
	char const * vSource = textFileRead(vShaderName);
	char const * fSource = textFileRead(fShaderName);
	glShaderSource(VS, 1, &vSource, NULL);
	glShaderSource(FS, 1, &fSource, NULL);

	// compile shader and print log
	glCompileShader(VS);
	printOpenGLError();
	glGetShaderiv(VS, GL_COMPILE_STATUS, &vCompiled);
	printShaderInfoLog(VS);

	// compile shader and print log
	glCompileShader(FS);
	printOpenGLError();
	glGetShaderiv(FS, GL_COMPILE_STATUS, &fCompiled);
	printShaderInfoLog(FS);

	if (!vCompiled || !fCompiled) {
		std::cerr << "Error compiling either shader " << vShaderName << " or "
				<< fShaderName << std::endl;
		return false;
	}

	// create a program object and attach the compiled shader
	ShadeProg = glCreateProgram();
	glAttachShader(ShadeProg, VS);
	glAttachShader(ShadeProg, FS);

	glLinkProgram(ShadeProg);

	// check shader status requires helper functions
	printOpenGLError();
	glGetProgramiv(ShadeProg, GL_LINK_STATUS, &linked);
	printProgramInfoLog(ShadeProg);

	glUseProgram(ShadeProg);

	// get handles to attribute data
	aPosition = safe_glGetAttribLocation(ShadeProg, "aPosition");
	aNormal = safe_glGetAttribLocation(ShadeProg, "aNormal");

	uColor = safe_glGetUniformLocation(ShadeProg, "uColor");
	uProjMatrix = safe_glGetUniformLocation(ShadeProg, "uProjMatrix");
	uViewMatrix = safe_glGetUniformLocation(ShadeProg, "uViewMatrix");
	uModelMatrix = safe_glGetUniformLocation(ShadeProg, "uModelMatrix");
	uNormalMatrix = safe_glGetUniformLocation(ShadeProg, "uNormalMatrix");

	//std::cout << "Sucessfully installed shader " << ShadeProg << std::endl;
	return true;
}

void loadTunnel() {
	std::ifstream modelFile("Models/tunnel.obj");
	Model model;
	model.load(modelFile);
	tunnelVertCount = model.meshes()[0].makeVBO(&tunnelIBO, &tunnelVBO, &tunnelUBO, &tunnelNBO);
}

void loadShip() {
	std::ifstream modelFile("Models/ship.obj");
	Model model;
	model.load(modelFile);
	shipVertCount = model.meshes()[0].makeVBO(&shipIBO, &shipVBO, &shipUBO, &shipNBO);
}

void loadAsteroid() {
	std::ifstream modelFile("Models/asteroid.obj");
	Model model;
	model.load(modelFile);
	asteroidVertCount = model.meshes()[0].makeVBO(&asteroidIBO, &asteroidVBO, &asteroidUBO, &asteroidNBO);
}

void Initialize() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	ModelTrans.useModelViewMatrix();
	ModelTrans.loadIdentity();
}

void setShipCollisions() {
	shipCollision[0] = vec3(0.75, 0.5, 0.15);
	shipCollision[1] = vec3(0.75, 0.5, -0.15);
	shipCollision[2] = vec3(0.75, -0.5, 0.15);
	shipCollision[3] = vec3(0.75, -0.5, -0.15);
	shipCollision[4] = vec3(-0.75, 0.5, 0.15);
	shipCollision[5] = vec3(-0.75, 0.5, -0.15);
	shipCollision[6] = vec3(-0.75, -0.5, 0.15);
	shipCollision[7] = vec3(-0.75, -0.5, -0.15);
}

void initAsteroidRows() {
	for (int i = 0; i < asteroidRows; i++) {
		for (int j = 0; j < asteroidDensity; j++) {
			// Positions
			asteroidPos[i][j].x = (float)(rand() % 100) * (0.045);
			asteroidPos[i][j].y = (float)(rand() % 100) * (0.045);
			asteroidPos[i][j].z = (float)-i*2.0 - 10.0;

			if ((rand() % 2) == 0) {
				asteroidPos[i][j].x *= -1;
			}
			if ((rand() % 2) == 0) {
				asteroidPos[i][j].y *= -1;
			}

			// Axis
			asteroidAxis[i][j].x = (float)(rand() % 100);
			asteroidAxis[i][j].y = (float)(rand() % 100);
			asteroidAxis[i][j].z = (float)(rand() % 100);

			if ((rand() % 2) == 0) {
				asteroidAxis[i][j].x *= -1;
			}
			if ((rand() % 2) == 0) {
				asteroidAxis[i][j].y *= -1;
			}
			if ((rand() % 2) == 0) {
				asteroidAxis[i][j].z *= -1;
			}
		}
	}
}

void resetAsteroidRow(int i) {
	for (int j = 0; j < asteroidDensity; j++){
		// Positions
		asteroidPos[i][j].x = (float)(rand() % 100) * (0.045);
		asteroidPos[i][j].y = (float)(rand() % 100) * (0.045);
		asteroidPos[i][j].z = -20.0;

		if ((rand() % 2) == 0) {
			asteroidPos[i][j].x *= -1;
		}
		if ((rand() % 2) == 0) {
			asteroidPos[i][j].y *= -1;
		}

		// Axis
		asteroidAxis[i][j].x = (float)(rand() % 100);
		asteroidAxis[i][j].y = (float)(rand() % 100);
		asteroidAxis[i][j].z = (float)(rand() % 100);

		if ((rand() % 2) == 0) {
			asteroidAxis[i][j].x *= -1;
		}
		if ((rand() % 2) == 0) {
			asteroidAxis[i][j].y *= -1;
		}
		if ((rand() % 2) == 0) {
			asteroidAxis[i][j].z *= -1;
		}
	}

	rowPassedCount++;
	hitTimeCount++;
	if ((rowPassedCount % 10) == 0) {
		asteroidSpeed += 0.01;
		rowPassedCount = 0;
		tunnelcolor.x = (float)(rand() % 100) * 0.01;
		tunnelcolor.y = (float)(rand() % 100) * 0.01;
		tunnelcolor.z = (float)(rand() % 100) * 0.01;
	}
	if ((rowPassedCount % 1) == 0) {
		hitTimeCount = 0;
		hit = 0;
	}
	// Update highest speed
	if ((asteroidSpeed*10000.0) > highestSpeed) {
		highestSpeed = asteroidSpeed*10000.0;
	}
}

void detectCollision(int i) {
	int collisionCount = 0;

	for (int j = 0; j < asteroidDensity; j++) {
		for (int k = 0; k < 8; k++) {
			// Collision
			if (dot(asteroidPos[i][j]-shipCollision[k], asteroidPos[i][j]-shipCollision[k]) < asteroidRadius) {
				hit = 1;
				collisionCount++;
				asteroidSpeed = 0.1;
			}
		}
	}
}

void updatePositions() {
	for (int i = 0; i < asteroidRows; i++) {
		for (int j = 0; j < asteroidDensity; j++) {
			asteroidPos[i][j].z += asteroidSpeed;
		}

		// Reset Row
		if (asteroidPos[i][0].z > 1.5) {
			resetAsteroidRow(i);
		}
		// Check for collisions
		if (asteroidPos[i][0].z > -1.0 && asteroidPos[i][0].z < 1.0) {
			detectCollision(i);
		}
	}
}

void DrawTunnel() {
	SetModel();

	// COLOR
	//glUniform3f(uColor, 0.15f, 0.15f, 0.58f);
	glUniform3f(uColor, tunnelcolor.x, tunnelcolor.y, tunnelcolor.z);

	// VBO
	safe_glEnableVertexAttribArray(aPosition);
	glBindBuffer(GL_ARRAY_BUFFER, tunnelVBO);
	safe_glVertexAttribPointer(aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// NBO
	safe_glEnableVertexAttribArray(aNormal);
	glBindBuffer(GL_ARRAY_BUFFER, tunnelNBO);
	safe_glVertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tunnelIBO);

	// DRAW
	glDrawElements(GL_TRIANGLES, tunnelVertCount, GL_UNSIGNED_INT, 0);

	safe_glDisableVertexAttribArray(aPosition);
	safe_glDisableVertexAttribArray(aNormal);
}

void DrawShip() {
	SetModel();

	// COLOR
	if (!hit) {
		glUniform3f(uColor, aliveColor.x, aliveColor.y, aliveColor.z);
	}
	else {
		glUniform3f(uColor, hurtColor.x, hurtColor.y, hurtColor.z);
	}

	// VBO
	safe_glEnableVertexAttribArray(aPosition);
	glBindBuffer(GL_ARRAY_BUFFER, shipVBO);
	safe_glVertexAttribPointer(aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// NBO
	safe_glEnableVertexAttribArray(aNormal);
	glBindBuffer(GL_ARRAY_BUFFER, shipNBO);
	safe_glVertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shipIBO);

	// DRAW
	glDrawElements(GL_TRIANGLES, shipVertCount, GL_UNSIGNED_INT, 0);

	safe_glDisableVertexAttribArray(aPosition);
	safe_glDisableVertexAttribArray(aNormal);
}

void DrawAsteroid() {
	SetModel();

	// COLOR
	//glUniform3f(uColor, 0.62f, 0.44f, 0.23f);
	glUniform3f(uColor, 0.33f, 0.23f, 0.043f);

	// VBO
	safe_glEnableVertexAttribArray(aPosition);
	glBindBuffer(GL_ARRAY_BUFFER, asteroidVBO);
	safe_glVertexAttribPointer(aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// NBO
	safe_glEnableVertexAttribArray(aNormal);
	glBindBuffer(GL_ARRAY_BUFFER, asteroidNBO);
	safe_glVertexAttribPointer(aNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, asteroidIBO);

	// DRAW
	glDrawElements(GL_TRIANGLES, asteroidVertCount, GL_UNSIGNED_INT, 0);

	safe_glDisableVertexAttribArray(aPosition);
	safe_glDisableVertexAttribArray(aNormal);
}

void Draw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(ShadeProg);

	SetProjectionMatrix();
	SetView();

	// Tunnel
	ModelTrans.loadIdentity();
	ModelTrans.pushMatrix();
	ModelTrans.scale(2.0); //makes size 10x10 (original 5x5)
	ModelTrans.rotate(90, up);
	DrawTunnel();
	ModelTrans.popMatrix();

	// Ship
	ModelTrans.loadIdentity();
	ModelTrans.pushMatrix();
	ModelTrans.translate(vec3(horizontal, vertical, 0.0));
	ModelTrans.rotate(alphaDeg, vec3(0.0, 1.0, 0.0));
	ModelTrans.rotate(betaDeg, vec3(1.0, 0.0, 0.0));
	DrawShip();
	ModelTrans.popMatrix();

	// Asteroids
	for (int i = 0; i < asteroidRows; i++) {
		for (int j = 0; j < asteroidDensity; j++){
			ModelTrans.loadIdentity();
			ModelTrans.pushMatrix();
			ModelTrans.translate(asteroidPos[i][j]);
			ModelTrans.rotate(Accumulator, asteroidAxis[i][j]);
			ModelTrans.scale(2.0);
			DrawAsteroid();
			ModelTrans.popMatrix();
		}
	}

	glUseProgram(0);
	glutSwapBuffers();
	glutPostRedisplay();
	printOpenGLError();
}

void Reshape(int width, int height) {
	WindowWidth = width;
	WindowHeight = height;
	glViewport(0, 0, width, height);
}

void Keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'w':
		keys['w'] = 1;
		betaDeg = moveAngle;
		break;
	case 's':
		keys['s'] = 1;
		betaDeg = -moveAngle;
		break;
	case 'a':
		keys['a'] = 1;
		alphaDeg = moveAngle;
		break;
	case 'd':
		keys['d'] = 1;
		alphaDeg = -moveAngle;
		break;

		// Quit program
	case 'q':
	case 'Q':
		printf("The highest speed you reached was %.0f km/h!!\n", highestSpeed);
		exit(EXIT_SUCCESS);
		break;

	}
}

void KeyboardUp(unsigned char key, int x, int y) {
	switch (key) {
	case 'w':
		keys['w'] = 0;
		betaDeg = 0.0;
		break;
	case 's':
		keys['s'] = 0;
		betaDeg = 0.0;
		break;
	case 'a':
		keys['a'] = 0;
		alphaDeg = 0.0;
		break;
	case 'd':
		keys['d'] = 0;
		alphaDeg = 0.0;
		break;
	}
}

float windowx(int num) {
	float ret = (float) num;

	ret -= (float) WindowWidth / 2;
	ret /= (float) WindowWidth / 2;

	return ret;
}

float windowy(int num) {
	float ret = (float) num;

	ret = (float) WindowHeight - (float) num - 1.0;
	ret -= (float) WindowHeight / 2;
	ret /= (float) WindowHeight / 2;

	return ret;
}

void aPressed() {
	if (horizontal > -4.0) {
		horizontal -= moveSpeed;
		eye.x -= moveSpeed;
		center.x -= moveSpeed;

		for (int i = 0; i < 8; i++) {
			shipCollision[i].x -= moveSpeed;
		}
	}
}

void dPressed() {
	if (horizontal < 4.0) {
		horizontal += moveSpeed;
		eye.x += moveSpeed;
		center.x += moveSpeed;

		for (int i = 0; i < 8; i++) {
			shipCollision[i].x += moveSpeed;
		}
	}
}

void wPressed() {
	if (vertical < 4.3) {
		vertical += moveSpeed;
		eye.y += moveSpeed;
		center.y += moveSpeed;

		for (int i = 0; i < 8; i++) {
			shipCollision[i].y += moveSpeed;
		}
	}
}

void sPressed() {
	if (vertical > -4.3) {
		vertical -= moveSpeed;
		eye.y -= moveSpeed;
		center.y -= moveSpeed;

		for (int i = 0; i < 8; i++) {
			shipCollision[i].y -= moveSpeed;
		}
	}
}

void Timer(int param) {
	Accumulator += StepSize * 0.1f;
	if (keys['a']) {
		aPressed();
	}
	if (keys['d']) {
		dPressed();
	}
	if (keys['w']) {
		wPressed();
	}
	if (keys['s']) {
		sPressed();
	}

	updatePositions();
	glutTimerFunc(StepSize, Timer, 1);
	glutPostRedisplay();
}

int main(int argc, char *argv[]) {
	// Initialize Global Variables
	Accumulator = 0.f;

	// Glut Setup
	glutInit(&argc, argv);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("Asteroid Runner");
	glutDisplayFunc(Draw);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(KeyboardUp);
	glutTimerFunc(StepSize, Timer, 1);

	// GLEW Setup (Windows only)
#ifdef _WIN32
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::cerr << "Error initializing glew! " << glewGetErrorString(err) << std::endl;
		return 1;
	}
#endif

	// OpenGL Setup
	Initialize();
	getGLversion();

	// Init models
	loadShip();
	loadAsteroid();
	loadTunnel();

	initAsteroidRows();
	setShipCollisions();

	// Shader Setup
	if (!InstallShader("Diffuse_vert.glsl", "Diffuse_frag.glsl")) {
		printf("Error installing shader!\n");
		return 1;
	}

	glutMainLoop();

	return 0;
}
