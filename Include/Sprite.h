// Sprite.h - 2D quad with optional texture or animation

#ifndef SPRITE_HDR
#define SPRITE_HDR

#include <glad.h>
#include <time.h>
#include <vector>
#include "VecMat.h"

using namespace std;

// Sprite Class

class ImageInfo {
public:
	GLuint textureName;
	int nChannels;
	float duration; // if animation
	ImageInfo(GLuint t = 0, int n = 0, float d = 0) : textureName(t), nChannels(n), duration(d) { }
};

class Sprite {
public:
	// display
	GLuint vao = 0;
	int winWidth = 0, winHeight = 0;
	int imgWidth = 0, imgHeight = 0;
	// geometry
	bool compensateAspectRatio = false;	// if true, adjust scale given OpenGL viewport
	float z = 0;						// in device coordinates (+/-1)
	vec2 position, scale = vec2(1, 1);
	float rotation = 0;
	mat4 ptTransform, uvTransform;
	// for collision:
	int id = 0;
	vector<int> collided;
	// single image
	int nTexChannels = 0;
	GLuint textureName = 0, matName = 0;
	// multiple images
	int frame = 0, nFrames = 0;
	bool autoAnimate = true;
	vector<ImageInfo> images;
	time_t change;
	// mouse
	vec2 mouseDown, oldMouse;
	// methods
	void SetFrame(int n);
	bool Intersect(Sprite &s);
	void UpdateTransform();
	vec2 GetScale();
	void SetScale(vec2 s);
	vec2 PtTransform(vec2 p);
	mat4 GetPtTransform();
	void SetPtTransform(mat4 m);
	void SetUvTransform(mat4 m);
	// initialization, release
	void Initialize(GLuint texName, float z = 0);
	void Initialize(string imageFile, float z = 0);
	void Initialize(string imageFile, string matFile, float z = 0);
	void Initialize(vector<string> &imageFiles, string matFile, float z = 0, float frameDuration = 1);
	void InitializeGIF(string gifFile, float z = 0);
	void Release();
	// orientation
	void SetRotation(float angle);
	void SetPosition(vec2 p);
	vec2 GetPosition();
	// mouse handlers
	void Down(double x, double y);
	vec2 Drag(double x, double y);
	void Wheel(double spin);
	bool Hit(double x, double y);
	// display
	void Display(mat4 *view = NULL, int textureUnit = 0);
		// if view NULL, space presumed NDC (+/-1)
	void Outline(vec3 color, float width = 2);
	void SetFrameDuration(float dt);	// if animating
	Sprite(vec2 p = vec2(), float s = 1, bool invVrt = true) : position(p), scale(vec2(s, s)) {  } // UpdateTransform(); }
	Sprite(vec2 p, vec2 s, bool invVrt = true) : position(p), scale(s) { } // UpdateTransform(); }
	~Sprite() { Release(); }
};

int TestCollisions(vector<Sprite *> &sprites);
	// return #pixels overlap of sprites

#endif
