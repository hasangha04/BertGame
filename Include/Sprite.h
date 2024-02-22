// Sprite.h - 2D quad with optional texture or animation

#ifndef SPRITE_HDR
#define SPRITE_HDR

#include <glad.h>
#include <time.h>
#include <vector>
#include "VecMat.h"

using namespace std;

// Support

struct ImageInfo {
	ImageInfo(GLuint t = 0, int n = 0, float d = 0) :
		textureName(t), nChannels(n), duration(d) {
	}
	GLuint textureName;								// OpenGL sampler2D ID
	int nChannels;									// bw, rgb, rgba
	float duration;									// in seconds (if animation)
};

typedef vector<ImageInfo> ImageInfos;
typedef vector<int> Ints;

// Sprite Class

class Sprite {
public:
	// display
	GLuint		vao = 0;							// vertex array buffer (but no vertex buffer)
	int			imgWidth = 0, imgHeight = 0;		// image size
	// geometry
	bool		compensateAspectRatio = true;		// if true, adjust scale wrt window aspect ratio
	float		z = 0;								// in device coordinates (+/-1, depends on GL_DEPTH_RANGE)
	vec2		position;							// in NDC (+/-1 coords)
	vec2		scale = vec2(1, 1);
	float		rotation = 0;						// in degrees
	mat4		ptTransform;						// based on position, scale, rotation
	mat4		uvTransform;						// transform texture
	// single image
	int			nTexChannels = 0;
	GLuint		textureName = 0, matName = 0;
	// multiple images
	ImageInfos	images;
	int			frame = 0, nFrames = 0;
	bool		autoAnimate = true;					// if true and multiple images, advance frame
	time_t		change;
	// mouse
	vec2		mouseDown, oldMouse;
	// pixel/pixel collision
	int			id = 0;
	Ints		collided;
	// initialization
	void Initialize(string imageFile, float z = 0, bool compensateAspectRatio = true);
	void Initialize(string imageFile, string matFile, float z = 0);
	void Initialize(vector<string> &imageFiles, string matFile, float z = 0, float frameDuration = 1);
	void Initialize(GLuint texName, float z = 0);
	void InitializeGIF(string gifFile, float z = 0);
	// transformation
	void UpdateTransform();							// compute .ptTransform given scale, rotation, position
	vec2 PtTransform(vec2 p);						// return p transformed by ptTransform
	void SetPtTransform(mat4 m);					// override UpdateTransform
	void SetUvTransform(mat4 m);					// set texture transform
	// geometry
	void SetScale(vec2 s);
	void SetRotation(float angle);					// in degrees ccw
	void SetPosition(vec2 p);						// p in +/-1 coords
	void SetScreenPosition(int x, int y);			// p (x,y) in pixel coords
	vec2 GetScreenPosition();						// return .position in pixel coords
	bool Intersect(Sprite &s);						// simple bounding-box test
	// mouse
	void Down(double x, double y);					// x,y in pixels
	vec2 Drag(double x, double y);					// move; x,y in pixels
	void Wheel(double spin, bool scale = true);		// scale or rotate
	bool Hit(double x, double y);					// test z-buf (if avail) or bounding-box; x,y in pixels
	// display
	void Display(mat4 *view = 0, int texUnit = 0);	// if view NULL, space presumed NDC (+/-1)
	void Outline(vec3 color, float width = 2);		// draw bounding-box
	// animation
	void SetFrame(int n);
	void SetFrameDuration(float dt);				// dt in seconds
	// constructors, destructors
	void Release();									// free image buffers
	Sprite(vec2 p = vec2(), float s = 1) : position(p), scale(vec2(s, s)) {  }
	Sprite(vec2 p, vec2 s) : position(p), scale(s) { }
	~Sprite() { Release(); }
};

int TestCollisions(vector<Sprite *> &sprites);
	// return #pixels overlap of sprites

#endif
