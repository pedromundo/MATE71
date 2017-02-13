//OpenGL Stuff
#include <algorithm>
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <stdio.h>
#include <math.h>
#include <string>
#include <sstream>

//My includes
#include "myDataStructures.h"
#include "initShaders.h"

GLvoid reshape(GLint x, GLint y);

//Shader Program Handles
GLuint basicShader, GUIShader, axisShader;
//Window Dimensions
GLuint wWidth = 1280, wHeight = 480;
//Data Dimensions
GLuint dWidth = 1280, dHeight = 480;
//Handlers for the VBO and FBOs
GLuint VertexArrayIDs[1], vertexbuffers[7];
//Control curve and revolution control variables
GLuint generatedcurvepoints, generatedrevolutionsteps, curvepoints = 30, revolutionSteps = 30;
//Revolution rotation axis
GLchar rotationAxis = 'y';
//OpenGL rendering mode
GLenum viewMode = GL_TRIANGLES;
//MVP Matrices
glm::mat4 Projection, View, Model;

//Camera Zoom Variables
GLfloat orthoBoxSize = 3.0f, perspFOV = 45.0f;
GLchar currentView = '4';

//Camera Movement variables
GLfloat deltaAngleX = 0.0f, deltaAngleY = 0.0f;
GLint xOrigin = -1, yOrigin = -1;
GLboolean isDragging = false;

//Buffer data vectors
std::vector<Point> controlPoints;
std::vector<Color> controlPointsColors;
std::vector<Point> points;
std::vector<Color> colors;
std::vector<GLuint> faces;

Point* heldPoint;

GLvoid initControlPointBufferData(){
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[2]);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat)*controlPoints.size(), controlPoints.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[3]);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat)*controlPointsColors.size(), controlPointsColors.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLvoid initBufferData(){
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat)*points.size(), points.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[1]);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat)*colors.size(), colors.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLvoid initReferenceBufferData(){
	std::vector<Point> axisPoints(4);
	axisPoints[0] = { 0.0, 0.0, 0.0 };
	axisPoints[1] = { 2.0, 0.0, 0.0 };
	axisPoints[2] = { 0.0, 2.0, 0.0 };
	axisPoints[3] = { 0.0, 0.0, 2.0 };

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[5]);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * 4, axisPoints.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	std::vector<GLuint> lineIDs(6);
	lineIDs[0] = 0;
	lineIDs[1] = 1;
	lineIDs[2] = 0;
	lineIDs[3] = 2;
	lineIDs[4] = 0;
	lineIDs[5] = 3;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexbuffers[6]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2 * sizeof(GLuint)*lineIDs.size(), lineIDs.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


Point getBezierPoint3D(std::vector<Point> points, GLint numPoints, GLfloat t) {
	GLint i = numPoints - 1;
	while (i > 0) {
		for (GLint k = 0; k < i; k++)
			points[k] = points[k] + t * (points[k + 1] - points[k]);
		i--;
	}
	Point answer = points[0];
	return answer;
}

GLvoid resetRevolution(){
	if (points.size() > generatedcurvepoints){
		points.erase(points.begin() + generatedcurvepoints, points.end());
		colors.erase(colors.begin() + generatedcurvepoints, colors.end());
		faces.clear();
		glutSwapBuffers();
		glutPostRedisplay();
	}
}

GLvoid resetDrawing(){
	points.clear();
	colors.clear();
	faces.clear();
	glutSwapBuffers();
	glutPostRedisplay();
}

