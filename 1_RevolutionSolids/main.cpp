//OpenGL Stuff
#include <algorithm>
#include <GL/glew.h>

#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL2/SOIL2.h>
#include <stdio.h>
#include <math.h>
#include <string>
#include <sstream>

//My includes
#include "myDataStructures.h"
#include "initShaders.h"

GLvoid reshape(GLint x, GLint y);

//Shader Program Handles
GLuint BasicShader, GUIShader, AxisShader;
//Window Dimensions
GLuint WWidth = 1280, WHeight = 480;
//Handlers for the VBO and FBOs
GLuint VertexArrayIDs[1], VertexBuffers[9], TextureBuffers[1];
//Control curve and revolution control variables
GLuint GeneratedCurvePoints, GeneratedRevolutionSteps, CurvePoints = 32, RevolutionSteps = 64;
//Revolution rotation axis
GLchar RotationAxis = 'y';
//OpenGL rendering mode
GLenum ViewMode = GL_TRIANGLES;
//MVP Matrices
glm::mat4 Projection, View, Model;

//Camera Zoom Variables
GLfloat OrthoBoxSize = 1.5f, PerspFOV = 25.0f;
GLchar CurrentView = '4';

//Camera Movement variables
GLfloat DeltaAngleX = 0.0f, DeltaAngleY = 0.0f;
GLint XOrigin = -1, YOrigin = -1;
GLboolean IsDragging = false;

//Buffer data vectors
std::vector<Point> ControlPoints;
std::vector<Point> Points;
std::vector<Point> Normals;
std::vector<Color> Colors;
std::vector<glm::vec2> UVs;
std::vector<GLuint> Faces;

//Lighting variables
glm::vec3 LightPos(3);
glm::vec3 EyePos(3);
GLfloat SpecularStrength = 1.0f;
GLint SpecularCoefficient = 128;

//Texture variables
GLint WTex, HTex, CTex;
GLubyte* Texture;

Point* HeldPoint;

GLvoid initControlPointBufferData() {
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[2]);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat)*ControlPoints.size(), ControlPoints.data(), GL_STATIC_DRAW);

	if (GeneratedCurvePoints) {
		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[3]);
		glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat)*GeneratedCurvePoints, Points.data(), GL_STATIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLvoid initBufferData() {
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat)*Points.size(), Points.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[1]);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat)*Colors.size(), Colors.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[7]);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat)*Normals.size(), Normals.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[8]);
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(GLfloat)*UVs.size(), UVs.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLvoid initReferenceBufferData() {
	std::vector<Point> axisPoints(4);
	axisPoints[0] = { 0.0, 0.0, 0.0 };
	axisPoints[1] = { 2.0, 0.0, 0.0 };
	axisPoints[2] = { 0.0, 2.0, 0.0 };
	axisPoints[3] = { 0.0, 0.0, 2.0 };

	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[5]);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * 4, axisPoints.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	std::vector<GLuint> lineIDs(6);
	lineIDs[0] = 0;
	lineIDs[1] = 1;
	lineIDs[2] = 0;
	lineIDs[3] = 2;
	lineIDs[4] = 0;
	lineIDs[5] = 3;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VertexBuffers[6]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2 * sizeof(GLuint)*lineIDs.size(), lineIDs.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

