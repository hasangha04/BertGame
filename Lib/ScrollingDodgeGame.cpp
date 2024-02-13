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
Sprite  hearts[3];
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
string  bertRun1 = "Bert-run1-mia.png";
string  bertRun2 = "Bert-run2-mia.png";
string  bertDeterminedImage = "Bert-determined.png";
string  clockImage = "Clock.png";
string  heartImage = "Heart.png";
string  sunImage = "Sun.png";
string	cloudsImage = "Clouds.png";
vector<string> bertNames = { bertRun1, bertRun2 };
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

// Jumping Bert

bool	jumping = false;
time_t  startJump, endJump;
float	maxJumpHeight;

void Keyboard(int key, bool press, bool shift, bool control) {
	if ((press && key == ' ') && jumping == false) {
		jumping = true;
		startJump = clock();
	}
}

void jumpingBert() {
	if (jumping && clock() - startJump < 380) {
		bertRunning.autoAnimate = false;
		bertRunning.SetFrame(0);
		float jumpHeight = (clock() - startJump) * 0.002 - 0.395;
		maxJumpHeight = jumpHeight;
		bertRunning.SetPosition(vec2(-0.5f, jumpHeight));
		endJump = clock();
	}
	else {
		vec2 p = bertRunning.GetPosition();
		float jumpHeight = p.y;
		if (clock() - endJump > 30) {
			jumpHeight = maxJumpHeight - 0.0018 * (clock() - endJump);
			bertRunning.SetPosition(vec2(-0.5f, jumpHeight));
		}
		if (jumpHeight <= -0.395f) {
			bertRunning.autoAnimate = true;
			bertRunning.SetPosition(vec2(-0.5f, -0.395f));
			jumping = false;
		}
	}
}

// Gameplay

bool bertHit = false;
bool bertPrevHit = false;
int numHearts = 3;

void UpdateStatus()
{
	if (!bertPrevHit && bertHit) 
	{
		bertPrevHit = true;
		numHearts--;
		if (numHearts < 0)
			numHearts = 0;
	}
	vec2 p = cactus.GetPosition();
	if (p.x < -1.2)
	{
		cactus.SetPosition(vec2(1.2f, -0.32f));
		bertPrevHit = false;
	}
}

// Display

void Display() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	clouds.Display();
	sun.Display();
	for (int i = 0; i < numHearts; i++)
	{
		hearts[i].Display();
	}
	bertRunning.Display();
	ground.Display();

	jumpingBert();

	// test branch probes first, then display branch
	const int nBranchSensors = sizeof(branchSensors) / sizeof(vec2);
	vec3 branchProbes[nBranchSensors];
	for (int i = 0; i < nBranchSensors; i++)
		branchProbes[i] = Probe(branchSensors[i], cactus.ptTransform);
	cactus.Display();
	bertHit = false;
	for (int i = 0; i < nBranchSensors; i++)
		if (abs(branchProbes[i].z - bertRunning.z) < .05f)
			bertHit = true;
	glFlush();
}

// Application

void Resize(int width, int height) {
	glViewport(0, 0, winWidth = width, winHeight = height);
	sun.UpdateTransform();
}

void initializeHearts() {
	hearts[0].compensateAspectRatio = true;
	hearts[1].compensateAspectRatio = true;
	hearts[2].compensateAspectRatio = true;

	hearts[0].Initialize(heartImage, -.4f);
	hearts[0].SetScale(vec2(0.05f, 0.05f));
	hearts[0].SetPosition(vec2(-0.97f, 0.9f));

	hearts[1].Initialize(heartImage, -.4f);
	hearts[1].SetScale(vec2(0.05f, 0.05f));
	hearts[1].SetPosition(vec2(-0.91f, 0.9f));

	hearts[2].Initialize(heartImage, -.4f);
	hearts[2].SetScale(vec2(0.05f, 0.05f));
	hearts[2].SetPosition(vec2(-0.85f, 0.9f));
}

void initializeBert() {
	bertRunning.compensateAspectRatio = true;
	bertRunning.Initialize(bertNames, "", -.4f, 0.08f);
	bertRunning.SetScale(vec2(0.12f, 0.12f));
	bertRunning.SetPosition(vec2(-0.5f, -0.395f));
}

Sprite initSprite(Sprite obj, string img, float z, float scaleX, float scaleY, float posX, float posY)
{
	obj.compensateAspectRatio = true;
	obj.Initialize(img, z);
	obj.SetScale(vec2(scaleX, scaleY));
	obj.SetPosition(vec2(posX, posY));
	return obj;
}

int main(int ac, char** av) {
	// init app window and GL context
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "BertGame");
	clouds.Initialize(cloudsImage, 0);
	clouds.compensateAspectRatio = true;

	sun = initSprite(sun, sunImage, -.4f, 0.3f, 0.28f, 0.92f, 0.82f);
	//freezeClock = initSprite(freezeClock, clockImage, -.4f, 0.075f, 0.075f, 0.0f, 0.0f);
	//bertNeutral = initSprite(bertNeutral, bertNeutralImage, -.4f, 0.12f, 0.12f, -0.5f, -0.395f);
	//bertDetermined = initSprite(bertDetermined, bertDeterminedImage, -.4f, 0.12f, 0.12f, -0.5f, -0.395f);
	ground = initSprite(ground, groundImage, -.4f, 2.0f, 0.25f, 0.0f, -0.75f);
	cactus = initSprite(cactus, cactusImage, -.8f, 0.13f, 0.18f, 0.3f, -0.32f);
	initializeHearts();
	initializeBert();

	// callbacks
	RegisterResize(Resize);
	RegisterKeyboard(Keyboard);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		ScrollClouds();
		ScrollGround();
		Display();
		UpdateStatus();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// terminate
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glfwDestroyWindow(w);
	glfwTerminate();
}