void rotatePoints(GLint steps){
	if (!points.empty()){
		resetRevolution();
		std::vector<std::vector<Point>>slices(steps);
		GLfloat step = 360.0f / steps;
		for (GLint currStep = 1; currStep < steps; ++currStep)
		{
			for (auto pt : points){
				glm::vec4 point = glm::vec4(pt.x, pt.y, pt.z, 1);
				glm::mat4 rotation;
				switch (rotationAxis)
				{
				default:
				case 'x':
					rotation = glm::rotate(Model, glm::radians(step*currStep), glm::vec3(1, 0, 0));
					break;
				case 'y':
					rotation = glm::rotate(Model, glm::radians(step*currStep), glm::vec3(0, 1, 0));
					break;
				case 'z':
					rotation = glm::rotate(Model, glm::radians(step*currStep), glm::vec3(0, 0, 1));
					break;
				}

				glm::vec4 rotatedPoint = rotation * point;
				slices[currStep].push_back({ rotatedPoint.x, rotatedPoint.y, rotatedPoint.z });
				colors.push_back({ abs(rotatedPoint.x), abs(rotatedPoint.y), abs(rotatedPoint.z), 1.0f });
			}
		}

		generatedrevolutionsteps = slices.size();

		for (auto slice : slices){
			points.insert(points.end(), slice.begin(), slice.end());
		}
		initBufferData();
	}
}

void generateFaces(){
	if (points.size() > generatedcurvepoints){
		faces.clear();
		int numpoints = generatedrevolutionsteps * generatedcurvepoints;
		int lastlineindex = numpoints - generatedcurvepoints;
		for (GLint i = 0; i < numpoints; ++i){
			if (i < lastlineindex){
				if (i % generatedcurvepoints == 0){
					faces.push_back(i);
					faces.push_back(i + generatedcurvepoints + 1);
					faces.push_back(i + generatedcurvepoints);
				}
				else if ((i + 1) % generatedcurvepoints == 0){
					faces.push_back(i);
					faces.push_back(i + generatedcurvepoints);
					faces.push_back(i - 1);
				}
				else{
					faces.push_back(i);
					faces.push_back(i + generatedcurvepoints + 1);
					faces.push_back(i + generatedcurvepoints);
					faces.push_back(i);
					faces.push_back(i + generatedcurvepoints);
					faces.push_back(i - 1);
				}
			}
			else{
				if (i % generatedcurvepoints == 0){
					faces.push_back(i);
					faces.push_back(i % lastlineindex + 1);
					faces.push_back(i % lastlineindex);
				}
				else if ((i + 1) % generatedcurvepoints == 0){
					faces.push_back(i);
					faces.push_back(i % lastlineindex);
					faces.push_back(i - 1);
				}
				else{
					faces.push_back(i);
					faces.push_back(i % lastlineindex + 1);
					faces.push_back(i % lastlineindex);
					faces.push_back(i);
					faces.push_back(i % lastlineindex);
					faces.push_back(i - 1);
				}
			}
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexbuffers[4]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*faces.size(), faces.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

void generateCurve(GLint numPoints){
	resetDrawing();
	if (!controlPoints.empty()){
		GLdouble step = 1.0 / numPoints;
		points.push_back(controlPoints.front());
		colors.push_back({ 1.0f, 1.0f, 1.0f, 1.0f });

		for (GLint i = 1; i < numPoints; ++i)
		{
			points.push_back(getBezierPoint3D(controlPoints, controlPoints.size(), i*(float)step));
			colors.push_back({ 1.0f, 1.0f, 1.0f, 1.0f });
		}

		points.push_back(controlPoints.back());
		colors.push_back({ 1.0f, 1.0f, 1.0f, 1.0f });

		generatedcurvepoints = points.size();

		initBufferData();
	}
}

GLvoid shaderPlumbing(){
	glPointSize(1);

	//MVP matrix
	glm::mat4 MVP = Projection * View * Model;
	GLuint MVPId = glGetUniformLocation(basicShader, "MVP");
	glUniformMatrix4fv(MVPId, 1, GL_FALSE, glm::value_ptr(MVP));

	//position data
	//glBindVertexArray(VertexArrayIDs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[0]);
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "aPosition"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "aPosition"), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	//color data
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[1]);
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "aColor"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "aColor"), 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

}

