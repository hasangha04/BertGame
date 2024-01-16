// Mesh.cpp - mesh operations (c) 2019-2023 Jules Bloomenthal

#include "GLXtras.h"
#include "Draw.h"
#include "Mesh.h"

// Shaders

namespace {

GLuint meshShaderLines = 0, meshShaderNoLines = 0;

// vertex shader
const char *meshVertexShader = R"(
	#version 410 core
	layout (location = 0) in vec3 point;
	layout (location = 1) in vec3 normal;
	layout (location = 2) in vec2 uv;
	layout (location = 3) in mat4 instance; // for use with glDrawArrays/ElementsInstanced
											// uses locations 3,4,5,6 for 4 vec4s = mat4
	layout (location = 7) in vec3 color;	// for instanced color (vec4?)
	out vec3 vPoint;
	out vec3 vNormal;
	out vec2 vUv;
	uniform mat4 modelview;
	uniform mat4 persp;
	uniform bool useInstance = false;
	uniform bool useNormalMatrix = false;
	uniform mat3 normalMatrix;
	void main() {
		mat4 m = useInstance? modelview*instance : modelview;
		vPoint = (m*vec4(point, 1)).xyz;
		vNormal = useNormalMatrix? normalMatrix*normal : (m*vec4(normal, 0)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vUv = uv;
	}
)";

// geometry shader
const char *meshGeometryShader = R"(
	#version 410 core
	layout (triangles) in;
	layout (triangle_strip, max_vertices = 3) out;
	in vec3 vPoint[], vNormal[];
	in vec2 vUv[];
	out vec3 gPoint, gNormal;
	out vec2 gUv;
	noperspective out vec3 gEdgeDistance;
	uniform mat4 vp;
	vec3 ViewPoint(int i) { return vec3(vp*(gl_in[i].gl_Position/gl_in[i].gl_Position.w)); }
	void main() {
		float ha = 0, hb = 0, hc = 0;
		// transform each vertex to viewport space
		vec3 p0 = ViewPoint(0), p1 = ViewPoint(1), p2 = ViewPoint(2);
		// find altitudes ha, hb, hc
		float a = length(p2-p1), b = length(p2-p0), c = length(p1-p0);
		float alpha = acos((b*b+c*c-a*a)/(2.*b*c));
		float beta = acos((a*a+c*c-b*b)/(2.*a*c));
		ha = abs(c*sin(beta));
		hb = abs(c*sin(alpha));
		hc = abs(b*sin(alpha));
		// send triangle vertices and edge distances
		for (int i = 0; i < 3; i++) {
			gEdgeDistance = i==0? vec3(ha, 0, 0) : i==1? vec3(0, hb, 0) : vec3(0, 0, hc);
			gPoint = vPoint[i];
			gNormal = vNormal[i];
			gUv = vUv[i];
			gl_Position = gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
)";

// pixel shader
const char *meshPixelShaderLines = R"(
	#version 410 core
	in vec3 gPoint, gNormal;
	in vec2 gUv;
	noperspective in vec3 gEdgeDistance;
	uniform sampler2D textureImage;
	uniform int nLights = 0;
	uniform vec3 lights[20];
	uniform vec3 defaultLight = vec3(1, 1, 1);
	uniform vec3 color = vec3(1, 0, 1);
	uniform float opacity = 1;
	uniform float ambient = .2;
	uniform bool useLight = true;
	uniform bool useTexture = true;
	uniform bool useTint = false;
	uniform bool fwdFacingOnly = false;
	uniform bool facetedShading = false;
	uniform vec4 outlineColor = vec4(0, 0, 0, 1);
	uniform float outlineWidth = 1;
	uniform float outlineTransition = 1;
	out vec4 pColor;
	float Intensity(vec3 normalV, vec3 eyeV, vec3 point, vec3 light) {
		vec3 lightV = normalize(light-point);		// light vector
		vec3 reflectV = reflect(lightV, normalV);   // highlight vector
		float d = max(0, dot(normalV, lightV));     // one-sided diffuse
		float s = max(0, dot(reflectV, eyeV));      // one-sided specular
		return clamp(d+pow(s, 50), 0, 1);
	}
	void main() {
		vec3 N = normalize(facetedShading? cross(dFdx(gPoint), dFdy(gPoint)) : gNormal);
		if (fwdFacingOnly && N.z < 0)
			discard;
		vec3 E = normalize(gPoint);					// eye vector
		float intensity = useLight? 0 : 1;
		if (useLight) {
			if (nLights == 0)
				intensity += Intensity(N, E, gPoint, defaultLight);
			else
				for (int i = 0; i < nLights; i++)
					intensity += Intensity(N, E, gPoint, lights[i]);
		}
		intensity = clamp(intensity, 0, 1);
		if (useTexture) {
			pColor = vec4(intensity*texture(textureImage, gUv).rgb, opacity);
			if (useTint) {
				pColor.r *= color.r;
				pColor.g *= color.g;
				pColor.b *= color.b;
			}
		}
		else
			pColor = vec4(intensity*color, opacity);
		float minDist = min(gEdgeDistance.x, gEdgeDistance.y);
		minDist = min(minDist, gEdgeDistance.z);
		float t = smoothstep(outlineWidth-outlineTransition, outlineWidth+outlineTransition, minDist);
		// mix edge and surface colors(t=0: edgeColor, t=1: surfaceColor)
		pColor = mix(outlineColor, pColor, t);
	}
)";

const char *meshPixelShaderNoLines = R"(
	#version 410 core
	in vec3 vPoint, vNormal;
	in vec2 vUv;
	uniform sampler2D textureImage;
	uniform int nLights = 0;
	uniform vec3 lights[20];
	uniform vec3 defaultLight = vec3(1, 1, 1);
	uniform vec3 color = vec3(1, 0, 1);
	uniform float opacity = 1;
	uniform bool useLight = true;
	uniform bool useTexture = true;
	uniform bool useTint = false;
	uniform bool fwdFacingOnly = false;
	uniform bool twoSidedShading = false;
	uniform bool facetedShading = false;
	uniform float amb = .1, dif = .7, spc =.7;		// ambient, diffuse, specular
	out vec4 pColor;
	// special for Cleave.cpp
	uniform vec4 cleaver;							// plane
	uniform bool useCleaver = false;
	float d = 0, s = 0;								// diffuse, specular terms
	vec3 N, E;
	void Intensity(vec3 light) {
		vec3 L = normalize(light-vPoint);
		float dd = dot(L, N);
		bool sideLight = dd > 0;
		bool sideViewer = gl_FrontFacing;
		if (twoSidedShading || sideLight == sideViewer) {
			d += abs(dd);
			vec3 R = reflect(L, N);					// highlight vector
			float h = max(0, dot(R, E));			// highlight term
			s += pow(h, 50);						// specular term
		}
	}
	void main() {
		N = normalize(facetedShading? cross(dFdx(vPoint), dFdy(vPoint)) : vNormal);
		if (fwdFacingOnly && N.z < 0)
			discard;
		E = normalize(vPoint);						// eye vector
		float ads = 1;
		if (useLight) {
			if (nLights == 0)
				Intensity(defaultLight);
			else
				for (int i = 0; i < nLights; i++)
					Intensity(lights[i]);
			ads = clamp(amb+dif*d, 0, 1)+spc*s;
		}
		if (useCleaver) {
			vec3 col = dot(cleaver, vec4(vPoint, 1)) < 0? vec3(1,0,0) : vec3(0,0,1);
			pColor = vec4(col, 1); // vec4(ads*col, opacity);
		}
		else if (useTexture) {
			pColor = vec4(ads*texture(textureImage, vUv).rgb, opacity);
			if (useTint) {
				pColor.r *= color.r;
				pColor.g *= color.g;
				pColor.b *= color.b;
			}
		}
		else
			pColor = vec4(ads*color, opacity);
	}
)";

} // end namespace

