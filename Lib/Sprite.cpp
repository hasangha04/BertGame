// Sprite.cpp

#include "Draw.h"
#include "GLXtras.h"
#include "IO.h"
#include "Sprite.h"
#include <algorithm>
#include <iostream>

// Shader storage buffers for collision tests
GLuint occupyBinding = 11, collideBinding = 12;
GLuint occupyBuffer = 0, collideBuffer = 0;

// Shaders
GLuint spriteShader = 0, spriteCollisionShader = 0;

namespace SpriteSpace {

int BuildSpriteShader(bool collisionTest = false) {
	const char *vShader = R"(
		#version 330
		uniform mat4 view;
		uniform float z = 0;
		out vec2 uv;
		void main() {
			// works for 1 quad or 2 tris
			const vec2 pts[6] = vec2[6](vec2(-1,-1), vec2(1,-1), vec2(1,1), vec2(-1,1), vec2(-1,-1), vec2(1,1));
			uv = (vec2(1,1)+pts[gl_VertexID])/2;
			gl_Position = view*vec4(pts[gl_VertexID], z, 1);
		}
	)";
	const char *pShader = R"(
		#version 330
		in vec2 uv;
		out vec4 pColor;
		uniform mat4 uvTransform;
		uniform sampler2D textureImage, textureMat;
		uniform bool useMat;
		uniform int nTexChannels = 3;
		void main() {
			vec2 st = (uvTransform*vec4(uv, 0, 1)).xy;
			if (nTexChannels == 4)
				pColor = texture(textureImage, st);
			else {
				pColor.rgb = texture(textureImage, st).rgb;
				pColor.a = useMat? texture(textureMat, st).r : 1;
			}
			if (pColor.a < .02) // if nearly full matte,
				discard;		// don't tag z-buffer
		}
	)";
	const char *pCollisionShader = R"(
		#version 430
		layout(binding = 11, std430) buffer Occupy  { int occupy[]; };		// set occupy[x][y] to sprite id
		layout(binding = 12, std430) buffer Collide { int collide[]; };		// does spriteId collide with spriteN?
		layout(binding = 0, r32ui) uniform uimage1D atomicCollide;			// does spriteId collide with spriteN?
		layout(binding = 0, offset = 0) uniform atomic_uint counter;
		in vec2 uv;
		out vec4 pColor;
		uniform vec4 vp;
		uniform bool showOccupy = false, useMat = false;
		uniform sampler2D textureImage, textureMat;
		uniform mat4 uvTransform;
		uniform int spriteId = 0, nTexChannels = 3;
		void main() {
			vec2 st = (uvTransform*vec4(uv, 0, 1)).xy;
			if (nTexChannels == 4)
				pColor = texture(textureImage, st);
			else {
				pColor.rgb = texture(textureImage, st).rgb;
				pColor.a = useMat? texture(textureMat, st).r : 1;
			}
			if (pColor.a < .02) // if nearly full matte, don't tag z-buffer
				discard;
			if (pColor.a >= .02) {
				vec3 cols[] = vec3[](vec3(1,0,0),vec3(1,1,0),vec3(0,1,0),vec3(0,0,1));
				int id = int((gl_FragCoord.y-vp[1])*vp[2]+gl_FragCoord.x-vp[0]);
				int o = occupy[id];
				if (o > -1) {
					collide[o] = 1;
					atomicCounterIncrement(counter);
					if (showOccupy)
						pColor = vec4(cols[o], 1);
				}
				occupy[id] = spriteId;
			}
		}
	)";
	return LinkProgramViaCode(&vShader, collisionTest? &pCollisionShader : &pShader);
}

GLuint GetShader() {
	if (!spriteShader)
		spriteShader = BuildSpriteShader();
	return spriteShader;
}

GLuint GetCollisionShader() {
	if (!spriteCollisionShader)
		spriteCollisionShader = BuildSpriteShader(true);
	return spriteCollisionShader;
}

bool CrossPositive(vec2 a, vec2 b, vec2 c) { return cross(vec2(b-a), vec2(c-b)) > 0; }

} // end namespace

// Collision

vector<int> clearOccupy, clearCollide;
int nCollisionSprites = 0;
GLuint spriteCountersBuf = 0;

void ResetCounter() {
	GLuint count = 0;
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, spriteCountersBuf);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, spriteCountersBuf);
	glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &count);
}

int ReadCounter() {
	GLuint count = 0;
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, spriteCountersBuf);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, spriteCountersBuf);
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &count);
	return count;
}

void InitCollisionShaderStorage(int nsprites) {
	vec4 vp = VP();
	int w = (int) vp[2], h = (int) vp[3];
	// occupancy buffer
	clearOccupy.assign(w*h, -1);
	glGenBuffers(1, &occupyBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, occupyBinding, occupyBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, w*h*sizeof(int), clearOccupy.data(), GL_DYNAMIC_DRAW);
	// collision buffer
	clearCollide.assign(nsprites, -1);
	glGenBuffers(1, &collideBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, collideBinding, collideBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, nsprites*sizeof(int), clearCollide.data(), GL_DYNAMIC_DRAW);
	// atomic counters
	glGenBuffers(1, &spriteCountersBuf);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, spriteCountersBuf);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, spriteCountersBuf);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, 0);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

