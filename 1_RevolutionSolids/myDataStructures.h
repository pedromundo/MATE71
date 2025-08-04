#ifndef MY_DATA_STRUCTURES
#define MY_DATA_STRUCTURES 1

#include <vector>

#if defined(__APPLE__) || defined(MACOSX)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

using namespace std;

typedef struct Point {
	GLfloat x;
	GLfloat y;
	GLfloat z;
	Point() { }
	Point(GLfloat x, GLfloat y, GLfloat z) : x(x), y(y), z(z) {}
} Point;

Point operator + (Point a, Point b) {
	return Point(a.x + b.x, a.y + b.y, a.z + b.z);
}

Point operator - (Point a, Point b) {
	return Point(a.x - b.x, a.y - b.y, a.z - b.z);
}

Point operator * (GLfloat s, Point a) {
	return Point(s * a.x, s * a.y, s * a.z);
}

typedef struct {
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat a;
} Color;

typedef struct {
	Point pto;
	Point normal;
	GLint ID;
	GLint valencia;
	Color color;
} Vertice;

typedef struct {
	GLint ID;
	GLint indV[3];
	Point normal;
} Face;

typedef struct {
	GLint ID;
	vector<Vertice *> vPoint;
	vector<Face *> vFace;
} Object;

typedef struct {
	GLfloat *vPoint;
	GLfloat *vNormal;
	GLfloat *vColor;
	GLfloat *vTextCoord;
	GLuint *vFace;
} ObjectVA;

struct MyVertex {
	GLfloat x, y, z;    // Vertex
	GLfloat nx, ny, nz; // Normal
	GLfloat s0, t0;     // Texcoord0
};

#endif