const char *GetMeshPixelShaderNoLines() { return meshPixelShaderNoLines; }

GLuint GetMeshShader(bool lines) {
	if (lines) {
		if (!meshShaderLines)
			meshShaderLines = LinkProgramViaCode(&meshVertexShader, NULL, NULL, &meshGeometryShader, &meshPixelShaderLines);
		return meshShaderLines;
	}
	else {
		if (!meshShaderNoLines)
			meshShaderNoLines = LinkProgramViaCode(&meshVertexShader, &meshPixelShaderNoLines);
		return meshShaderNoLines;
	}
}

GLuint UseMeshShader(bool lines) {
	GLuint s = GetMeshShader(lines);
	glUseProgram(s);
	return s;
}

// Mesh Transforms

void Mesh::SetToWorld(mat4 m) {
	toWorld = m;
	SetWrtParent();
	for (size_t i = 0; i < children.size(); i++)
		children[i]->SetToWorld();
}

void Mesh::SetToWorld() {
	// set toWorld given parent and wrtParent; recurse on children
	// see bottom of this file for alternative to wrtParent
	if (parent)
		toWorld = parent->toWorld*wrtParent;
	for (size_t i = 0; i < children.size(); i++)
		children[i]->SetToWorld();
}

void Mesh::SetWrtParent() {
	// set wrtParent such that toWorld = parent.toWorld*wrtParent
	if (parent != NULL) {
		mat4 inv = Invert(parent->toWorld);
		wrtParent = inv*toWorld;
	}
}

