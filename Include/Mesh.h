// Mesh.h - 3D mesh of triangles (c) 2019-2022 Jules Bloomenthal

#ifndef MESH_HDR
#define MESH_HDR

#include <vector>
#include "glad.h"
#include "Camera.h"
#include "IO.h"
#include "Quaternion.h"
#include "VecMat.h"

using std::string;
using std::vector;

// Shader Programs

GLuint GetMeshShader(bool lines = false);
GLuint UseMeshShader(bool lines = false);
	// lines true uses geometry shader to draw lines along triangle edges
	// lines false is slightly more efficient

const char *GetMeshPixelShaderNoLines();

struct TriInfo {
	vec4 plane;
	int majorPlane = 0; // 0: XY, 1: XZ, 2: YZ
	vec2 p1, p2, p3;    // vertices projected to majorPlane
	TriInfo() { };
	TriInfo(vec3 p1, vec3 p2, vec3 p3);
};

struct QuadInfo {
	vec4 plane;
	int majorPlane = 0;
	vec2 p1, p2, p3, p4;
	QuadInfo() { };
	QuadInfo(vec3 p1, vec3 p2, vec3 p3, vec3 p4);
};

// Mesh Class and Operations

class Mesh {
public:
	Mesh() { };
	Mesh(const char *filename) { Read(string(filename)); }
	~Mesh() { if (vbo > 0) glDeleteBuffers(1, &vbo); };
	string objFilename, texFilename;
	// vertices and facets
	vector<vec3>	points;
	vector<vec3>	normals;
	vector<vec2>	uvs;
	vector<int3>	triangles;
	vector<int4>	quads;
	// ancillary data
	vector<Group>	triangleGroups;
	vector<Mtl>		triangleMtls;
	// position/orientation
	vec3			centerOfRotation;
	mat4			toWorld;		// view = camera.modelview*toWorld
	mat4			wrtParent;		// toWorld = parent->toWorld*wrtParent
	// hierarchy
	Mesh		   *parent = NULL;
	vector<Mesh *>	children;
	// GPU vertex buffer and texture
	GLuint			vao = 0;		// vertex array object
	GLuint			vbo = 0;		// vertex buffer]
	GLuint			ebo = 0;		// element (triangle) buffer
	// texture, color
	GLuint			textureName = 0;
	vec3			color = vec3(1, 1, 1);
	// intersection
	vector<TriInfo> triInfos;
	vector<QuadInfo> quadInfos;
	vector<QuadInfo> bounds;
	// operations
	void Clear();
	void Buffer();
	void Buffer(vector<vec3> &pts, vector<vec3> *nrms = NULL, vector<vec2> *uvs = NULL);
		// if non-null, nrms and uvs assumed same size as pts
	void Set(vector<vec3> &pts, vector<vec3> *nrms = NULL, vector<vec2> *tex = NULL,
			 vector<int> *tris = NULL, vector<int> *quads = NULL);
	void SetToWorld();
		// for this mesh set toWorld given parent and wrtParent; recurse on children
	void SetToWorld(mat4 m);
		// as above but first assigning toWorld
	void SetWrtParent();
		// for this mesh set wrtParent given parent and toWorld
	void Display(Camera camera, bool lines = false, bool useGroupColor = false);
		// display with assigned color
	void Display(Camera camera, vec3 color, bool lines = false);
		// display with given color
	void Display(Camera camera, vec3 color, int textureUnit, bool lines = false);
		// display with given color and texture - used primarily for tinting the texture by the color
	void Display(Camera camera, int textureUnit, bool lines = false, bool useGroupColor = false);
		// texture is enabled if textureUnit >= 0 and textureName set
		// before this call, app can optionally change uniforms from their default, including:
		//     nLights, lights, color, opacity, ambient
		//     useLight, useTint, fwdFacingOnly, facetedShading
		//     outlineColor, outlineWidth, transition
		// see Mesh.cpp pixel shader uniform inputs for complete list
	bool Read(string objFile, mat4 *m = NULL, bool standardize = true, bool buffer = true, bool forceTriangles = false);
		// read in object file (with normals, uvs), initialize matrix, build vertex buffer
	bool Read(string objFile, string texFile, mat4 *m = NULL, bool standardize = true, bool buffer = true, bool forceTriangles = false);
		// read in object file (with normals, uvs) and texture file, initialize matrix, build vertex buffer
	void BuildInfos();
	bool IntersectWithLine(vec3 p1, vec3 p2, float *alpha = NULL);
	bool IntersectWithSegment(vec3 p1, vec3 p2, float *alpha = NULL);
		// as above but true if 0 <= alpha <= 1
};

// Intersections

void BuildTriInfos(vector<vec3> &points, vector<int3> &triangles, vector<TriInfo> &triInfos);
	// for interactive selection

void BuildQuadInfos(vector<vec3> &points, vector<int4> &quads, vector<QuadInfo> &quadiInfos);

int IntersectWithLine(vec3 p1, vec3 p2, vector<TriInfo> &triInfos, float &alpha);
	// return triangle index of nearest intersected triangle, or -1 if none
	// intersection = p1+alpha*(p2-p1)

int IntersectWithLine(vec3 p1, vec3 p2, vector<QuadInfo> &quadInfos, float &alpha);

#endif