GLvoid shaderPlumbingAxis(){
	glLineWidth(2);

	//MVP matrix
	glm::mat4 MVP = Projection * View * Model;
	GLuint MVPId = glGetUniformLocation(axisShader, "MVP");
	glUniformMatrix4fv(MVPId, 1, GL_FALSE, glm::value_ptr(MVP));

	//position data
	//glBindVertexArray(VertexArrayIDs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[5]);
	glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aPosition"));
	glVertexAttribPointer(glGetAttribLocation(axisShader, "aPosition"), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

}

GLvoid shaderPlumbingGUI(){
	//Point size 1 looks like shit
	glPointSize(5);

	//position data
	//glBindVertexArray(VertexArrayIDs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[2]);
	glEnableVertexAttribArray(glGetAttribLocation(GUIShader, "aPosition"));
	glVertexAttribPointer(glGetAttribLocation(GUIShader, "aPosition"), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	//color data
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[3]);
	glEnableVertexAttribArray(glGetAttribLocation(GUIShader, "aColor"));
	glVertexAttribPointer(glGetAttribLocation(GUIShader, "aColor"), 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
GLvoid updateTitle(){
	stringstream ss;
	string target;
	ss << "Assignment 1 - Revolution Solids - Current Revolution Axis: [" << rotationAxis << "]" << " - Current Curve Complexity:" << "[" << curvepoints << "] - Revolution Steps:" << "[" << revolutionSteps << "]";
	glutSetWindowTitle(ss.str().c_str());
}

GLvoid display(GLvoid){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);

	glBindVertexArray(VertexArrayIDs[0]);

	if (faces.empty()){
		glUseProgram(basicShader);
		shaderPlumbing();
		glViewport(wWidth / 2, 0, wWidth / 2, wHeight);
		glDrawArrays(GL_POINTS, 0, (GLsizei)points.size());
	}
	else{
		glUseProgram(basicShader);
		shaderPlumbing();
		glViewport(wWidth / 2, 0, wWidth / 2, wHeight);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexbuffers[4]);
		glDrawElements(viewMode, faces.size(), GL_UNSIGNED_INT, (void*)0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	glUseProgram(axisShader);
	shaderPlumbingAxis();
	glViewport(wWidth / 2, 0, wWidth / 2, wHeight);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexbuffers[6]);
	glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, (void*)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisable(GL_DEPTH_TEST);

	glUseProgram(GUIShader);
	shaderPlumbingGUI();
	glViewport(0, 0, wWidth / 2, wHeight);
	glDrawArrays(GL_POINTS, 0, (GLsizei)controlPoints.size());

	glutSwapBuffers();
	glutPostRedisplay();

	glBindVertexArray(0);
}

GLvoid initShaders() {
	basicShader = InitShader("basicShader.vert", "basicShader.frag");
	GUIShader = InitShader("GUIShader.vert", "GUIShader.frag");
	axisShader = InitShader("axisShader.vert", "axisShader.frag");
}

GLvoid generateFullRevolutionSolid(){
	generateCurve(curvepoints);
	rotatePoints(revolutionSteps);
	generateFaces();
}

GLvoid updateProjection(){
	switch (currentView){
	case '1':
		View = glm::lookAt(
			glm::vec3(3, 0, 0), //eye
			glm::vec3(0, 0, 0), //center
			glm::vec3(0, 1, 0)  //up
			);
		Projection = glm::ortho(-orthoBoxSize, orthoBoxSize, -orthoBoxSize, orthoBoxSize, 0.0f, 100.0f);
		break;
	case '2':
		View = glm::lookAt(
			glm::vec3(0, 3, 0), //eye
			glm::vec3(0, 0, 0), //center
			glm::vec3(1, 0, 0)  //up
			);
		Projection = glm::ortho(-orthoBoxSize, orthoBoxSize, -orthoBoxSize, orthoBoxSize, 0.0f, 100.0f);
		break;
	case '3':
		View = glm::lookAt(
			glm::vec3(0, 0, 3), //eye
			glm::vec3(0, 0, 0), //center
			glm::vec3(0, 1, 0)  //up
			);
		Projection = glm::ortho(-orthoBoxSize, orthoBoxSize, -orthoBoxSize, orthoBoxSize, 0.0f, 100.0f);
		break;
	case '4':
		View = glm::lookAt(
			glm::vec3(3, 3, 3), //eye
			glm::vec3(0, 0, 0), //center
			glm::vec3(0, 1, 0)  //up
			);
		Projection = glm::perspective(glm::radians(perspFOV), (GLfloat)wWidth / (GLfloat)wHeight, 0.1f, 100.0f);
		break;
	}
}

GLvoid updateProjectionZoom(){
	switch (currentView){
	case '1':
	case '2':
	case '3':
		Projection = glm::ortho(-orthoBoxSize, orthoBoxSize, -orthoBoxSize, orthoBoxSize, 0.0f, 100.0f);
		break;
	case '4':
		Projection = glm::perspective(glm::radians(perspFOV), (GLfloat)wWidth / (GLfloat)wHeight, 0.1f, 100.0f);
		break;
	}
}

GLvoid keyboard(GLubyte key, GLint x, GLint y)
{
	switch (key)
	{
	case 'q':
		viewMode = GL_POINTS;
		break;
	case 'w':
		viewMode = GL_LINES;
		break;
	case 'e':
		viewMode = GL_TRIANGLES;
		break;
	case '1':
	case '2':
	case '3':
	case '4':
		currentView = key;
		updateProjection();
		break;
	case 'x':
	case 'y':
	case 'z':
		rotationAxis = key;
		updateTitle();
		generateFullRevolutionSolid();
		break;
	case 'R':
		resetRevolution();
		break;
	case 'r':
		rotatePoints(revolutionSteps);
		break;
	case 'C':
		resetDrawing();
		controlPoints.clear();
		break;
	case 'c':
		generateCurve(curvepoints);
		break;
	case 'f':
		generateFaces();
		break;
	case 'a':
		if (curvepoints > 3){
			--curvepoints;
		}
		generateFullRevolutionSolid();
		updateTitle();
		break;
	case 's':
		++curvepoints;
		generateFullRevolutionSolid();
		updateTitle();
		break;
	case ',':
		if (revolutionSteps > 3){
			--revolutionSteps;
		}
		generateFullRevolutionSolid();
		updateTitle();
		break;
	case '.':
		++revolutionSteps;
		generateFullRevolutionSolid();
		updateTitle();
		break;
	case '0':
		orthoBoxSize = 3.0f;
		perspFOV = 45.0f;
		updateProjectionZoom();
		break;
	case '=':
		orthoBoxSize *= 1.1;
		perspFOV *= 0.9;
		updateProjectionZoom();
		break;
	case '-':
		orthoBoxSize *= 0.9;
		perspFOV *= 1.1;
		updateProjectionZoom();
		break;
	case 27:
#if defined (__APPLE__) || defined(MACOSX)
		exit(EXIT_SUCCESS);
#else
		glutDestroyWindow(glutGetWindow());
		return;
#endif
		break;
	default:
		break;
	}
}

GLvoid reshape(GLint x, GLint y)
{
	wWidth = x;
	wHeight = y;
	glViewport(0, 0, x, y);
	glutPostRedisplay();
}

GLvoid mouseHandler(GLint button, GLint state, GLint x, GLint y){
	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];
	GLfloat winX, winY, winZ;
	GLdouble posX, posY, posZ;

	winX = (GLfloat)x;
	winY = wHeight - (GLfloat)y;
	glReadPixels(GLint(winX), GLint(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);
	gluUnProject(winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

	if (button == GLUT_LEFT_BUTTON){
		if (winX <= wWidth / 2){
			if (state == GLUT_UP && !isDragging){
				controlPoints.push_back({ (GLfloat)posX, (GLfloat)posY, (GLfloat)posZ });
				controlPointsColors.push_back({ 1, 1, 1, 1.0f });
				initControlPointBufferData();

				generateFullRevolutionSolid();

				printf("Window (bottom left based): %.0f %.0f\n", winX, winY);
				printf("World: %lf %lf %lf\n", posX, posY, posZ);
				isDragging = false;
			}
		}
		else{
			xOrigin = x;
			yOrigin = y;
			isDragging = true;
		}
		if (state == GLUT_UP) {
			xOrigin = -1;
			yOrigin = -1;
			isDragging = false;
		}
	}
	else if (GLUT_RIGHT_BUTTON){
		auto heldPointIterator = std::find_if(controlPoints.begin(), controlPoints.end(), [posX, posY](Point a)->bool { return abs(a.x - posX) < 0.03 && abs(a.y - posY) < 0.03; });
		if (winX <= wWidth / 2){
			if (state == GLUT_DOWN){
				if (controlPoints.end() != heldPointIterator){
					heldPoint = &*heldPointIterator;
					printf("Point Held: %lf %lf\n", (*heldPoint).x, (*heldPoint).y);
				}
			}
			else if (state == GLUT_UP){
				if (glutGetModifiers() & GLUT_ACTIVE_SHIFT){
					if (controlPoints.end() != heldPointIterator && (controlPoints.begin() != controlPoints.end())){
						controlPoints.erase(heldPointIterator);
						initControlPointBufferData();
					}
				}
				generateFullRevolutionSolid();
				heldPoint = NULL;
			}
		}
	}
}

void mouseMove(GLint x, GLint y) {

	// this will only be true when the left button is down
	if (xOrigin >= 0) {
		// update deltaAngle
		deltaAngleX = (x - xOrigin) * 0.1f;
		xOrigin = x;

		glm::vec3 xRotation = glm::vec3(glm::vec4(0.0, 1.0, 0.0, 1.0)*View);
		View = glm::rotate(View, glm::radians(deltaAngleX), xRotation);
	}

	if (yOrigin >= 0) {
		// update deltaAngle
		deltaAngleY = (y - yOrigin) * 0.1f;
		yOrigin = y;

		glm::vec3 yRotation = glm::vec3(glm::vec4(1.0, 0.0, 0.0, 1.0)*View);
		View = glm::rotate(View, glm::radians(deltaAngleY), yRotation);
	}

	if (heldPoint != NULL){
		GLint viewport[4];
		GLdouble modelview[16];
		GLdouble projection[16];
		GLfloat winX, winY, winZ;
		GLdouble posX, posY, posZ;

		winX = (GLfloat)x;
		winY = wHeight - (GLfloat)y;
		glReadPixels(GLint(winX), GLint(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
		glGetDoublev(GL_PROJECTION_MATRIX, projection);
		glGetIntegerv(GL_VIEWPORT, viewport);
		gluUnProject(winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

		(*heldPoint).x = (GLfloat)posX;
		(*heldPoint).y = (GLfloat)posY;
		(*heldPoint).z = (GLfloat)posY;
		initControlPointBufferData();
	}
}


GLint initGL(GLint *argc, GLchar **argv)
{
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(wWidth, wHeight);
	stringstream ss;
	string target;
	ss << "Assignment 1 - Revolution Solids - Current Revolution Axis: [" << rotationAxis << "]" << " - Current Curve Complexity:" << "[" << curvepoints << "] - Revolution Steps:" << "[" << revolutionSteps << "]";
	glutCreateWindow(ss.str().c_str());
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouseHandler);
	glutMotionFunc(mouseMove);
	glewInit();
	return 1;
}

inline GLfloat interpolate(const GLfloat a, const GLfloat b, const GLfloat coefficient)
{
	return a + coefficient * (b - a);
}

GLint main(GLint argc, GLchar **argv)
{
	//Setting up our MVP Matrices

	Model = glm::mat4(1.0f);
	View = glm::lookAt(
		glm::vec3(3, 3, 3), //eye
		glm::vec3(0, 0, 0), //center
		glm::vec3(0, 1, 0)  //up
		);
	Projection = glm::perspective(glm::radians(perspFOV), (GLfloat)wWidth / (GLfloat)wHeight, 0.1f, 100.0f);

#if defined(__linux__)
	setenv("DISPLAY", ":0", 0);
#endif	

	if (false == initGL(&argc, argv))
	{
		return EXIT_FAILURE;
	}

	glGenVertexArrays(1, VertexArrayIDs);
	glGenBuffers(7, vertexbuffers);

	initShaders();

	initReferenceBufferData();

	glutMainLoop();
}