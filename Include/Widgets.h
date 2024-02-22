// Widgets.h: Cursor Support, Mover, Arcball, Framer, Joystick, Button, Toggler, Magnifier
// (c) 2019-2022 Jules Bloomenthal

#ifndef WIDGETS_HDR
#define WIDGETS_HDR

#include <string>
#include <time.h>
#include <vector>
#include "Quaternion.h"
#include "VecMat.h"

using namespace std;

// Time

inline float TimeDif(time_t later, time_t earlier) { return (float)(later-earlier)/CLOCKS_PER_SEC; }
	// return time difference, in seconds

class Timer {
public:
	time_t start = 0;
	bool paused = false;
	float accumulated = 0;		// total elapsed when paused, in seconds
	float duration = 0;			// length of animation (if any), in seconds
	void Pause() {
		if (!paused) accumulated += TimeDif(clock(), start);
		paused = true;
	}
	void Resume() { if (paused) start = clock(); paused = false; }
	void Reset() { start = clock(); accumulated = 0; }
	float Elapsed() { return accumulated+(paused? 0 : TimeDif(clock(), start)); }
	float Progress() { return duration > 0? Elapsed()/duration : 0; }
	Timer(float duration = 0) : duration(duration) { Reset(); }
};

// Mouse

// xmouse and ymouse are in screen (pixel) coordinates with origin at LOWER-left (increasing ymouse is *upwards*)
// MouseOver with 3D arguments expects an appropriate viewport

vec2 NDCfromScreen(vec2 screen);
vec2 NDCfromScreen(int x, int y);
vec2 NDCfromScreen(float x, float y);
vec2 NDCfromScreen(double x, double y);

vec2 ScreenFromNDC(vec2 v);
vec2 ScreenFromNDC(float x, float y);

bool MouseOver(double xmouse, double ymouse, vec3 p, mat4 &view, int proximity = 12);

vec3 *MouseOver(vec3 *points, int npoints, int xmouse, int ymouse, mat4 fullview, int proximity = 12);
vec3 *MouseOver(vector<vec3> &points, int xmouse, int ymouse, mat4 fullview, int proximity = 12);
	// return pointer to point or NULL if none

bool MouseOver(double xmouse, double ymouse, vec2 p, int proximity = 12);
	// is mouse(x,y) within proximity pixels of screen point p?

bool MouseOver(int xmouse, int ymouse, vec2 p, int proximity = 12);

// Matrix Support

int TransformArray(vec3 *in, vec3 *out, int n, mat4 m);

int TransformArray(vector<vec3> &vecs, vector<vec3> &xvecs, mat4 m);

vector<vec3> TransformArray(vector<vec3> &vecs, mat4 m);

vec3 MatrixOrigin(mat4 m);
	// get matrix position

void SetMatrixOrigin(mat4 &m, vec3 position);
	// set matrix position

float MatrixScale(mat4 m);
	// get scale of matrix

void Scale3x3(mat4 &m, float scale);
	// scale matrix

// Mover: move selected point along plane perpendicular to camera

class Mover {
public:
	Mover();
	void Down(vec3 *p, int xmouse, int ymouse, mat4 modelview, mat4 persp);
	void Down(mat4 *t, int xmouse, int ymouse, mat4 modelview, mat4 persp);
	vec3 Drag(int xmouse, int ymouse, mat4 modelview, mat4 persp);
		// return dif between current point and mousedown
	void Wheel(double spin);
		// bring pointer closer-to/further-from camera
	bool Hit(int xmouse, int ymouse, mat4 &view, int proximity = 12);
	bool IsSet(void *p = NULL);
		// return true if Mover::point has been set; if p non-null, return true if point==p
	void Unset();
	vec3 pMousedown;
	vec3 *point = NULL;
	mat4 *transform = NULL;
private:
	vec3 cameraPosition;
	float plane[4] = {0, 0, 0, 0};	// unnormalized
	vec2  mouseOffset;
	friend class Framer;
};

// Arcball: quaternion rotation of reference frame with trackball-like UI

class Arcball {
public:
	vec3 pink = vec3(1, .2f, .8f);
	enum class Use { Camera, Body } use = Use::Camera;
	enum class ConstraintIndex { XAxis = 0, YAxis, ZAxis, None };
	struct Constraint { ConstraintIndex id; vec3 axis; } constraint = { ConstraintIndex::None, vec3() };
	Arcball() { };
	// mouse behavior and drawing depend only on center and radius
	void SetBody(mat4 *m, float radius = 25);
		// set Body arcball, set qStart from *m, set center from m->MatrixPosition
		// on Drag, update *m
		// radius in pixels
	void SetBody(mat4 m, float radius = 25);
		// set Body arcball, set qStart from m, set center from m.MatrixPosition
		// Drag does not update m
	void SetCamera(mat4 *m, vec2 center, float radius);
		// set Camera arcball with given center, radius
	void SetCenter(vec2 center);
		// update center
	void SetCenter(vec2 center, float radius);
		// update center and radius
	void Draw(bool showConstrainAxes = false, mat4 *matOverride = NULL, vec3 *colorOverride = NULL);
		// draw circle at center with given radius and possibly constrained axes
		// should be in screen mode
	bool MouseOver(int xmouse, int ymouse);
	void Down(int xmouse, int ymouse, mat4 modelview, bool constrain = false, mat4 *mOverride = NULL);
		// for Camera arcball or Body arcball
		// set qStart from mOverride if non-null, or from non-null m
	Quaternion Drag(int xmouse, int ymouse);
		// return rotation, update *m if non-null
	void Up();
	void Wheel(double spin, bool shift);
	vec3 ConstrainToAxis(vec3 loose, vec3 axis);
	Constraint GetConstraint(int xmouse, int ymouse, mat4 *m = NULL);
	void SetNearestAxis(int xmouse, int ymouse, mat4 *downMat = NULL);
	bool Hit(int xmouse, int ymouse);
	mat4 *GetMatrix();
	Quaternion GetQ();
	float scale = 1;					// scale of m on input
private:
	mat4 *m = NULL;						// matrix to adjust (upper 3x3 only), only if use is Body
	mat4 modelviewInverse;
	float radius = 100;					// display radius, in pixels
	vec2 center;						// display center
	Quaternion qstart;					// quaternion representing m on mouse down
	Quaternion qq;						// qstart multiplied by mouse drag
	vec2 mouseDown, mouseMove;			// ball hits on mouse down/drag, in pixels
	bool dragging = false;
	vec3 BallV(vec2 mouse);				// vector from frame position to mouse ballpick
	friend class Framer;
};

