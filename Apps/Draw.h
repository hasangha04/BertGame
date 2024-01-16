// Draw.h (c) 2019-2022 Jules Bloomenthal
// as of OpenGLv3.1, LINE_STIPPLE and glLineStipple are deprecated (maybe available in compatibility profile)

#ifndef DRAW_HDR
#define DRAW_HDR

#include <glad.h>
#include "VecMat.h"

// viewport operations
void ViewportSize(int &width, int &height);
vec4 VP();
int4 VPi();	// as above, but integer
int VPw();	// viewport width
int VPh();	// viewport height
mat4 Viewport();
	// matrix to map NDC space to pixel space, inverse of ScreenMode

// screen operations
mat4 ScreenMode();
	// create matrix to map pixel space, (xorigin, yorigin)-(xorigin+width,yorigin+height), to NDC (clip) space, (-1,-1)-(1,1)
bool IsVisible(vec3 p, mat4 fullview, vec2 *screen = NULL, int *w = NULL, int *h = NULL, float fudge = 0);
	// if the depth test is enabled, is point p visible?
	// if non-null, set screen location (in pixels) of transformed p
	// **** this is slow when used during rendering!
bool DepthXY(int x, int y, float &depth);
	// return false if depth-buffer disabled, else
	// return true and set depth to z-value at pixel(x,y)
	// normalized for +/-1 space
// ScreenPoint, ScreenD require appropriate viewport
vec2 ScreenPoint(vec3 p, mat4 m, int vp[4], float *zscreen = NULL);
vec2 ScreenPoint(vec3 p, mat4 m, float *zscreen = NULL);
	// transform 3D point to location (xscreen, yscreen), in pixels; if non-null, set zscreen
	// uses current GL viewport and presumes returned y increases upwards
void ScreenRay(float xscreen, float yscreen, mat4 modelview, mat4 persp, vec3 &p, vec3 &v);
void ScreenLine(float xscreen, float yscreen, mat4 modelview, mat4 persp, vec3 &p1, vec3 &p2);
	// compute 3D world space line, given by p1 and p2, that transforms
	// to a line perpendicular to the screen at pixel (xscreen, yscreen)
	// uses current viewport
float ScreenD(double x, double y, vec3 p, mat4 m, float *zscreen = NULL);
float ScreenD(int x, int y, vec3 p, mat4 m, float *zscreen = NULL);
float ScreenDSq(double x, double y, vec3 p, mat4 m, float *zscreen = NULL);
	// invert if +y is down (eg, mouse); do not invert for pixel space (+y is up)
float ScreenDSq(int x, int y, vec3 p, mat4 m, float *zscreen = NULL);
	// return distance squared, in pixels, between screen point (x, y) and point p xformed by view matrix
	// y presumed in original screen space (increasing y is downwards)
double ScreenZ(vec3 p, mat4 m);
bool FrontFacing(vec3 base, vec3 vec, mat4 view);

// 2D/3D drawing operations

mat4 GetDrawView();
void SetDrawView(mat4 m);

GLuint GetDrawShader();
GLuint GetTriangleShader();
GLuint GetCylinderShader();

GLuint UseDrawShader();
	// invoke shader for Disk, Line, Quad, and Arrow, but do not change view transformation
	// return previous shader ID
GLuint UseDrawShader(mat4 viewMatrix);
	// as above, but set view transformation

void Disk(vec2 p, float diameter, vec3 color, float opacity = 1, bool ring = false);
void Disk(vec3 p, float diameter, vec3 color, float opacity = 1, bool ring = false);
void Line(vec3 p1, vec3 p2, float width, vec3 col, float opacity = 1);
void Line(vec3 p1, vec3 p2, float width, vec3 col1, vec3 col2, float opacity = 1);
void Line(vec2 p1, vec2 p2, float width, vec3 col, float opacity = 1);
void Line(vec2 p1, vec2 p2, float width, vec3 col1, vec3 col2, float opacity = 1);
void Line(int x1, int y1, int x2, int y2, float width, vec3 col, float opacity = 1);
void LineDash(vec3 p1, vec3 p2, float width, vec3 col1, vec3 col2, float opacity = 1, float dashLen = 20, float percentDash = .5);
void LineDot(vec3 p1, vec3 p2, float width, vec3 col, float opacity = 1, int pixelSpacing = 7);
void LineStrip(int nPoints, vec3 *points, vec3 &color, float opacity, float width);
void Quad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, bool solid, vec3 color, float opacity = 1, float lineWidth = 1);
void Quad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, bool solid, vec3 color, float opacity = 1, float lineWidth = 1);
void Quad(vec3 p1, vec3 p2, vec3 p3, vec3 p4, bool solid, vec3 color, float opacity = 1, float lineWidth = 1);
void Quad(vec3 p1, vec3 p2, vec3 p3, vec3 p4, GLuint textureName, int textureUnit, int nChannels = 3);
	// render image if textureUnit >= 0
void Star(vec3 p, float size, vec3 color);
void Star(vec3 p, float size, vec3 color, vec3 colorHidden);
void Arrow(vec2 base, vec2 head, vec3 color, float lineWidth = 1, double headSize = 4);
	// display an arrow between base and head
void ArrowV(vec3 base, vec3 v, mat4 modelview, mat4 persp, vec3 color, float lineWidth = 1, double headSize = 4);
	// as above but vector and base are 3D, transformed by m
void Cylinder(vec3 p1, vec3 p2, float r1, float r2, mat4 modelview, mat4 persp, vec4 color);
	// p1 and p2 specify x,y,z for cylinder endpoints, and w for radius

// triangle operations
GLuint UseTriangleShader();
GLuint UseTriangleShader(mat4 viewMatrix);
void Triangle(vec3 p1, vec3 p2, vec3 p3, vec3 c1, vec3 c2, vec3 c3,
			  float opacity = 1, bool outline = false,
			  vec4 outlineCol = vec3(0,0,0), float outlineWidth = 1, float transition = 1);

void Box(vec3 a, vec3 b, float width, vec3 col);

#endif
