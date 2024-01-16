// IO.h - read image files, read/write STL, read/write OBJ (c) 2019-2023 Jules Bloomenthal

#ifndef IO_HDR
#define IO_HDR

#include <glad.h>
#include <string.h>
#include <vector>
#include "Camera.h"
#include "VecMat.h"

using std::string;
using std::vector;

// Texture

GLuint ReadTexture(const char *filename, bool mipmap = true, int *nchannels = NULL, int *width = NULL, int *height = NULL);
	// load image file; return texture name

GLuint LoadTexture(unsigned char *pixels, int width, int height, int bpp, bool bgr = false, bool mipmap = true);
	// load pixels (bpp: bytes per pixel), return texture name

void LoadTexture(unsigned char *pixels, int width, int height, int bpp, unsigned int textureName, bool bgr, bool mipmap = true);

void SavePng(const char *filename);

void SaveBmp(const char *filename);

void SaveTga(const char *filename);

int ReadGIF(const char *filename, vector<GLuint> &textureNames, int *nChannels = NULL, vector<float> *frameDurations = NULL);
	// return #frames successfully read
	// if non-null, set nChannels (bytes/pixel), set frameDurations

// Buffer to GPU
//    GLuint int textureName;
//    glGenTextures(1, &textureName);
//    glBindTexture(GL_TEXTURE_2D, textureName); 
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,  GL_RGB, GL_UNSIGNED_BYTE, pixels);
// Display
//    int textureUnit = 0;							// arbitrary
//    glActiveTexture(GL_TEXTURE0+textureUnit);
//    glBindTexture(GL_TEXTURE_2D, textureName);	// bind GPU buffer to active texture unit
//    SetUniform("textureImage", textureUnit);
// In shader
//    uniform sampler2D textureImage;
//    vec4 rgba = texture(textureImage, uv);
// In main
//	  textureName = ReadTexture(<filename>);

// Standardize

mat4 StandardizeMat(vec3 *points, int npoints, float scale = 1);

void Standardize(vec3 *points, int npoints, float scale = 1);
	// translate and apply uniform scale so that vertices fit in -scale,+scale in X,Y,Z

// Normals

void SetVertexNormals(vector<vec3> &points, vector<int3> &triangles, vector<vec3> &normals);
	// compute/recompute vertex normals as the average of surrounding triangle normals

// STL

struct VertexSTL {
	vec3 point, normal;
	VertexSTL() { }
	VertexSTL(float *p, float *n) : point(vec3(p[0], p[1], p[2])), normal(vec3(n[0], n[1], n[2])) { }
};

int ReadSTL(const char *filename, vector<VertexSTL> &vertices);
	// binary format only
	// read vertices from file, three per triangle; return # triangles

bool ReadSTL(const char *filename, vector<vec3> &points, vector<vec3> &normals, vector<int3> &triangles);
	// must be binary file

// OBJ

struct Group {
	string name;
	int startTriangle = 0, nTriangles = 0;
	//? mat4 transform;
	//? string parent;
	//? Group *children;
	//? bool smooth = true;
	//? char *material;
	vec3 color = vec3(1, 1, 1);
	Group(int start = 0, string n = "", vec3 c = vec3(1, 1, 1)) : startTriangle(start), name(n), color(c) { }
};

struct Mtl {
	string name;
	vec3 ka, kd, ks;
	int startTriangle = 0, nTriangles = 0;
	Mtl() {startTriangle = -1, nTriangles = 0; }
	Mtl(int start, string n, vec3 a, vec3 d, vec3 s) : startTriangle(start), name(n), ka(a), kd(d), ks(s) { }
};

bool ReadAsciiObj(const char    *filename,                  // must be ASCII file
				  vector<vec3>  &points,                    // unique set of points determined by vertex/normal/uv triplets in file
				  vector<int3>  &triangles,                 // array of triangle vertex ids
				  vector<vec3>  *normals  = NULL,           // if non-null, read normals from file, correspond with points
				  vector<vec2>  *textures = NULL,           // if non-null, read uvs from file, correspond with points
				  vector<Group> *triangleGroups = NULL,     // correspond with triangle groups
				  vector<Mtl>   *triangleMtls = NULL,		// correspond with triangle groups
				  vector<int4>  *quads = NULL,              // optional quadrilaterals
				  vector<int2>  *segs = NULL);				// optional line segments
	// set points and triangles; normals, textures
	// if quads == NULL, then any file quad is converted to triangles
	// return true if successful

bool WriteAsciiObj(const char      *filename,
				   vector<vec3>    &points,
				   vector<vec3>    &normals,
				   vector<vec2>    &uvs,
				   vector<int3>    *triangles = NULL,
				   vector<int4>    *quads = NULL,
				   vector<int2>    *segs = NULL,
				   vector<Group>   *triangleGroups = NULL);
	// write to file mesh points, normals, and uvs
	// optionally write triangles, quads, segs, groups

#endif
