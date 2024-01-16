// Camera.cpp, Copyright (c) Jules Bloomenthal, Seattle, 2018, All rights reserved.

#include <glad.h>
#include "Camera.h"
#include "Draw.h"

namespace {

int VpHeight() {  // viewport height
	vec4 vp;
	glGetFloatv(GL_VIEWPORT, (float *) &vp);
	return (int) vp[3];
}

vec3 EulerFromMatrix(mat4 R) {
	float sy = sqrt(R[0][0]*R[0][0]+R[1][0]*R[1][0]);
	bool singular = sy < 1e-6;
	float x, y, z;
	if (!singular) {
		x = atan2(R[2][1], R[2][2]);
		y = atan2(-R[2][0], sy);
		z = atan2(R[1][0], R[0][0]);
	}
	else {
		x = atan2(-R[1][2], R[1][1]);
		y = atan2(-R[2][0], sy);
		z = 0;
	}
	return vec3(x, y, z);
}

const char *usage = R"(
	mouse-drag:\trotate x,y
	with shift:\ttranslate x,y
	with control:\tconstrain
	mouse-wheel:\trotate z
	with shift:\ttranslate z
)";
}

void Camera::Save(const char *filename) {
	FILE *out = fopen(filename, "wb");
	fwrite(&rot, sizeof(mat4), 1, out);
	fwrite(&tran, sizeof(vec4), 1, out);
	fwrite(&fov, sizeof(float), 1, out);
	fwrite(&nearDist, sizeof(float), 1, out);
	fwrite(&farDist, sizeof(float), 1, out);
	fclose(out);
}

bool Camera::Read(const char *filename) {
	FILE *in = fopen(filename, "rb");
	if (!in) return false;
	bool ok =
		fread(&rot, sizeof(mat4), 1, in) == 1 &&
		fread(&tran, sizeof(vec4), 1, in) == 1 &&
		fread(&fov, sizeof(float), 1, in) == 1 &&
		fread(&nearDist, sizeof(float), 1, in) == 1 &&
		fread(&farDist, sizeof(float), 1, in) == 1;
	fclose(in);
	if (ok) {
		persp = Perspective(fov, aspectRatio, nearDist, farDist);
		modelview = Translate(tran)*rot;
		fullview = persp*modelview;
	}
	return ok;
}

void Camera::Set(int *vp) { Set(vp[0], vp[1], vp[2], vp[3]); }

void Camera::Set(int scrnX, int scrnY, int scrnW, int scrnH) {
	aspectRatio = (float) scrnW/scrnH;
	persp = Perspective(fov, aspectRatio, nearDist, farDist);
	modelview = Translate(tran)*rot;
	fullview = persp*modelview;
	float minS = (float) (scrnW < scrnH? scrnW : scrnH);
#ifdef __APPLE__
	arcball.SetCamera(&this->rot, 2*vec2((float) (scrnX+scrnW/2), (float) (scrnY+scrnH/2)), minS-100);
		// **** above probably wrong but this proc mostly deprecated
#else
	arcball.SetCamera(&this->rot, vec2((float) (scrnX+scrnW/2), (float) (scrnY+scrnH/2)), minS/2-50);
		// **** scrnX relevant?
#endif
};

void Camera::Set(int scrnX, int scrnY, int scrnW, int scrnH, mat4 rot, vec3 tran, float fov, float nearDist, float farDist) {
	this->aspectRatio = (float) scrnW/scrnH;
	this->rot = rot;
	this->tran = tran;
	this->fov = fov;
	this->nearDist = nearDist;
	this->farDist = farDist;
//	tranSpeed = .005f;
	persp = Perspective(fov, aspectRatio, nearDist, farDist);
	modelview = Translate(tran)*rot;
	fullview = persp*modelview;
	float minS = (float) (scrnW < scrnH? scrnW : scrnH);
#ifdef __APPLE__
	arcball.SetCamera(&this->rot, vec2((float) scrnW, (float) scrnH)*1.5f, minS-100);
//	arcball.SetCamera(&this->rot, vec2((float) scrnW, (float) scrnH), minS-100);
#else
	arcball.SetCamera(&this->rot, vec2((float) (scrnW/2), (float) (scrnH/2)), minS/2-50);
#endif
};

void Camera::Set(int *vp, mat4 rot, vec3 tran, float fov, float nearDist, float farDist) {
	Set(vp[0], vp[1], vp[2], vp[3], rot, tran, fov, nearDist);
}

void Camera::Set(int scrnX, int scrnY, int scrnW, int scrnH, Quaternion qrot, vec3 tran, float fov, float nearDist, float farDist) {
	rot = qrot.GetMatrix();
	Set(scrnX, scrnY, scrnW, scrnH, rot, tran, fov, nearDist, farDist);
}

Camera::Camera(int *viewport, vec3 eulerAngs, vec3 tran, float fov, float nearDist, float farDist) {
	rot = RotateX(eulerAngs[0])*RotateY(eulerAngs[1])*RotateZ(eulerAngs[2]);
	Set(viewport, rot, tran, fov, nearDist, farDist);
};

Camera::Camera(int scrnX, int scrnY, int scrnW, int scrnH, vec3 eulerAngs, vec3 tran, float fov, float nearDist, float farDist) {
	rot = RotateX(eulerAngs[0])*RotateY(eulerAngs[1])*RotateZ(eulerAngs[2]);
	Set(scrnX, scrnY, scrnW, scrnH, rot, tran, fov, nearDist, farDist);
};

Camera::Camera(int scrnX, int scrnY, int scrnW, int scrnH, Quaternion qrot, vec3 tran, float fov, float nearDist, float farDist) {
	rot = qrot.GetMatrix();
	Set(scrnX, scrnY, scrnW, scrnH, rot, tran, fov, nearDist, farDist);
};