/* if not using wrtParent:
void RotateTransform(Mesh *m, Quaternion qrot, vec3 *center) {
	// set m.transform, recurse on m.children
	// rotate selected mesh and child meshes by qrot (returned by Arcball::Drag)
	//   apply qrot to rotation elements of m->transform (upper left 3x3)
	//   if non-null center, rotate origin of m about center
	// recursive routine initially called with null center
	Quaternion qq = m->frameDown.orientation*qrot;
		// arcball:use=Camera(?) works (qrot*m->qstart Body? fails)
	// rotate m
	qq.SetMatrix(m->toWorld, m->frameDown.scale);
	if (center) {
		// this is a child mesh: rotate origin of mesh around center
		mat4 rot = qrot.GetMatrix();
		mat4 x = Translate((*center))*rot*Translate(-(*center));
		vec4 xbase = x*vec4(m->frameDown.position, 1);
		SetMatrixOrigin(m->toWorld, vec3(xbase.x, xbase.y, xbase.z));
	}
	// recurse on children
	for (int i = 0; i < (int) m->children.size(); i++)
		RotateTransform(m->children[i], qrot, center? center : &m->frameDown.position);
			// rotate descendant children around initial mesh base
}
void TranslateTransform(Mesh *m, vec3 pDif) {
	SetMatrixOrigin(m->toWorld, m->frameDown.position+pDif);
	for (int i = 0; i < (int) m->children.size(); i++)
		TranslateTransform(m->children[i], pDif);
}
*/

// Display

// void Display(Camera camera, bool lines = false, bool useGroupColor = false);
// void Display(Camera camera, vec3 color, bool lines = false);
// void Display(Camera camera, int textureUnit, bool lines = false, bool useGroupColor = false);

void Mesh::Display(Camera camera, bool lines, bool useGroupColor) {
	Display(camera, -1, lines, useGroupColor);
}

void Mesh::Display(Camera camera, vec3 c, bool lines) {
	vec3 save = color;
	color = c;
	Display(camera, -1, lines);
	color = save;
}

void Mesh::Display(Camera camera, vec3 c, int textureUnit, bool lines) {
	vec3 save = color;
	color = c;
	Display(camera, textureUnit, lines);
	color = save;
}