// Framer: combination mover and arcball for reference frame adjustment

class Framer {
public:
	Framer();
	Framer(mat4 *m, float radius, mat4 fullview);
	void Set(mat4 *m, float radius, mat4 fullview);
		// radius is of arcball, in pixels
	bool Hit(int x, int y);
	void Down(int x, int y, mat4 modelview, mat4 persp, bool control = false);
	void Drag(int x, int y, mat4 modelview, mat4 persp);
	void Up();
	void Wheel(double spin, bool shift);
	void Draw(mat4 fullview, bool showConstrainAxes = false, vec3 *colorOverride = NULL);
	Mover mover;
	Arcball arcball;
	bool moverPicked;
	friend class Arcball;
};

// Joystick: adjust selected vector, along sphere centered at vector base

enum class JoyType {A_None, A_Base, A_Tip};

class Joystick {
public:
	Joystick(vec3 *b = NULL, vec3 *v = NULL, float arrowScale = 1, vec3 color = vec3(0,0,0));
	bool Hit(int xmouse, int ymouse, vec3 b,  vec3 v, mat4 fullview);
	bool Hit(int xmouse, int ymouse, mat4 fullview);
	void Down(int xmouse, int ymouse, vec3 *b, vec3 *v, mat4 modelview, mat4 persp);
	void Drag(int xmouse, int ymouse, mat4 modelview, mat4 persp);
	void Draw(vec3 color, mat4 modelview, mat4 persp);
	void SetVector(vec3 v);
	void SetBase(vec3 b);
	vec3 *base = NULL, *vec = NULL;		// pointers to client data
private:
	float arrowScale = 1;
	vec3 color;
	JoyType mode = JoyType::A_None;
	float plane[4] = {0, 0, 0, 0};
	bool fwdFace = true;
	bool  hit = false;
};

// Rectangular Push-Button

class Button {
public:
	unsigned int textureName = 0;
//	GLuint textureName = 0;
	int nTexChannels = 0;
	string name;
	int x = 0, y = 0, w = 0, h = 0; // in pixels
	vec3 color;
	void Draw(int textureUnit);
	void Draw(const char *nameOverride = NULL, float textSize = 14, vec3 *colorOverride = NULL, vec3 textColor = vec3(0,0,0));
	void Outline(vec3 color, float width = 1);
	bool Hit(int xMouse, int yMouse);
	bool Hit(double xMouse, double yMouse);
	void Initialize(string imageFilename);
	Button() { }
	Button(const char *name, int x, int y, int w, int h, vec3 col = vec3(.7f));
	Button(int x, int y, int w, int h);
};

// Toggle Button

class Toggler {
public:
	bool *ptr = NULL, on = false;
	string name;
	int x, y;
	float dia;
	vec3 onCol, offCol, ringCol;
	Toggler(const char *name = NULL, int x = 0, int y = 0, float dia = 12,
			vec3 col = vec3(.7f), vec3 ringCol = vec3(.1f));
	Toggler(const char *name, bool on, int x, int y, float dia = 12,
			vec3 onCol = vec3(1,0,0), vec3 offCol = vec3(.7f), vec3 ringCol = vec3(.1f));
	Toggler(bool *on, const char *name, int x, int y, float dia = 12,
			vec3 onCol = vec3(1,0,0), vec3 offCol = vec3(.7f), vec3 ringCol = vec3(.1f));
	bool On();
	void Set(bool on);
	void Draw(const char *nameOverride = NULL, float textSize = 14, vec3 *color = NULL);
	bool Hit(int xMouse, int yMouse, int proximity = 12);
	bool Hit(float xMouse, float yMouse, int proximity = 12);
	bool Hit(double xMouse, double yMouse, int proximity = 12);
	bool DownHit(int xMouse, int yMouse, int proximity = 12);
	bool DownHit(float xMouse, float yMouse, int proximity = 12);
	bool DownHit(double xMouse, double yMouse, int proximity = 12);
	const char *Name();
	void SetName(const char *name);
};

// Magnifier

class Magnifier {
public:
	int2 srcLoc, srcLocSave, mouseDown, displaySize;
	vec3 frameColor, cursorColor;
	int blockSize;
	Magnifier(int2 srcLoc = int2(), int2 displaySize = int2(), int blockSize = 20, vec3 frameColor = vec3(0,.7f,0), vec3 cursorColor = vec3(1,0,0));
	Magnifier(int srcX, int srcY, int sizeX, int sizeY, int blockSize = 20, vec3 frameColor = vec3(0,.7f,0), vec3 cursorColor = vec3(1,0,0));
	void Down(int x, int y);
	void Drag(int x, int y);
	bool Hit(int x, int y);
	void Display(int2 displayLoc, bool showSrcWindow = true);
};

#endif