Camera::Camera(int scrnX, int scrnY, int scrnW, int scrnH, vec3 eye, vec3 lookAt, vec3 up, float fov, float nearDist, float farDist) {
	rot = LookTowards(vec3(0,0,0), lookAt-eye, up);
	Set(scrnX, scrnY, scrnW, scrnH, rot, eye, fov, nearDist, farDist);
};

Camera::Camera(int scrnX, int scrnY, int scrnW, int scrnH, mat4 mv, float fov, float nearDist, float farDist) {
	tran = vec3(mv[0][3], mv[1][3], mv[2][3]);
	rot = mv;
	rot[0][3] = rot[1][3] = rot[2][3] = 0;                  // remove tran from rot
	Set(scrnX, scrnY, scrnW, scrnH, rot, tran, fov, nearDist, farDist);
}

void Camera::SetModelview(mat4 mv) {
	tranOld = tran = vec3(mv[0][3], mv[1][3], mv[2][3]);    // FrameBase(mv);
	rot = modelview = mv;
	rot[0][3] = rot[1][3] = rot[2][3] = 0;                  // remove tran from rot
	fullview = persp*modelview;
}

void Camera::SetFOV(float newFOV) {
	fov = newFOV;
	persp = Perspective(fov, aspectRatio, nearDist, farDist);
	fullview = persp*modelview;
}

float Camera::GetFOV() { return fov; }

void Camera::Resize(int width, int height) {
	aspectRatio = (float) width/height;
	persp = Perspective(fov, aspectRatio, nearDist, farDist);
	fullview = persp*modelview;
#ifdef __APPLE__
	arcball.SetCamera(&rot, vec2((float) width, (float) height)*1.5f, (float) (width < height? width : height)-100);
#else
	arcball.SetCamera(&rot, vec2((float) width, (float) height)/2, (float) (width < height? width : height)/2-50);
#endif
}

void Camera::SetSpeed(float tranS) { tranSpeed = tranS; }

mat4 Camera::GetRotate() {
	mat4 moveToCenter = Translate(-rotateCenter);
	mat4 moveBack = Translate(rotateCenter+rotateOffset);
	return moveBack*rot*moveToCenter;
}

void Camera::SetRotateCenter(vec3 r) {
	// if center of rotation changed, orientation of modelview doesn't change, 
	// but orientation applied to scene with new center of rotation causes shift
	// to scene origin - this is fixed with translation offset computed here
	mat4 m = GetRotate();
	vec4 rXformedWithOldRotateCenter = m*vec4(r);
	rotateCenter = r;
	m = GetRotate(); // this is not redundant!
	vec4 rXformedWithNewRotateCenter = m*vec4(r);
	for (int i = 0; i < 3; i++)
		rotateOffset[i] += rXformedWithOldRotateCenter[i]-rXformedWithNewRotateCenter[i];
}

void Camera::Down(double x, double y, bool shift, bool control) { Down((int) x, (int) y, shift, control); }

void Camera::Down(int x, int y, bool s, bool c) {
	down = true;
	shift = s;
	control = c;
	int width, height;
	ViewportSize(width, height);
	arcball.SetCamera(&rot, vec2 ((float) width, (float) height)/2, (float) (width < height? width : height)/2-50);
	arcball.Down(x, y, c); // mOverride = NULL
	mouseDown = vec2((float) x, (float) y);
	tranOld = tran;
}

void Camera::Drag(int x, int y) { Drag((double) x, (double) y); }

void Camera::Drag(double x, double y) {
	if (shift) {
		vec2 mouse((float) x, (float) y), dif = mouse-mouseDown;
		tran = tranOld+tranSpeed*vec3(dif.x, dif.y, 0);
	}
	else
		arcball.Drag((int) x, (int) y);
	modelview = Translate(tran)*GetRotate();
	fullview = persp*modelview;
}

vec3 Camera::Position() {
	mat4 inv = Transpose(rot)*Translate(-tran); // same as Invert(modelview)
	vec4 oldPositionH = inv*vec4(0, 0, 0, 1);
	return vec3(oldPositionH.x, oldPositionH.y, oldPositionH.z); // inv[0][3], inv[1][3], inv[2][3]
}

void Camera::MoveTo(vec3 p) {
	tranOld = tran;
	// camera modelview C = TR; thus C-inverse = R-inverse * T-inverse
	// world location of camera p = C-inverse * origin
	// p = R-inverse * T-inverse * origin
	// R * p = R * R-inverse * T-inverse * origin
	// R * p = T-inverse * origin, as T-inverse * origin = -t,
	// t = -(R * p)
	mat4 R = GetRotate();
	vec4 t4 = R * vec4(p.x, p.y, p.z, 1);
	tran = -vec3(t4.x, t4.y, t4.z);
	modelview = Translate(tran)*R;
	fullview = persp*modelview;
}

void Camera::Move(vec3 m) { MoveTo(m+Position()); }

void Camera::Wheel(double spin, bool shift) {
	if (shift) tranOld.z = (tran.z += (float) spin); // dolly in/out
	else arcball.Wheel(spin, shift);
	modelview = Translate(tran)*GetRotate();
	fullview = persp*modelview;
}

void Camera::Up() {
	down = false;
	tranOld = tran;
	arcball.Up();
}

vec3 Camera::GetRot() { return EulerFromMatrix(*arcball.GetMatrix()); }

vec3 Camera::GetTran() { return tran; }

void Camera::Draw() { if (!shift) arcball.Draw(control); }

const char *Camera::Usage() { return (char *) usage; }
