// Camera.h (c) 2019-2022 Jules Bloomenthald

#ifndef CAMERA_HDR
#define CAMERA_HDR

#include "VecMat.h"
#include "Widgets.h"

// mousex, mousey in screen coordinates (xmouse increases rightwards, ymouse increases downwards)
// OpenGL expects Y-axis upwards, mouse coords expect Y downwards
// simple camera parameters and methods for mouse
// no-shift
//   drag: rotate about X and Y axes
//   wheel: rotate about Z axis
// shift
//   drag: translate along X and Y axes
//   wheel: translate along Z axis
// control
//   drag: constrain rotation/translation to major axes

class Camera {
private:
	float   aspectRatio = 1;
	float   fov = 30;
	float   nearDist = .001f, farDist = 500;
	vec3    rotateCenter;               // world rotation origin
	vec3    rotateOffset;               // for temp change in world rotation origin
	mat4    rot;                        // rotations controlled by arcball
	float   tranSpeed = .005f;
	vec3    tran, tranOld;              // translation controlled directly by mouse
public:
	vec2    mouseDown;                  // for each mouse down, need start point
	bool	shift = false, control = false, down = false;
	Arcball arcball;
	mat4    modelview, persp, fullview; // read-only
	mat4    GetRotate();
	vec3	Position();
	void    SetRotateCenter(vec3 r);
	void    Up();
	void    Down(double xmouse, double ymouse, bool shift = false, bool control = false);
	void    Down(int xmouse, int ymouse, bool shift = false, bool control = false);
	void    Drag(double xmouse, double ymouse);
	void    Drag(int xmouse, int ymouse);
	void    Wheel(double spin, bool shift = false);
	void	MoveTo(vec3 t);
	void	Move(vec3 m);
	void    Resize(int w, int h);
	float   GetFOV();
	void    SetFOV(float fov);
	void    SetSpeed(float tranSpeed);
	void    SetModelview(mat4 m);
	vec3    GetRot(); // return x, y, z rotations (in radians)
	vec3    GetTran();
	void	Draw();
	void    Set(int *vp);
	void    Set(int scrnX, int scrnY, int scrnW, int scrnH);
	void    Set(int *viewport, mat4 rot, vec3 tran,
				float fov = 30, float nearDist = .001f, float farDist = 500);
	void    Set(int scrnX, int scrnY, int scrnW, int scrnH, mat4 rot, vec3 tran,
				float fov = 30, float nearDist = .001f, float farDist = 500);
	void    Set(int scrnX, int scrnY, int scrnW, int scrnH, Quaternion qrot, vec3 tran,
				float fov = 30, float nearDist = .001f, float farDist = 500);
	void    Save(const char *filename);
	bool    Read(const char *filename);
	const char *Usage();
	Camera() { };
	Camera(int *vp, vec3 rot = vec3(0,0,0), vec3 tran = vec3(0,0,0),
		   float fov = 30, float nearDist = .001f, float farDist = 500);
	Camera(int scrnX, int scrnY, int scrnW, int scrnH, vec3 rot = vec3(0,0,0), vec3 tran = vec3(0,0,0),
		   float fov = 30, float nearDist = .001f, float farDist = 500);
	Camera(int scrnX, int scrnY, int scrnW, int scrnH, Quaternion rot, vec3 tran = vec3(0,0,0),
		   float fov = 30, float nearDist = .001f, float farDist = 500);
	Camera(int scrnX, int scrnY, int scrnW, int scrnH, vec3 pos, vec3 lookAt, vec3 up,
		   float fov = 30, float nearDist = .001f, float farDist = 500);
	Camera(int scrnX, int scrnY, int scrnW, int scrnH, mat4 modelview,
		   float fov = 30, float nearDist = .001f, float farDist = 500);
friend class Arcball;
};

#endif