void Mesh::Display(Camera camera, int textureUnit, bool lines, bool useGroupColor) {
	int nTris = triangles.size(), nQuads = quads.size();
	// enable shader and vertex array object
	int shader = UseMeshShader(lines);
	glBindVertexArray(vao);
	// texture
	bool useTexture = textureName > 0 && uvs.size() > 0 && textureUnit >= 0;
	SetUniform(shader, "useTexture", useTexture);
	if (useTexture) {
		glActiveTexture(GL_TEXTURE0+textureUnit);
		glBindTexture(GL_TEXTURE_2D, textureName);
		SetUniform(shader, "textureImage", textureUnit); // but app can unset useTexture
	}
	// set matrices
	SetUniform(shader, "modelview", camera.modelview*toWorld);
	SetUniform(shader, "persp", camera.persp);
	if (lines)
		SetUniform(shader, "vp", Viewport());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	if (useGroupColor) {
		int textureSet = 0;
		glGetUniformiv(shader, glGetUniformLocation(shader, "useTexture"), &textureSet);
		// show ungrouped triangles without texture mapping
		int nGroups = triangleGroups.size(), nUngrouped = nGroups? triangleGroups[0].startTriangle : nTris;
		SetUniform(shader, "useTexture", false);
		glDrawElements(GL_TRIANGLES, 3*nUngrouped, GL_UNSIGNED_INT, 0); // triangles.data());
		// show grouped triangles with texture mapping
		SetUniform(shader, "useTexture", textureSet == 1);
		for (int i = 0; i < nGroups; i++) {
			Group g = triangleGroups[i];
			SetUniform(shader, "color", g.color);
			glDrawElements(GL_TRIANGLES, 3*g.nTriangles, GL_UNSIGNED_INT, (void *) (3*g.startTriangle*sizeof(int)));
		}
	}
	else {
		SetUniform(shader, "color", color);
		glDrawElements(GL_TRIANGLES, 3*nTris, GL_UNSIGNED_INT, 0);
#ifdef GL_QUADS
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glDrawElements(GL_QUADS, 4*nQuads, GL_UNSIGNED_INT, quads.data());
#endif
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

// Buffering

void Enable(int id, int ncomps, int offset) {
	glEnableVertexAttribArray(id);
	glVertexAttribPointer(id, ncomps, GL_FLOAT, GL_FALSE, 0, (void *) offset);
}

void Mesh::Buffer(vector<vec3> &pts, vector<vec3> *nrms, vector<vec2> *tex) {
	size_t nPts = pts.size(), nNrms = nrms? nrms->size() : 0, nUvs = tex? tex->size() : 0;
	if (!nPts) { printf("Buffer: no points!\n"); return; }
	// create vertex buffer
	if (!vbo)
		glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// allocate GPU memory for vertex position, texture, normals
	size_t sizePoints = nPts*sizeof(vec3), sizeNormals = nNrms*sizeof(vec3), sizeUvs = nUvs*sizeof(vec2);
	int bufferSize = sizePoints+sizeUvs+sizeNormals;
	glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
	// load vertex buffer
	if (nPts) glBufferSubData(GL_ARRAY_BUFFER, 0, sizePoints, pts.data());
	if (nNrms) glBufferSubData(GL_ARRAY_BUFFER, sizePoints, sizeNormals, nrms->data());
	if (nUvs) glBufferSubData(GL_ARRAY_BUFFER, sizePoints+sizeNormals, sizeUvs, tex->data());
	// create and load element buffer for triangles
	size_t sizeTriangles = sizeof(int3)*triangles.size();
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeTriangles, triangles.data(), GL_STATIC_DRAW);
	// create vertex array object for mesh
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	// enable attributes
	if (nPts) Enable(0, 3, 0);						// VertexAttribPointer(shader, "point", 3, 0, (void *) 0);
	if (nNrms) Enable(1, 3, sizePoints);			// VertexAttribPointer(shader, "normal", 3, 0, (void *) sizePoints);
	if (nUvs) Enable(2, 2, sizePoints+sizeNormals); // VertexAttribPointer(shader, "uv", 2, 0, (void *) (sizePoints+sizeNormals));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Mesh::Clear() {
	points.resize(0);
	normals.resize(0);
	uvs.resize(0);
	triangles.resize(0);
	quads.resize(0);
	triangleGroups.resize(0);
	triangleMtls.resize(0);
}

void Mesh::Buffer() { Buffer(points, normals.size()? &normals : NULL, uvs.size()? &uvs : NULL); }

void Mesh::Set(vector<vec3> &pts, vector<vec3> *nrms, vector<vec2> *tex, vector<int> *tris, vector<int> *quas) {
	if (tris) {
		triangles.resize(tris->size()/3);
		for (int i = 0; i < (int) triangles.size(); i++)
			triangles[i] = { (*tris)[3*i], (*tris)[3*i+1], (*tris)[3*i+2] };
	}
	if (quas) {
		quads.resize(quas->size()/4);
		for (int i = 0; i < (int) quads.size(); i++)
			quads[i] = { (*quas)[4*i], (*quas)[4*i+1], (*quas)[4*i+2], (*quas)[4*i+3] };
	}
	Buffer(pts, nrms, tex);
}

// I/O

bool Mesh::Read(string objFile, mat4 *m, bool standardize, bool buffer, bool forceTriangles) {
	if (!ReadAsciiObj((char *) objFile.c_str(), points, triangles, &normals, &uvs, &triangleGroups, &triangleMtls, forceTriangles? NULL : &quads, NULL)) {
		printf("Mesh.Read: can't read %s\n", objFile.c_str());
		return false;
	}
	objFilename = objFile;
	if (standardize) {
		Standardize(points.data(), points.size(), 1);
		for (size_t i = 0; i < normals.size(); i++)
			normals[i] = normalize(normals[i]);
	}
	if (buffer)
		Buffer();
	if (m)
		toWorld = *m;
	return true;
}

bool Mesh::Read(string objFile, string texFile, mat4 *m, bool standardize, bool buffer, bool forceTriangles) {
#ifdef __APPLE__
	forceTriangles = true;
#endif
	if (!Read(objFile, m, standardize, buffer, forceTriangles))
		return false;
	objFilename = objFile;
	texFilename = texFile;
	textureName = ReadTexture((char *) texFile.c_str());
	if (!textureName)
		printf("Mesh.Read: bad texture name\n");
	return textureName > 0;
}

// Intersections / Insidedness

vec2 MajPln(vec3 &p, int mp) { return mp == 1? vec2(p.y, p.z) : mp == 2? vec2(p.x, p.z) : vec2(p.x, p.y); }

TriInfo::TriInfo(vec3 a, vec3 b, vec3 c) {
	vec3 v1(b-a), v2(c-b), x = normalize(cross(v1, v2));
	plane = vec4(x.x, x.y, x.z, -dot(a, x));
	float ax = fabs(x.x), ay = fabs(x.y), az = fabs(x.z);
	majorPlane = ax > ay? (ax > az? 1 : 3) : (ay > az? 2 : 3);
	p1 = MajPln(a, majorPlane);
	p2 = MajPln(b, majorPlane);
	p3 = MajPln(c, majorPlane);
}

QuadInfo::QuadInfo(vec3 a, vec3 b, vec3 c, vec3 d) {
	vec3 v1(b-a), v2(c-b), x = normalize(cross(v1, v2));
	plane = vec4(x.x, x.y, x.z, -dot(a, x));
	float ax = fabs(x.x), ay = fabs(x.y), az = fabs(x.z);
	majorPlane = ax > ay? (ax > az? 1 : 3) : (ay > az? 2 : 3);
	p1 = MajPln(a, majorPlane);
	p2 = MajPln(b, majorPlane);
	p3 = MajPln(c, majorPlane);
	p4 = MajPln(d, majorPlane);
}

bool LineIntersectPlane(vec3 p1, vec3 p2, vec4 plane, vec3 *intersection, float *alpha) {
  vec3 normal(plane.x, plane.y, plane.z);
  vec3 axis(p2-p1);
  float pdDot = dot(axis, normal);
  if (fabs(pdDot) < FLT_MIN)
	  return false;
  float a = (-plane.w-dot(p1, normal))/pdDot;
  if (intersection != NULL)
	  *intersection = p1+a*axis;
  if (alpha)
	  *alpha = a;
  return true;
}

static bool IsZero(float d) { return d < FLT_EPSILON && d > -FLT_EPSILON; };

int CompareVs(vec2 &v1, vec2 &v2) {
	if ((v1.y > 0 && v2.y > 0) ||           // edge is fully above query point p'
		(v1.y < 0 && v2.y < 0) ||           // edge is fully below p'
		(v1.x < 0 && v2.x < 0))             // edge is fully left of p'
		return 0;                           // can't cross
	float zcross = v2.y*v1.x-v1.y*v2.x;     // right-handed cross-product
	zcross /= length(v1-v2);
	if (IsZero(zcross) && (v1.x <= 0 || v2.x <= 0))
		return 1;                           // on or very close to edge
	if ((v1.y > 0 || v2.y > 0) && ((v1.y-v2.y < 0) != (zcross < 0)))
		return 2;                           // edge is crossed
	else
		return 0;                           // edge not crossed
}

bool IsInside(const vec2 &p, vector<vec2> &pts) {
	bool odd = false;
	int npts = pts.size();
	vec2 q = p, v2 = pts[npts-1]-q;
	for (int n = 0; n < npts; n++) {
		vec2 v1 = v2;
		v2 = pts[n]-q;
		if (CompareVs(v1, v2) == 2)
			odd = !odd;
	}
	return odd;
}

bool IsInside(const vec2 &p, const vec2 &a, const vec2 &b, const vec2 &c) {
	bool odd = false;
	vec2 q = p, v2 = c-q;
	for (int n = 0; n < 3; n++) {
		vec2 v1 = v2;
		v2 = (n==0? a : n==1? b : c)-q;
		if (CompareVs(v1, v2) == 2)
			odd = !odd;
	}
	return odd;
}

void BuildTriInfos(vector<vec3> &points, vector<int3> &triangles, vector<TriInfo> &triInfos) {
	triInfos.resize(triangles.size());
	for (size_t i = 0; i < triangles.size(); i++)
		triInfos[i] = TriInfo(points[triangles[i].i1], points[triangles[i].i2], points[triangles[i].i3]);
}

void BuildQuadInfos(vector<vec3> &points, vector<int4> &quads, vector<QuadInfo> &quadInfos) {
	quadInfos.resize(quads.size());
	for (size_t i = 0; i < quads.size(); i++)
		quadInfos[i] = QuadInfo(points[quads[i].i1], points[quads[i].i2], points[quads[i].i3], points[quads[i].i4]);
}

void Mesh::BuildInfos() {
	vec3 min, max;
	Bounds(points.data(), points.size(), min, max);
	float l = min.x, r = max.x, b = min.y, t = max.y, n = min.z, f = max.z;
	vec3 lbn(l,b,n), ltn(l,t,n), lbf(l,b,f), ltf(l,t,f), rbn(r,b,n), rtn(r,t,n), rbf(r,b,f), rtf(r,t,f);
	bounds.resize(6);
	bounds[0] = QuadInfo(lbn, ltn, ltf, lbf); // left (ccw)
	bounds[1] = QuadInfo(rbn, rbf, rtf, rtn); // right
	bounds[2] = QuadInfo(lbn, lbf, rbf, rbn); // bottom
	bounds[3] = QuadInfo(ltn, rtn, rtf, ltf); // top
	bounds[4] = QuadInfo(lbn, rbn, rtn, ltn); // near
	bounds[5] = QuadInfo(lbf, ltf, rtf, rbf); // far
	BuildTriInfos(points, triangles, triInfos);
	BuildQuadInfos(points, quads, quadInfos);
}

int IntersectWithLine(vec3 p1, vec3 p2, vector<TriInfo> &triInfos, float &retAlpha) {
	int picked = -1;
	float alpha, minAlpha = FLT_MAX;
	for (size_t i = 0; i < triInfos.size(); i++) {
		TriInfo &t = triInfos[i];
		vec3 inter;
		if (LineIntersectPlane(p1, p2, t.plane, &inter, &alpha))
			if (alpha < minAlpha) {
				if (IsInside(MajPln(inter, t.majorPlane), t.p1, t.p2, t.p3)) {
					minAlpha = alpha;
					picked = i;
				}
			}
	}
	retAlpha = minAlpha;
	return picked;
}

int IntersectWithLine(vec3 p1, vec3 p2, vector<QuadInfo> &quadInfos, float &retAlpha) {
	int picked = -1;
	float alpha, minAlpha = FLT_MAX;
	for (size_t i = 0; i < quadInfos.size(); i++) {
		QuadInfo &q = quadInfos[i];
		vec3 inter;
		if (LineIntersectPlane(p1, p2, q.plane, &inter, &alpha))
			if (alpha < minAlpha) {
				if (IsInside(MajPln(inter, q.majorPlane), q.p1, q.p2, q.p3) ||
					IsInside(MajPln(inter, q.majorPlane), q.p1, q.p3, q.p4)) {
					minAlpha = alpha;
					picked = i;
				}
			}
	}
	retAlpha = minAlpha;
	return picked;
}

bool Mesh::IntersectWithLine(vec3 p1, vec3 p2, float *alpha) {
	if (triInfos.size() == 0 && quadInfos.size() == 0)
		BuildInfos();
	float a;
	if (::IntersectWithLine(p1, p2, bounds, a) < 0)
		return false;
	if (::IntersectWithLine(p1, p2, triInfos, a) >= 0) {
		if (alpha) *alpha = a;
		return true;
	}
	if (::IntersectWithLine(p1, p2, quadInfos, a) >= 0) {
		if (alpha) *alpha = a;
		return true;
	}
	return false;
}

bool Mesh::IntersectWithSegment(vec3 p1, vec3 p2, float *alpha) {
	float a;
	if (IntersectWithLine(p1, p2, &a) && a >= 0 && a <= 1) {
		if (alpha) *alpha = a;
		return true;
	}
	return false;
}