void ClearCollide() {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, collideBinding, collideBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, clearCollide.size()*sizeof(int), clearCollide.data());
}

void ClearOccupyAndCounter(int nsprites) {
	int w = VPw(), h = VPh();
	// occupancy buffer
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, occupyBinding, occupyBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, w*h*sizeof(int), clearOccupy.data());
	// collision buffer
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, collideBinding, collideBuffer);
//	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, clearCollide.size()*sizeof(int), clearCollide.data());
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, nsprites*sizeof(int), clearCollide.data());
	// atomic counter
	ResetCounter();
}

void GetCollided(int nsprites, Sprite *s) {
	if ((int) s->collided.size() < nsprites)
		s->collided.resize(nsprites);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, nsprites*sizeof(int), s->collided.data());
}

bool ZCompare(Sprite *s1, Sprite *s2) { return s1->z > s2->z; }

int TestCollisions(vector<Sprite *> &sprites) {
	int nsprites = sprites.size();
	if (nsprites != nCollisionSprites) {
		nCollisionSprites = nsprites;
		InitCollisionShaderStorage(nsprites);
	}
//	else
		ClearOccupyAndCounter(nsprites);
	vector<Sprite *> tmp = sprites;
	for (int i = 0; i < nsprites; i++)
		tmp[i]->id = i;
	sort(tmp.begin(), tmp.end(), ZCompare);
	GLuint program = SpriteSpace::GetCollisionShader();
	glUseProgram(program);
	vec4 vp = VP();
	SetUniform(program, "vp", vp);
	SetUniform(program, "showOccupy", true);
	// display in descending z order
	for (int i = 0; i < nsprites; i++) {
		Sprite *s = tmp[i];
		SetUniform(program, "spriteId", s->id);
		ClearCollide();
		s->Display();
		GetCollided(nsprites, s);
	}
	SetUniform(program, "showOccupy", false);
	UseDrawShader(ScreenMode());
	glUseProgram(0);
	return ReadCounter();
}

bool Intersect(mat4 m1, mat4 m2) {
	vec2 pts[] = { {-1,-1}, {-1,1}, {1,1}, {1,-1} };
	float x1min = FLT_MAX, x1max = -FLT_MAX, y1min = FLT_MAX, y1max = -FLT_MAX;
	float x2min = FLT_MAX, x2max = -FLT_MAX, y2min = FLT_MAX, y2max = -FLT_MAX;
	for (int i = 0; i < 4; i++) {
		vec2 p1 = Vec2(m1*vec4(pts[i], 0, 1));
		vec2 p2 = Vec2(m2*vec4(pts[i], 0, 1));
		x1min = min(x1min, p1.x); x1max = max(x1max, p1.x);
		y1min = min(y1min, p1.y); y1max = max(y1max, p1.y);
		x2min = min(x2min, p2.x); x2max = max(x2max, p2.x);
		y2min = min(y2min, p2.y); y2max = max(y2max, p2.y);
	}
	bool xNoOverlap = x1min > x2max || x2min > x1max;
	bool yNoOverlap = y1min > y2max || y2min > y1max;
	return !xNoOverlap && !yNoOverlap;
}

bool Sprite::Intersect(Sprite &s) {
	return ::Intersect(ptTransform, s.ptTransform);
}

void Sprite::Initialize(GLuint texName, float z) {
	this->z = z;
	textureName = texName;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	UpdateTransform();
}

void Sprite::Initialize(string imageFile, float z) {
	this->z = z;
	textureName = ReadTexture(imageFile.c_str(), true, &nTexChannels, &imgWidth, &imgHeight);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	UpdateTransform();
}

void Sprite::Initialize(string imageFile, string matFile, float z) {
	Initialize(imageFile, z);
	matName = ReadTexture(matFile.c_str());
}

void Sprite::Initialize(vector<string> &imageFiles, string matFile, float z, float frameDuration) {
	this->z = z;
	nFrames = imageFiles.size();
	images.resize(nFrames);
	for (int i = 0; i < nFrames; i++) {
		int nTexChannels;
		GLuint textureName = ReadTexture(imageFiles[i].c_str(), true, &nTexChannels);
		images[i] = ImageInfo(textureName, nTexChannels, frameDuration);
	}
	if (!matFile.empty())
		matName = ReadTexture(matFile.c_str());
	change = clock()+(time_t)(frameDuration*CLOCKS_PER_SEC);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	UpdateTransform();
}

void Sprite::InitializeGIF(string gifFile, float z) {
	int nChannels = 0;
	vector<GLuint> textureNames;
	vector<float> frameDurations;
	nFrames = ReadGIF(gifFile.c_str(), textureNames, &nChannels, &frameDurations);
	images.resize(nFrames);
	for (int i = 0; i < nFrames; i++)
		images[i] = ImageInfo(textureNames[i], nChannels, frameDurations[i]);
	UpdateTransform();
}

