// ScrollingDodgeGame.cpp

// Main file for 2D side-scrolling dodge game. Main character: Bert

#include <glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include "GLXtras.h"
#include "Draw.h"
#include "IO.h"
#include "Sprite.h"
#include <vector>

// Application Variables

int		winWidth = 1522, winHeight = 790;

Sprite	clouds;
Sprite  sun;
Sprite  heart1;
Sprite  heart2;
Sprite  heart3;
Sprite  freezeClock;
Sprite  bertNeutral;
Sprite  bertRunning;
Sprite  bertDetermined;
Sprite  ground;
Sprite  cactus;
string  cactusImage = "Cactus.png";
string  groundImage = "Ground.png";
string  bertNeutralImage = "Bert-neutral.png";
string  bertRunningImage = "Bert.gif";
string  bertDeterminedImage = "Bert-determined.png";
string  clockImage = "Clock.png";
string  heartImage = "Heart.png";
string  sunImage = "Sun.png";
string	cloudsImage = "Clouds.png";
vector<string> bertNames = { bertRunningImage, bertDeterminedImage };
bool	scrolling = true;

// probes
vec2 branchSensors[] = { { .9f, .3f}, {.3f, 0.9f}, {-.2f, 0.9f}, {0.9f, 0.0f}, {-.9f, -.4f}, {-.9f, .0f} };
// locations wrt branch sprite

// Display

vec3 red(1, 0, 0), grn(0, .5f, 0), yel(1, 1, 0);

vec3 Probe(vec2 ndc) {
	// ndc (normalized device coords) lower left (-1,-1) to upper right (1,1)
	// return screen-space s: s.z is depth-buffer value at pixel (s.x, s.y)
	int4 vp = VP();
	vec3 s(vp[0] + (ndc.x + 1) * vp[2] / 2, vp[1] + (ndc.y + 1) * vp[3] / 2, 0);
	DepthXY((int)s.x, (int)s.y, s.z);
	return s;
}

vec3 Probe(vec2 v, mat4 m) {
	return Probe(Vec2(m * vec4(v, 0, 1)));
}

// Application

const char* usage = R"(Usage:
	mouse drag: move sprite
	mouse wheel: scale
	r/R: rotate raygun
)";

// Scrolling Clouds

float	loopDurationClouds = 60, accumulatedTimeClouds = 0;
time_t	scrollTime = clock();

void ScrollClouds() {
	time_t now = clock();
	float dt = (float)(now - scrollTime) / CLOCKS_PER_SEC;
	accumulatedTimeClouds += dt;
	scrollTime = now;
	float u = accumulatedTimeClouds / loopDurationClouds;
	clouds.uvTransform = Translate(u, 0, 0);
}

// Scrolling Ground

float	loopDurationGround = 3, accumulatedTimeGround = 0;
time_t	scrollTimeGround = clock();

void ScrollGround() {
	time_t now = clock();
	if (scrolling) {
		float dt = (float)(now - scrollTimeGround) / CLOCKS_PER_SEC;
		accumulatedTimeGround += dt;
	}
	scrollTimeGround = now;
	float oldU = ground.uvTransform[0][3];
	float u = accumulatedTimeGround / loopDurationGround;
	ground.uvTransform = Translate(u, 0, 0);
	float newU = ground.uvTransform[0][3];
	float du = newU - oldU;

	// Move Cactus by 2 * du in order to keep pace with ground
	vec2 p = cactus.GetPosition();
	p.x -= 2 * du;
	cactus.SetPosition(p);
}

// Display

void Display() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	clouds.Display();
	sun.Display();
	heart1.Display();
	heart2.Display();
	heart3.Display();
	//freezeClock.Display();
	//bertNeutral.Display();
	//bertDetermined.Display();
	bertRunning.Display();
	ground.Display();

	// test branch probes first, then display branch
	const int nBranchSensors = sizeof(branchSensors) / sizeof(vec2);
	vec3 branchProbes[nBranchSensors];
	for (int i = 0; i < nBranchSensors; i++)
		branchProbes[i] = Probe(branchSensors[i], cactus.ptTransform);
	cactus.Display();
	// draw probes
	glDisable(GL_DEPTH_TEST);
	UseDrawShader(ScreenMode());
	for (int i = 0; i < nBranchSensors; i++)
		Disk(branchProbes[i], abs(branchProbes[i].z - bertRunning.z) < .05f ? 20.f : 9.f, red);
	glEnable(GL_DEPTH_TEST); // need z-buffer for mouse hit-test
	glFlush();
}

// Application

void Resize(int width, int height) {
	glViewport(0, 0, winWidth = width, winHeight = height);
	sun.UpdateTransform();
}

void initializeHearts() {
	heart1.compensateAspectRatio = true;
	heart2.compensateAspectRatio = true;
	heart3.compensateAspectRatio = true;

	heart1.Initialize(heartImage, 0.9f);
	heart1.SetScale(vec2(0.05f, 0.05f));
	heart1.SetPosition(vec2(-0.97f, 0.9f));

	heart2.Initialize(heartImage, 0.9f);
	heart2.SetScale(vec2(0.05f, 0.05f));
	heart2.SetPosition(vec2(-0.91f, 0.9f));

	heart3.Initialize(heartImage, 0.9f);
	heart3.SetScale(vec2(0.05f, 0.05f));
	heart3.SetPosition(vec2(-0.85f, 0.9f));
}

int main(int ac, char** av) {
	// init app window and GL context
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "Dodge!!");
	clouds.Initialize(cloudsImage, .7f);
	freezeClock.compensateAspectRatio = true;
	sun.compensateAspectRatio = true;
	clouds.compensateAspectRatio = true;
	bertNeutral.compensateAspectRatio = true;
	bertDetermined.compensateAspectRatio = true;
	bertRunning.compensateAspectRatio = true;
	ground.compensateAspectRatio = true;
	cactus.compensateAspectRatio = true;
	sun.Initialize(sunImage, .8f);
	sun.SetScale(vec2(0.3f, 0.28f));
	sun.SetPosition(vec2(0.92f, 0.82f));
	initializeHearts();
	freezeClock.Initialize(clockImage, 0.9f);
	freezeClock.SetScale(vec2(0.075f, 0.075f));
	bertNeutral.Initialize(bertNeutralImage, 0.9f);
	bertNeutral.SetScale(vec2(0.12f, 0.12f));
	bertNeutral.SetPosition(vec2(-0.5f, -0.395f));
	bertDetermined.Initialize(bertDeterminedImage, 0.9f);
	bertDetermined.SetScale(vec2(0.12f, 0.12f));
	bertDetermined.SetPosition(vec2(-0.5f, -0.395f));
	//bertRunning.Initialize(bertRunningImage, 0.9f);
	bertRunning.Initialize(bertNames, "", 0.9f);
	bertRunning.autoAnimate = false;
	bertRunning.SetFrame(0);
	bertRunning.SetScale(vec2(0.12f, 0.12f));
	bertRunning.SetPosition(vec2(-0.5f, -0.395f));
	ground.Initialize(groundImage, 0.9f);
	ground.SetScale(vec2(2.0f, 0.25f));
	ground.SetPosition(vec2(0.0f, -0.75f));
	cactus.Initialize(cactusImage, 0.9f);
	cactus.SetScale(vec2(0.13f, 0.18f));
	cactus.SetPosition(vec2(0.3f, -0.32f));
	// callbacks
	RegisterResize(Resize);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		ScrollClouds();
		ScrollGround();
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// terminate
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glfwDestroyWindow(w);
	glfwTerminate();
}