GLvoid initTextureData(const std::string& filename) {
	Texture = SOIL_load_image(filename.data(), &WTex, &HTex, &CTex, SOIL_LOAD_RGB);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, TextureBuffers[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WTex, HTex, 0, GL_RGB, GL_UNSIGNED_BYTE, Texture);
	glUniform1i(glGetUniformLocation(BasicShader, "tex"), 0);
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

GLvoid resetRevolution() {
	if (Points.size() > GeneratedCurvePoints) {
		Points.erase(Points.begin() + GeneratedCurvePoints, Points.end());
		Colors.erase(Colors.begin() + GeneratedCurvePoints, Colors.end());
		Faces.clear();
		glutSwapBuffers();
		glutPostRedisplay();
	}
}

GLvoid resetDrawing() {
	Points.clear();
	Normals.clear();
	UVs.clear();
	Colors.clear();
	Faces.clear();
	glutSwapBuffers();
	glutPostRedisplay();
}

void rotatePoints(GLint steps) {
	if (!Points.empty()) {
		resetRevolution();
		std::vector<std::vector<Point>>slices(steps);
		GLdouble step = 360.0 / (steps);

		for (GLint currStep = 1; currStep < steps; ++currStep)
		{
			for (auto pt : Points) {
				glm::vec4 point = glm::vec4(pt.x, pt.y, pt.z, 1);
				glm::mat4 rotation;

				auto rotationAngle = glm::ceil(step*currStep);

				switch (RotationAxis)
				{
				default:
				case 'x':
					rotation = glm::rotate(Model, (GLfloat)glm::radians(rotationAngle), glm::vec3(1, 0, 0));
					break;
				case 'y':
					rotation = glm::rotate(Model, (GLfloat)glm::radians(rotationAngle), glm::vec3(0, 1, 0));
					break;
				case 'z':
					rotation = glm::rotate(Model, (GLfloat)glm::radians(rotationAngle), glm::vec3(0, 0, 1));
					break;
				}

				glm::vec4 rotatedPoint = rotation * point;
				slices[currStep].push_back({ rotatedPoint.x, rotatedPoint.y, rotatedPoint.z });
				Colors.push_back({ abs(rotatedPoint.x), abs(rotatedPoint.y), abs(rotatedPoint.z), 1.0f });
			}
		}

		GeneratedRevolutionSteps = slices.size();

		for (auto slice : slices) {
			Points.insert(Points.end(), slice.begin(), slice.end());
		}
		initBufferData();
	}
}

inline void pushNormal(GLint i, GLint gridSize, GLint columnSize) {
	//Verificar se ele é do começo ou do fim da linha
	GLint direita = i + columnSize, esquerda = i - columnSize;
	if (direita > gridSize - 1) {
		direita = direita - gridSize;
	}

	if (esquerda < 0) {
		esquerda = esquerda + gridSize;
	}

	if (i % columnSize == 0) { //Primeira linha
		GLint iA = i, iB = direita + 1, iC = direita, iD = i + 1, iE = esquerda;
		glm::vec3 A(Points[iA].x, Points[iA].y, Points[iA].z),
			B(Points[iB].x, Points[iB].y, Points[iB].z),
			C(Points[iC].x, Points[iC].y, Points[iC].z),
			D(Points[iD].x, Points[iD].y, Points[iD].z),
			E(Points[iE].x, Points[iE].y, Points[iE].z);

		glm::vec3 normalDirectionABC = glm::cross((B - A), (C - A));
		glm::vec3 normalABC = normalDirectionABC / (GLfloat)normalDirectionABC.length();

		glm::vec3 normalDirectionADB = glm::cross((D - A), (B - A));
		glm::vec3 normalADB = normalDirectionADB / (GLfloat)normalDirectionADB.length();

		glm::vec3 normalDirectionAED = glm::cross((E - A), (D - A));
		glm::vec3 normalAED = normalDirectionAED / (GLfloat)normalDirectionAED.length();

		glm::vec3 meanNormal = (normalABC + normalADB + normalAED) / 1.5f;

		Normals.push_back({ meanNormal.x, meanNormal.y, meanNormal.z });
		return;
	}
	else if ((i + 1) % columnSize == 0) { //Ultima linha
		GLint iA = i, iB = direita, iC = i - 1, iD = esquerda - 1, iE = esquerda;
		glm::vec3 A(Points[iA].x, Points[iA].y, Points[iA].z),
			B(Points[iB].x, Points[iB].y, Points[iB].z),
			C(Points[iC].x, Points[iC].y, Points[iC].z),
			D(Points[iD].x, Points[iD].y, Points[iD].z),
			E(Points[iE].x, Points[iE].y, Points[iE].z);

		glm::vec3 normalDirectionABC = glm::cross((B - A), (C - A));
		glm::vec3 normalABC = normalDirectionABC / (GLfloat)normalDirectionABC.length();

		glm::vec3 normalDirectionACD = glm::cross((C - A), (D - A));
		glm::vec3 normalACD = normalDirectionACD / (GLfloat)normalDirectionACD.length();

		glm::vec3 normalDirectionADE = glm::cross((C - A), (D - A));
		glm::vec3 normalADE = normalDirectionADE / (GLfloat)normalDirectionADE.length();

		glm::vec3 meanNormal = (normalABC + normalACD + normalADE) / 1.5f;

		Normals.push_back({ meanNormal.x, meanNormal.y, meanNormal.z });
		return;
	}
	else { //Meio
		GLint iA = i, iB = direita, iC = i - 1, iD = direita + 1, iE = i + 1, iF = esquerda, iG = esquerda - 1;
		glm::vec3 A(Points[iA].x, Points[iA].y, Points[iA].z),
			B(Points[iB].x, Points[iB].y, Points[iB].z),
			C(Points[iC].x, Points[iC].y, Points[iC].z),
			D(Points[iD].x, Points[iD].y, Points[iD].z),
			E(Points[iE].x, Points[iE].y, Points[iE].z),
			F(Points[iF].x, Points[iF].y, Points[iF].z),
			G(Points[iG].x, Points[iG].y, Points[iG].z);

		glm::vec3 normalDirectionABC = glm::cross((B - A), (C - A));
		glm::vec3 normalABC = normalDirectionABC / (GLfloat)normalDirectionABC.length();

		glm::vec3 normalDirectionADB = glm::cross((D - A), (B - A));
		glm::vec3 normalADB = normalDirectionADB / (GLfloat)normalDirectionADB.length();

		glm::vec3 normalDirectionAED = glm::cross((E - A), (D - A));
		glm::vec3 normalAED = normalDirectionAED / (GLfloat)normalDirectionAED.length();

		glm::vec3 normalDirectionAFE = glm::cross((F - A), (E - A));
		glm::vec3 normalAFE = normalDirectionAFE / (GLfloat)normalDirectionAFE.length();

		glm::vec3 normalDirectionACG = glm::cross((C - A), (G - A));
		glm::vec3 normalACG = normalDirectionACG / (GLfloat)normalDirectionACG.length();

		glm::vec3 normalDirectionAGF = glm::cross((G - A), (F - A));
		glm::vec3 normalAGF = normalDirectionAGF / (GLfloat)normalDirectionAGF.length();

		glm::vec3 meanNormal = (normalABC + normalADB + normalAED + normalAFE + normalACG + normalAGF) / 6.0f;

		Normals.push_back({ meanNormal.x, meanNormal.y, meanNormal.z });
		return;
	}
}

void generateFaces() {
	if (Points.size() > GeneratedCurvePoints) {
		Faces.clear();
		Normals.clear();
		UVs.clear();
		GLint numpoints = GeneratedRevolutionSteps * GeneratedCurvePoints;
		GLint lastlineindex = numpoints - GeneratedCurvePoints;
		for (GLint i = 0; i < numpoints; ++i) {
			if (i < lastlineindex) {
				if (i % GeneratedCurvePoints == 0) {
					Faces.push_back(i);
					Faces.push_back(i + GeneratedCurvePoints + 1);
					Faces.push_back(i + GeneratedCurvePoints);

					pushNormal(i, numpoints, GeneratedCurvePoints);
				}
				else if ((i + 1) % GeneratedCurvePoints == 0) {
					Faces.push_back(i);
					Faces.push_back(i + GeneratedCurvePoints);
					Faces.push_back(i - 1);

					pushNormal(i, numpoints, GeneratedCurvePoints);
				}
				else {
					Faces.push_back(i);
					Faces.push_back(i + GeneratedCurvePoints + 1);
					Faces.push_back(i + GeneratedCurvePoints);
					Faces.push_back(i);
					Faces.push_back(i + GeneratedCurvePoints);
					Faces.push_back(i - 1);

					pushNormal(i, numpoints, GeneratedCurvePoints);
				}
			}
			else {
				if (i % GeneratedCurvePoints == 0) {
					Faces.push_back(i);
					Faces.push_back(i % lastlineindex + 1);
					Faces.push_back(i % lastlineindex);

					pushNormal(i, numpoints, GeneratedCurvePoints);
				}
				else if ((i + 1) % GeneratedCurvePoints == 0) {
					Faces.push_back(i);
					Faces.push_back(i % lastlineindex);
					Faces.push_back(i - 1);

					pushNormal(i, numpoints, GeneratedCurvePoints);
				}
				else {
					Faces.push_back(i);
					Faces.push_back(i % lastlineindex + 1);
					Faces.push_back(i % lastlineindex);
					Faces.push_back(i);
					Faces.push_back(i % lastlineindex);
					Faces.push_back(i - 1);

					pushNormal(i, numpoints, GeneratedCurvePoints);
				}
			}

			GLint row = i % GeneratedCurvePoints;
			GLint column = i / GeneratedCurvePoints;

			GLfloat u;
			if (column < GeneratedRevolutionSteps / 2.0f) {
				u = glm::mix(0.0f, 2.0f, 1.0f / GeneratedRevolutionSteps * column);
			}
			else {
				if (column == GeneratedRevolutionSteps - 1) {
					auto hack = 1;
					u = glm::mix(0.0f, 2.0f, 1.0f / GeneratedRevolutionSteps * hack);
				}
				else {
					u = glm::mix(2.0f, 0.0f, 1.0f / GeneratedRevolutionSteps * column);
				}
			}

			GLfloat v = (1.0f / (GeneratedCurvePoints - 1)) * row;

			UVs.push_back(glm::vec2(u, v));
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VertexBuffers[4]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*Faces.size(), Faces.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VertexBuffers[7]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(GLfloat)*Normals.size(), Normals.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VertexBuffers[8]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2 * sizeof(GLfloat)*UVs.size(), UVs.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

void generateCurve(GLint numPoints) {
	resetDrawing();
	if (!ControlPoints.empty()) {
		GLdouble step = 1.0 / (numPoints - 1);
		Points.push_back(ControlPoints.front());
		Colors.push_back({ 1.0f, 1.0f, 1.0f, 1.0f });

		for (GLint i = 1; i < numPoints - 1; ++i)
		{
			Points.push_back(getBezierPoint3D(ControlPoints, ControlPoints.size(), i*(GLfloat)step));
			Colors.push_back({ 1.0f, 1.0f, 1.0f, 1.0f });
		}

		Points.push_back(ControlPoints.back());
		Colors.push_back({ 1.0f, 1.0f, 1.0f, 1.0f });
		Normals.push_back({ 1.0f, 1.0f, 1.0f });

		GeneratedCurvePoints = Points.size();

		initBufferData();
	}
}

GLvoid shaderPlumbing() {
	glPointSize(1);

	//Model matrix
	glm::mat4 M = Model;
	GLuint MId = glGetUniformLocation(BasicShader, "M");
	glUniformMatrix4fv(MId, 1, GL_FALSE, glm::value_ptr(M));

	// Calculate normal matrix (transpose of inverse of model matrix)
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(Model)));
	GLuint normalMatrixId = glGetUniformLocation(BasicShader, "normalMatrix");
	glUniformMatrix3fv(normalMatrixId, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	//MVP matrix
	glm::mat4 MVP = Projection * View * Model;
	GLuint MVPId = glGetUniformLocation(BasicShader, "MVP");
	glUniformMatrix4fv(MVPId, 1, GL_FALSE, glm::value_ptr(MVP));

	//Lighting vectors
	GLuint lightPosID = glGetUniformLocation(BasicShader, "lightPos_world");
	glUniform3fv(lightPosID, 1, glm::value_ptr(LightPos));
	GLuint eyePosID = glGetUniformLocation(BasicShader, "eyePos_world");
	glUniform3fv(eyePosID, 1, glm::value_ptr(EyePos));

	//Material lighting uniforms
	GLuint specularStrengthID = glGetUniformLocation(BasicShader, "specularStrength");
	glUniform1f(specularStrengthID, SpecularStrength);
	GLuint specularCoefficientID = glGetUniformLocation(BasicShader, "specularCoefficient");
	glUniform1i(specularCoefficientID, SpecularCoefficient);

	//position data
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[0]);
	glEnableVertexAttribArray(glGetAttribLocation(BasicShader, "aPosition_object"));
	glVertexAttribPointer(glGetAttribLocation(BasicShader, "aPosition_object"), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	//normal data
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[7]);
	glEnableVertexAttribArray(glGetAttribLocation(BasicShader, "aNormal_object"));
	glVertexAttribPointer(glGetAttribLocation(BasicShader, "aNormal_object"), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	//uv data
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[8]);
	glEnableVertexAttribArray(glGetAttribLocation(BasicShader, "aUV"));
	glVertexAttribPointer(glGetAttribLocation(BasicShader, "aUV"), 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	//color data
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[1]);
	glEnableVertexAttribArray(glGetAttribLocation(BasicShader, "aColor"));
	glVertexAttribPointer(glGetAttribLocation(BasicShader, "aColor"), 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLvoid shaderPlumbingAxis() {
	glLineWidth(2);

	//MVP matrix
	glm::mat4 MVP = Projection * View * Model;
	GLuint MVPId = glGetUniformLocation(AxisShader, "MVP");
	glUniformMatrix4fv(MVPId, 1, GL_FALSE, glm::value_ptr(MVP));

	//position data
	//glBindVertexArray(VertexArrayIDs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[5]);
	glEnableVertexAttribArray(glGetAttribLocation(AxisShader, "aPosition"));
	glVertexAttribPointer(glGetAttribLocation(AxisShader, "aPosition"), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLvoid shaderPlumbingGUI() {
	//Point size 1 looks like shit
	glPointSize(10);

	//position data
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[2]);
	glEnableVertexAttribArray(glGetAttribLocation(GUIShader, "aPosition"));
	glVertexAttribPointer(glGetAttribLocation(GUIShader, "aPosition"), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
GLvoid updateTitle() {
	stringstream ss;
	string target;
	ss << "Assignment 1 - Revolution Solids - Current Revolution Axis: [" << RotationAxis << "]" << " - Current Curve Complexity:" << "[" << CurvePoints << "] - Revolution Steps:" << "[" << RevolutionSteps << "]";
	glutSetWindowTitle(ss.str().c_str());
}

GLvoid display(GLvoid) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);

	glBindVertexArray(VertexArrayIDs[0]);

	if (Faces.empty()) {
		glUseProgram(BasicShader);
		shaderPlumbing();
		glViewport(WWidth / 2, 0, WWidth / 2, WHeight);
		glDrawArrays(GL_POINTS, 0, (GLsizei)Points.size());
	}
	else {
		glUseProgram(BasicShader);
		shaderPlumbing();
		glViewport(WWidth / 2, 0, WWidth / 2, WHeight);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VertexBuffers[4]);
		glDrawElements(ViewMode, Faces.size(), GL_UNSIGNED_INT, (void*)0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	glUseProgram(AxisShader);
	shaderPlumbingAxis();
	glViewport(WWidth / 2, 0, WWidth / 2, WHeight);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VertexBuffers[6]);
	glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, (void*)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisable(GL_DEPTH_TEST);

	glUseProgram(GUIShader);
	shaderPlumbingGUI();
	glViewport(0, 0, WWidth / 2, WHeight);
	glDrawArrays(GL_POINTS, 0, (GLsizei)ControlPoints.size());
	if (GeneratedCurvePoints) {
		//Plumbing the curve data by hand
		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffers[3]);
		glEnableVertexAttribArray(glGetAttribLocation(GUIShader, "aPosition"));
		glVertexAttribPointer(glGetAttribLocation(GUIShader, "aPosition"), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_LINE_STRIP, 0, GeneratedCurvePoints);
	}

	glutSwapBuffers();
	glutPostRedisplay();

	glBindVertexArray(0);
}

GLvoid initShaders() {
	BasicShader = InitShader("basicShader.vert", "basicShader.frag");
	GUIShader = InitShader("GUIShader.vert", "GUIShader.frag");
	AxisShader = InitShader("axisShader.vert", "axisShader.frag");
}

GLvoid generateFullRevolutionSolid() {
	generateCurve(CurvePoints);
	rotatePoints(RevolutionSteps);
	generateFaces();
}

GLvoid updateProjection() {
	switch (CurrentView) {
	case '1':
		View = glm::lookAt(
			glm::vec3(3, 0, 0), //eye
			glm::vec3(0, 0, 0), //center
			glm::vec3(0, 1, 0)  //up
		);
		EyePos = glm::vec3(3, 0, 0);
		Projection = glm::ortho(-OrthoBoxSize, OrthoBoxSize, -OrthoBoxSize, OrthoBoxSize, 0.0f, 100.0f);
		break;
	case '2':
		View = glm::lookAt(
			glm::vec3(0, 3, 0), //eye
			glm::vec3(0, 0, 0), //center
			glm::vec3(1, 0, 0)  //up
		);
		EyePos = glm::vec3(0, 3, 0);
		Projection = glm::ortho(-OrthoBoxSize, OrthoBoxSize, -OrthoBoxSize, OrthoBoxSize, 0.0f, 100.0f);
		break;
	case '3':
		View = glm::lookAt(
			glm::vec3(0, 0, 3), //eye
			glm::vec3(0, 0, 0), //center
			glm::vec3(0, 1, 0)  //up
		);
		EyePos = glm::vec3(0, 0, 3);
		Projection = glm::ortho(-OrthoBoxSize, OrthoBoxSize, -OrthoBoxSize, OrthoBoxSize, 0.0f, 100.0f);
		break;
	case '4':
		View = glm::lookAt(
			glm::vec3(3, 3, 3), //eye
			glm::vec3(0, 0, 0), //center
			glm::vec3(0, 1, 0)  //up
		);
		EyePos = glm::vec3(3, 3, 3);
		Projection = glm::perspective(glm::radians(PerspFOV), (GLfloat)WWidth / (GLfloat)WHeight, 0.1f, 100.0f);
		break;
	}
}

GLvoid updateProjectionZoom() {
	switch (CurrentView) {
	case '1':
	case '2':
	case '3':
		Projection = glm::ortho(-OrthoBoxSize, OrthoBoxSize, -OrthoBoxSize, OrthoBoxSize, 0.0f, 100.0f);
		break;
	case '4':
		Projection = glm::perspective(glm::radians(PerspFOV), (GLfloat)WWidth / (GLfloat)WHeight, 0.1f, 100.0f);
		break;
	}
}

GLvoid keyboard(GLubyte key, GLint x, GLint y)
{
	switch (key)
	{
	case 'u': {
		initTextureData("steel.jpg");
		SpecularStrength = 1.0f;
		SpecularCoefficient = 128;
		break;
	}
	case 'i': {
		initTextureData("fabric.jpg");
		SpecularStrength = 0.1f;
		SpecularCoefficient = 16;
		break;
	}
	case 'o': {
		initTextureData("clay.jpg");
		SpecularStrength = 0.3f;
		SpecularCoefficient = 16;
		break;
	}
	case 'q':
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	case 'w':
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case 'e':
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case '1':
	case '2':
	case '3':
	case '4':
		CurrentView = key;
		updateProjection();
		break;
	case 'x':
	case 'y':
	case 'z':
		RotationAxis = key;
		updateTitle();
		generateFullRevolutionSolid();
		break;
	case 'R':
		//resetRevolution();
		break;
	case 'r':
		//rotatePoints(RevolutionSteps);
		break;
	case 'C':
		resetDrawing();
		ControlPoints.clear();
		break;
		//case 'c':
			//generateCurve(CurvePoints);
			//break;
	case 'f':
		//generateFaces();
		break;
	case 'a':
		if (CurvePoints > 3) {
			--CurvePoints;
		}
		generateFullRevolutionSolid();
		updateTitle();
		break;
	case 's':
		++CurvePoints;
		generateFullRevolutionSolid();
		updateTitle();
		break;
	case ',':
		if (RevolutionSteps > 3) {
			--RevolutionSteps;
		}
		generateFullRevolutionSolid();
		updateTitle();
		break;
	case '.':
		++RevolutionSteps;
		generateFullRevolutionSolid();
		updateTitle();
		break;
	case '0':
		OrthoBoxSize = 1.5f;
		PerspFOV = 25.0f;
		updateProjectionZoom();
		break;
	case '=':
		OrthoBoxSize *= 0.9;
		PerspFOV *= 0.9;
		updateProjectionZoom();
		break;
	case '-':
		OrthoBoxSize *= 1.1;
		PerspFOV *= 1.1;
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
	WWidth = x;
	WHeight = y;
	glViewport(0, 0, x, y);
	glutPostRedisplay();
}

GLvoid reorderPoints() {
	std::sort(ControlPoints.begin(), ControlPoints.end(), [](Point a, Point b)->bool { return a.y > b.y; });
}

GLvoid mouseHandler(GLint button, GLint state, GLint x, GLint y) {
	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];
	GLfloat winX, winY, winZ;
	GLdouble posX, posY, posZ;

	winX = (GLfloat)x;
	winY = WHeight - (GLfloat)y;
	glReadPixels(GLint(winX), GLint(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);
	gluUnProject(winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

	if (button == GLUT_LEFT_BUTTON) {
		if (winX <= WWidth / 2) {
			if (state == GLUT_UP && !IsDragging) {
				ControlPoints.push_back({ (GLfloat)posX, (GLfloat)posY, (GLfloat)0 });
				reorderPoints();
				generateFullRevolutionSolid();
				initControlPointBufferData();
				printf("Window (bottom left based): %.0f %.0f\n", winX, winY);
				printf("World: %lf %lf %lf\n", posX, posY, posZ);
				IsDragging = false;
			}
		}
		else {
			XOrigin = x;
			YOrigin = y;
			IsDragging = true;
		}
		if (state == GLUT_UP) {
			XOrigin = -1;
			YOrigin = -1;
			IsDragging = false;
		}
	}
	else if (GLUT_RIGHT_BUTTON) {
		auto heldPointIterator = std::find_if(ControlPoints.begin(), ControlPoints.end(), [posX, posY](Point a)->bool { return abs(a.x - posX) < 0.05 && abs(a.y - posY) < 0.05; });
		if (winX <= WWidth / 2) {
			if (state == GLUT_DOWN) {
				if (ControlPoints.end() != heldPointIterator) {
					HeldPoint = &*heldPointIterator;
					printf("Point Held: %lf %lf\n", (*HeldPoint).x, (*HeldPoint).y);
				}
			}
			else if (state == GLUT_UP) {
				if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
					if (ControlPoints.end() != heldPointIterator && (ControlPoints.begin() != ControlPoints.end())) {
						ControlPoints.erase(heldPointIterator);
						initControlPointBufferData();
					}
				}
				reorderPoints();
				generateFullRevolutionSolid();
				initControlPointBufferData();
				HeldPoint = NULL;
			}
		}
	}
}

void mouseMove(GLint x, GLint y) {
	// this will only be true when the left button is down
	if (XOrigin >= 0) {
		// update deltaAngle
		DeltaAngleX = (x - XOrigin) * 0.1f;
		XOrigin = x;

		glm::vec3 xRotation = glm::vec3(glm::vec4(0.0, 1.0, 0.0, 1.0)*View);
		View = glm::rotate(View, glm::radians(DeltaAngleX), xRotation);
		glm::mat4 viewInv = glm::inverse(View);
		EyePos = glm::vec3(viewInv[3][0], viewInv[3][1], viewInv[3][2]);
	}

	if (YOrigin >= 0) {
		// update deltaAngle
		DeltaAngleY = (y - YOrigin) * 0.1f;
		YOrigin = y;

		glm::vec3 yRotation = glm::vec3(glm::vec4(1.0, 0.0, 0.0, 1.0)*View);
		View = glm::rotate(View, glm::radians(DeltaAngleY), yRotation);
		glm::mat4 viewInv = glm::inverse(View);
		EyePos = glm::vec3(viewInv[3][0], viewInv[3][1], viewInv[3][2]);
	}

	if (HeldPoint != NULL) {
		GLint viewport[4];
		GLdouble modelview[16];
		GLdouble projection[16];
		GLfloat winX, winY, winZ;
		GLdouble posX, posY, posZ;

		winX = (GLfloat)x;
		winY = WHeight - (GLfloat)y;
		glReadPixels(GLint(winX), GLint(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
		glGetDoublev(GL_PROJECTION_MATRIX, projection);
		glGetIntegerv(GL_VIEWPORT, viewport);
		gluUnProject(winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

		(*HeldPoint).x = (GLfloat)posX;
		(*HeldPoint).y = (GLfloat)posY;
		(*HeldPoint).z = (GLfloat)0;
		initControlPointBufferData();
	}
}

GLint initGL(GLint *argc, GLchar **argv)
{
	printf("Initializing GLUT...\n");
	glutInit(argc, argv);

	printf("Setting display mode...\n");
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	printf("Setting window size...\n");
	glutInitWindowSize(WWidth, WHeight);
	stringstream ss;
	string target;
	ss << "Assignment 1 - Revolution Solids - Current Revolution Axis: [" << RotationAxis << "]" << " - Current Curve Complexity:" << "[" << CurvePoints << "] - Revolution Steps:" << "[" << RevolutionSteps << "]";
	printf("Creating window...\n");
	glutCreateWindow(ss.str().c_str());
	printf("Setting callbacks...\n");
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouseHandler);
	glutMotionFunc(mouseMove);

	// Initialize GLEW with experimental mode for modern OpenGL
	printf("Initializing GLEW...\n");
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();

	printf("GLEW initialized successfully\n");
	printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
	printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Vendor: %s\n", glGetString(GL_VENDOR));

	return 1;
}

inline GLfloat interpolate(const GLfloat a, const GLfloat b, const GLfloat coefficient)
{
	return a + coefficient * (b - a);
}

GLint main(GLint argc, GLchar **argv)
{
	printf("Starting main function...\n");
	//Setting up our MVP Matrices

	Model = glm::mat4(1.0f);
	View = glm::lookAt(
		glm::vec3(3, 3, 3), //eye
		glm::vec3(0, 0, 0), //center
		glm::vec3(0, 1, 0)  //up
	);
	EyePos = glm::vec3(3, 3, 3);
	Projection = glm::perspective(glm::radians(PerspFOV), (GLfloat)WWidth / (GLfloat)WHeight, 0.1f, 100.0f);

#if defined(__linux__)
	setenv("DISPLAY", ":0", 0);
#elif defined(__APPLE__)
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

	if (false == initGL(&argc, argv))
	{
		return EXIT_FAILURE;
	}

	printf("Initializing OpenGL buffers...\n");
	glGenVertexArrays(1, VertexArrayIDs);
	printf("Generated VAOs\n");
	glGenBuffers(9, VertexBuffers);
	printf("Generated VBOs\n");
	glGenTextures(1, TextureBuffers);
	printf("Generated textures\n");

	printf("Initializing shaders...\n");
	initShaders();
	printf("Shaders initialized\n");

	printf("Initializing texture data...\n");
	const char* texturePath = "steel.jpg";
	initTextureData(const_cast<char*>(texturePath));
	printf("Texture data initialized\n");

	printf("Initializing reference buffers...\n");
	initReferenceBufferData();
	printf("Reference buffers initialized\n");

	printf("Entering main loop...\n");
	glutMainLoop();
	printf("Main loop exited\n");
}