bool Sprite::Hit(double x, double y) {
	// test against z-buffer
	float depth;
	if (DepthXY((int) x, (int) y, depth))
		return abs(depth-z) < .01;
	// test against quad
	ViewportSize(winWidth, winHeight);
	vec2 test((2.f*x)/winWidth-1, (2.f*y)/winHeight-1);
	vec2 xPts[] = { PtTransform({-1,-1}), PtTransform({-1,1}), PtTransform({1,1}), PtTransform({1,-1}) };
	for (int i = 0; i < 4; i++)
		if (SpriteSpace::CrossPositive(test, xPts[i], xPts[(i+1)%4]))
			return false;
	return true;
}

void Sprite::SetRotation(float angle) { rotation = angle; UpdateTransform(); }

void Sprite::SetPosition(vec2 p) { position = p; UpdateTransform(); }

vec2 Sprite::GetPosition() { return position; }

void Sprite::UpdateTransform() {
	if (compensateAspectRatio) {
		int vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		float aspectRatio = (float)vp[2]/(float)vp[3];
		ptTransform = Translate(position.x, position.y, 0)*Scale(scale.x/aspectRatio, scale.y, 1)*RotateZ(rotation);
	}
	else
		ptTransform = Translate(position.x, position.y, 0)*Scale(scale.x, scale.y, 1)*RotateZ(rotation);
}

vec2 Sprite::PtTransform(vec2 p) {
	vec4 q = ptTransform*vec4(p, 0, 1);
	return vec2(q.x, q.y); // /q.w
}

void Sprite::Down(double x, double y) {
	oldMouse = position;
	ViewportSize(winWidth, winHeight);
	mouseDown = vec2((float) x, (float) y);
}

vec2 Sprite::Drag(double x, double y) {
	vec2 dif(x-mouseDown.x, y-mouseDown.y);
	vec2 difScale(2*dif.x/winWidth, 2*dif.y/winHeight);
	SetPosition(oldMouse+difScale);
	return difScale;
}

void Sprite::Wheel(double spin) {
	scale += .1f*(float) spin;
	scale.x = scale.x < FLT_MIN? FLT_MIN : scale.x;
	scale.y = scale.y < FLT_MIN? FLT_MIN : scale.y;
	UpdateTransform();
}

vec2 Sprite::GetScale() { return scale; }

void Sprite::SetScale(vec2 s) {
	scale = s;
	UpdateTransform();
}

mat4 Sprite::GetPtTransform() { return ptTransform; }

void Sprite::SetPtTransform(mat4 m) { ptTransform = m; }

void Sprite::SetUvTransform(mat4 m) { uvTransform = m; }

int GetSpriteShader() {
	if (!spriteShader) SpriteSpace::BuildSpriteShader();
	return spriteShader;
}

void Sprite::Outline(vec3 color, float width) {
	UseDrawShader(mat4());
	vec2 pts[] = { PtTransform({-1,-1}), PtTransform({-1,1}), PtTransform({1,1}), PtTransform({1,-1}) };
	for (int i = 0; i < 4; i++)
		Line(pts[i], pts[(i+1)%4], width, color);
}

void Sprite::SetFrame(int n) {
	ImageInfo i = images[frame = n];
	textureName = i.textureName;
	nTexChannels = i.nChannels;
}

void Sprite::Display(mat4 *fullview, int textureUnit) {
	int s = CurrentProgram();
	glBindVertexArray(vao);
	if (s <= 0 || (s != spriteShader && s != spriteCollisionShader))
		s = SpriteSpace::GetShader();
	glUseProgram(s);
	glActiveTexture(GL_TEXTURE0+textureUnit);
	if (nFrames && autoAnimate) {
		time_t now = clock();
		ImageInfo i = images[frame];
		if (now > change) {
			frame = (frame+1)%nFrames;
			change = now+(time_t)(i.duration*CLOCKS_PER_SEC);
		}
		glBindTexture(GL_TEXTURE_2D, i.textureName);
		SetUniform(s, "nTexChannels", i.nChannels);
	}
	else {
		glBindTexture(GL_TEXTURE_2D, textureName);
		SetUniform(s, "nTexChannels", nTexChannels);
	}
	SetUniform(s, "textureImage", (int) textureUnit);
	SetUniform(s, "useMat", matName > 0);
	SetUniform(s, "z", z);
	if (matName > 0) {
		glActiveTexture(GL_TEXTURE0+textureUnit+1);
		glBindTexture(GL_TEXTURE_2D, matName);
		SetUniform(s, "textureMat", (int) textureUnit+1);
	}
	SetUniform(s, "view", fullview? *fullview*ptTransform : ptTransform);
	SetUniform(s, "uvTransform", uvTransform);
#ifdef GL_QUADS
	glDrawArrays(GL_QUADS, 0, 4);
#else
	glDrawArrays(GL_TRIANGLES, 0, 6);
#endif
}

void Sprite::SetFrameDuration(float dt) {
	for (ImageInfo i : images)
		i.duration = dt;
}

void Sprite::Release() {
	glDeleteBuffers(1, &textureName);
	if (matName > 0) glDeleteBuffers(1, &matName);
}
