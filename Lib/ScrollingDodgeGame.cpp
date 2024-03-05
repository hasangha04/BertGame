// ScrollingDodgeGame.cpp
// Application code for 2D side-scrolling dodge game. Main character: Bert

#include <glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include <vector>
#include "GLXtras.h"
#include "Draw.h"
#include "Text.h"
#include "IO.h"
#include "Sprite.h"

// window
int		winWidth = 1522, winHeight = 790;
float	aspectRatio = (float) winWidth/winHeight;

// sprites
Sprite	clouds, sun, heart, freezeClock, bertNeutral, bertRunning, bertDetermined,
		bertDead, bertHurt, bertIdle, gameLogo, ground, cactus, fence, bush, gameOver;

// images
string  gameImage = "GameOver.png", gameLogoImage = "bert-game-logo.png";
string  cactusImage = "Cactus.png";
string  fenceImage = "Fence.png";
string  groundImage = "Ground.png";
string  bushImage = "Bush.png";
string  bertNeutralImage = "Bert-neutral.png", bertRunningImage = "Bert.gif";
string  bertRun1 = "Bert-run1-mia.png", bertRun2 = "Bert-run2-mia.png";
string  bertDeterminedImage = "Bert-determined.png", bertDeadImage = "Bert-dead.png";
string  clockImage = "Clock.png";
string  heartImage = "Heart.png";
string  sunImage = "Sun.png";
string	cloudsImage = "Clouds.png";

// animations
vector<string> bertNames = { bertRun1, bertRun2 };
vector<string> bertHurts = {"Bert-determined-0.png", "Bert-determined-1.png", "Bert-determined-2.png"};
vector<string> bertIdles = {"Bert-idle_0.png", "Bert-idle_1.png","Bert-idle_2.png", "Bert-idle_3.png", "Bert-idle_4.png"};

// hearts
int		numHearts = 3;
vec2	heartPositions[] = { {-1.86f, 0.9f}, {-1.75f, 0.9f}, {-1.64f, 0.9f} };

// gamestate
bool	startedGame = false, scrolling = false, endGame = false;
float	currentScore = 0.0, highScore = 0.0;
bool	bertBlinking = false;
bool	jumping = false;
bool	bertHit = false, bertPrevHit = false, bertSwitch = false;
bool	loopedGround = false;
int 	level = 1;
int		levelBound = 0;
int		chance = 0;
int     bounds[4];

// times
time_t	startTime = clock();
time_t	bertIdleTime = clock(), bertBlinkTime = clock();
time_t	scrollTimeClouds = clock(), scrollTimeGround = clock();
time_t  startJump, endJump;
time_t  bertHurtDisplayTime = 0;
float	loopDurationClouds = 60, loopDurationGround = 4;	// in seconds
float   levelTime = 20.0; // Keeps track of how long a level is
const float minLoopDurationGround = 1.5f;

// probes: locations wrt cactus sprite
vec2	cactusSensors[] = { { .9f, .3f}, { .3f, 0.9f }, { -.2f, 0.9f }, { 0.9f, 0.0f }, { -.9f, -.4f }, { -.9f, .0f } };
const	int nCactusSensors = sizeof(cactusSensors) / sizeof(vec2);
vec3	cactusProbes[nCactusSensors];

vec2	fenceSensors[] = { {-0.99f, -0.14f}, {-0.96f, 0.31f}, {-0.72f, 0.85f}, {-0.35f, 0.84f}, {0.03f, 0.84f}, {0.40f, 0.84f}, 
	{0.77f, 0.84f}, {0.95f, 0.31f}, {0.98f, -0.15f} };
const	int nFenceSensors = sizeof(fenceSensors) / sizeof(vec2);
vec3	fenceProbes[nFenceSensors];

vec2	clockSensors[] = { {-0.87f, -0.67f}, {-0.85f, -0.47f}, {-0.86f, -0.23f}, {-0.87f, 0.15f}, {-0.98f, 0.36f}, {-0.84f, 0.52f}, 
	{-0.66f, 0.60f}, {-0.35f, 0.74f}, {-0.25f, 0.84f}, {0.01f, 0.84f}, {0.26f, 0.83f}, {0.34f, 0.75f}, {0.63f, 0.60f}, {0.85f, 0.52f}, 
	{0.96f, 0.38f}, {0.85f, 0.15f}, {0.85f, -0.23f} };
const	int nClockSensors = sizeof(clockSensors) / sizeof(vec2);
vec3	clockProbes[nClockSensors];

vec2	bushSensors[] = { {-0.97f, -0.63f}, {-0.98f, -0.20f}, {-0.92f, 0.02f}, {-0.88f, 0.10f}, {-0.82f, 0.22f}, {-0.47f, 0.32f},  
	{-0.38f, 0.51f}, {-0.31f, 0.63f}, {-0.21f, 0.75f}, {-0.11f, 0.83f}, {0.11f, 0.83f}, {0.22f, 0.72f}, {0.31f, 0.61f}, {0.37f, 0.52f}, 
	{0.43f, 0.40f}, {0.47f, 0.31f}, {0.53f, 0.01f}, {0.61f, 0.01f}, {0.78f, -0.00f}, {0.89f, -0.11f}, {0.94f, -0.22f}, {0.98f, -0.31f} };
const	int nBushSensors = sizeof(bushSensors) / sizeof(vec2);
vec3	bushProbes[nBushSensors];

// misc
float	maxJumpHeight;

// Level Selection

void levelOutput()
{
	bounds[0] = 25 - (5 * levelBound);
	bounds[1] = 30 - (5 * levelBound);
	bounds[2] = 45 + (5 * levelBound);
	bounds[3] = 0 + (5 * levelBound);
}

int chanceOutput(int chance)
{
	if (scrolling)
	{
		// Level 1: Display cactus
		if (chance >= 0 && chance < bounds[0])
		{
			return 1;
		}
		// Level 2: Display cactus and bush
		else if (chance >= bounds[0] && chance < bounds[1])
		{
			return 2;
		}
		// Level 3: Display cactus, bush, and fence
		else if (chance >= bounds[1] && chance < bounds[2])
		{
			return 3;
		}
		// Level 4: Display cactus, bush, and fence closer together
		else if (chance >= bounds[2] && chance <= bounds[3])
		{
			return 4;
		}
		// Do the same as Level 1
		else
		{
			return 2;
		}
	}
}

// Animation

void ScrollClouds() {
	time_t now = clock();
	float dt = (float)(now - scrollTimeClouds) / CLOCKS_PER_SEC;
	float du = dt/loopDurationClouds;
	scrollTimeClouds = now;
	clouds.uvTransform[0][3] += du;
}

void AdjustGroundLoopDuration() 
{
	float elapsedTime = (float)(clock() - startTime) / CLOCKS_PER_SEC;

	loopDurationGround = max(minLoopDurationGround, 5.0f - elapsedTime * 0.05f);
}

void ScrollGround() {
	time_t now = clock();
	if (scrolling) {
		float dt = (float)(now - scrollTimeGround) / CLOCKS_PER_SEC;
		float du = dt/loopDurationGround;
		ground.uvTransform[0][3] += du;
		// move sprites to keep pace with ground
		cactus.position.x -= 2 * du * ground.scale.x * aspectRatio;
		fence.position.x -= 2 * du * ground.scale.x * aspectRatio;
		bush.position.x -= 2 * du * ground.scale.x * aspectRatio;
		freezeClock.position.x -= 2 * du * ground.scale.x * aspectRatio;
		// position in +/-1 space, but u is in 0,1, so scale by 2
		// also scale by ground scale and aspect ratio
		cactus.UpdateTransform();
		fence.UpdateTransform();
		bush.UpdateTransform();
		freezeClock.UpdateTransform();
	}
	scrollTimeGround = now;
}

void jumpingBert() {
	if (bertSwitch) {
		if (jumping && clock() - startJump < 380) {
			bertHurt.autoAnimate = false;
			bertHurt.SetFrame(0);
			float jumpHeight = (float)(clock() - startJump) * 0.002f - 0.395f;
			maxJumpHeight = jumpHeight;
			bertHurt.SetPosition(vec2(-1.0f, jumpHeight));
			endJump = clock();
		}
		else {
			vec2 p = bertHurt.position;
			float jumpHeight = p.y;
			if (clock() - endJump > 30) {
				jumpHeight = maxJumpHeight - 0.0018f * (float)(clock() - endJump);
				bertHurt.SetPosition(vec2(-1.0f, jumpHeight));
			}
			if (jumpHeight <= -0.395f) {
				bertHurt.autoAnimate = true;
				bertHurt.SetPosition(vec2(-1.0f, -0.395f));
				jumping = false;
			}
		}
	}
	if (jumping && clock() - startJump < 380) {
		bertRunning.autoAnimate = false;
		bertRunning.SetFrame(0);
		float jumpHeight = (float)(clock() - startJump) * 0.002f - 0.395f;
		maxJumpHeight = jumpHeight;
		bertRunning.SetPosition(vec2(-1.0f, jumpHeight));
		endJump = clock();
	}
	else {
		vec2 p = bertRunning.position;
		float jumpHeight = p.y;
		if (clock() - endJump > 30) {
			jumpHeight = maxJumpHeight - 0.0018f * (float)(clock() - endJump);
			bertRunning.SetPosition(vec2(-1.0f, jumpHeight));
		}
		if (jumpHeight <= -0.395f) {
			bertRunning.autoAnimate = true;
			bertRunning.SetPosition(vec2(-1.0f, -0.395f));
			jumping = false;
		}
	}
}

// Gameplay

void UpdateStatus() 
{
	if (!bertPrevHit && bertHit) {
		bertPrevHit = true;
		numHearts--;
		endGame = numHearts <= 0;
	}

	vec2 c = cactus.position;
	vec2 b = bush.position;
	vec2 f = fence.position;

	switch (level) 
	{
		case 1:
			if (c.x < -1.0f * aspectRatio) 
			{
			}
			else 
			{
				break;
			}
		case 2:
			if (b.x >= -1.0f * aspectRatio)
			{
				break;
			}
		case 3:
		case 4:
			if (f.x >= -1.0f * aspectRatio)
			{
				break;
			}
			chance = 1 + (rand() % 100);
			level = chanceOutput(chance);
			loopedGround = true;
			break;
	}

	if (loopedGround)
	{
		switch (level)
		{
			case 1:
				cactus.SetPosition(vec2(1.2f * aspectRatio, -.32f));
				bertPrevHit = false;
				bertSwitch = false;
				loopedGround = false;
				break;
			case 2:
				cactus.SetPosition(vec2(1.2f * aspectRatio, -.32f));
				bush.SetPosition(vec2(1.8f * aspectRatio, -.42f));
				bertPrevHit = false;
				bertSwitch = false;
				loopedGround = false;
				break;
			case 3:
				cactus.SetPosition(vec2(1.2f * aspectRatio, -.32f));
				bush.SetPosition(vec2(1.8f * aspectRatio, -.42f));
				fence.SetPosition(vec2(2.5f * aspectRatio, -.36f));
				bertPrevHit = false;
				bertSwitch = false;
				loopedGround = false;
				break;
			case 4:
				cactus.SetPosition(vec2(1.2f * aspectRatio, -.32f));
				bush.SetPosition(vec2(1.7f * aspectRatio, -.42f));
				fence.SetPosition(vec2(2.2f * aspectRatio, -.36f));
				bertPrevHit = false;
				bertSwitch = false;
				loopedGround = false;
				break;
		}
	}
}

// Probing Z-Buffer

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

// Display

void blinkingBert() {
	if (clock() - bertIdleTime > 3000 && !bertBlinking) {
		bertIdle.autoAnimate = true;
		bertIdle.SetPosition(vec2(-1.0f, -0.395f));
		bertBlinkTime = clock();
		bertBlinking = true;
	}
	else if (clock() - bertBlinkTime > 200 && bertBlinking) {
		bertIdleTime = clock();
		bertIdle.autoAnimate = false;
		bertIdle.SetFrame(0);
		bertIdle.SetPosition(vec2(-1.0f, -0.395f));
		bertBlinking = false;
	}
}

void displayCactus()
{
	for (int i = 0; i < nCactusSensors; i++)
	{
	    cactusProbes[i] = Probe(cactusSensors[i], cactus.ptTransform);
	}
	cactus.Display();
	bertHit = false;
	for (vec3 p : cactusProbes)
	{
		if (abs(p.z - bertRunning.z) < .05f)
		{
			bertHit = true;
			bertSwitch = true;
		}
	}
}

void displayCactusBush()
{
	for (int i = 0; i < nCactusSensors; i++)
	{
		cactusProbes[i] = Probe(cactusSensors[i], cactus.ptTransform);
	}
	cactus.Display();
	bertHit = false;
	for (vec3 p : cactusProbes)
	{
		if (abs(p.z - bertRunning.z) < .05f)
		{
			bertHit = true;
			bertSwitch = true;
		}
	}	

	for (int i = 0; i < nBushSensors; i++)
	{
		bushProbes[i] = Probe(bushSensors[i], bush.ptTransform);
	}
	bush.Display();
	for (vec3 u : bushProbes)
	{
		if (abs(u.z - bertRunning.z) < .05f)
		{
			bertHit = true;
			bertSwitch = true;
		}
	}
}

void displayCactusBushFence()
{
	for (int i = 0; i < nCactusSensors; i++)
	{
		cactusProbes[i] = Probe(cactusSensors[i], cactus.ptTransform);
	}
	cactus.Display();
	bertHit = false;
	for (vec3 p : cactusProbes)
	{
		if (abs(p.z - bertRunning.z) < .05f)
		{
			bertHit = true;
			bertSwitch = true;
		}
	}

	for (int i = 0; i < nBushSensors; i++)
	{
		bushProbes[i] = Probe(bushSensors[i], bush.ptTransform);
	}
	bush.Display();
	for (vec3 u : bushProbes)
	{
		if (abs(u.z - bertRunning.z) < .05f)
		{
			bertHit = true;
			bertSwitch = true;
		}
	}

	for (int i = 0; i < nFenceSensors; i++)
	{
		fenceProbes[i] = Probe(fenceSensors[i], fence.ptTransform);
	}
	fence.Display();
	for (vec3 q : fenceProbes)
	{
		if (abs(q.z - bertRunning.z) < .05f)
		{
			bertHit = true;
			bertSwitch = true;
		}
	}
}

void Display(float dt) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	clouds.Display();
	sun.Display();
	for (int i = 0; i < numHearts; i++) {
		heart.SetPosition(heartPositions[i]);
		heart.Display();
	}

	if (scrolling == false && jumping == false && startedGame)
		scrolling = true;
	ground.Display();

	if (!startedGame) {
		bertIdle.Display();
		gameLogo.Display();
		blinkingBert();
	}

	jumpingBert();

	if (startedGame && !endGame && !bertSwitch)
		bertRunning.Display();

	if (bertSwitch && numHearts >= 1) 
	{
		bertHurt.Display();
		bertHurtDisplayTime += (time_t) dt * 1000; 
	}
	
	if (bertSwitch && (bertHurtDisplayTime > 800)) 
	{ 
		bertSwitch = false;
		bertRunning.SetFrame(0); 
		bertRunning.autoAnimate = true; 
		bertHurtDisplayTime = 0; 
	}

	//freezeClock.Display();
	
	switch (level)
	{
		case 1:
			displayCactus();
			break;
		case 2:
			displayCactusBush();
			break;
		case 3:
			displayCactusBushFence();
			break;
		case 4:
			displayCactusBushFence();
			break;
	}

	if (endGame) {
		gameOver.Display();
		bertDead.Display();
		scrolling = false;
		bertPrevHit = false;
		bertSwitch = false;
		loopedGround = false;
		levelBound = 0;
		levelTime = 20.0;
		chance = 0;
		level = 1;
		highScore = currentScore > highScore? currentScore : highScore;
	}
	if (startedGame && !endGame) {
		float duration = (float) (clock()-startTime)/CLOCKS_PER_SEC;
		currentScore += (float) (duration*0.01);
		highScore = currentScore > highScore ? currentScore : highScore;
	}

	glDisable(GL_DEPTH_TEST);
	Text(winWidth-550, winHeight - 50, vec3(1, 1, 1), 20, "Current Score: %.0f", currentScore);
	Text(winWidth-550, winHeight - 100, vec3(1, 1, 1), 20, "High Score: %.0f", highScore);

	glFlush();
}

// Sprite Initialization

void initializeBert() {
	bertRunning.Initialize(bertNames, "", -.4f, 0.08f);
	bertRunning.SetScale(vec2(0.12f, 0.12f));
	bertRunning.SetPosition(vec2(-1.0f, -0.395f));

	bertIdle.Initialize(bertIdles, "", -.4f, 0.05f);
	bertIdle.SetScale(vec2(0.12f, 0.12f));
	bertIdle.SetPosition(vec2(-1.0f, -0.395f));
	bertIdle.autoAnimate = false;
	bertIdle.SetFrame(0);

	bertDead.Initialize(bertDeadImage, -.4f);
	bertDead.SetScale(vec2(0.12f, 0.12f));
	bertDead.SetPosition(vec2(-1.0f, -0.395f));

	bertHurt.Initialize(bertHurts, "", - .4f, 0.08f);
	bertHurt.SetScale(vec2(0.12f, 0.12f));
	bertHurt.SetPosition(vec2(-1.0f, -0.395f));
}

void initSprite(Sprite &obj, string img, float z, vec2 scale, vec2 pos, bool compensateAR = true) {
	obj.Initialize(img, z, compensateAR);
	obj.SetScale(scale);
	obj.SetPosition(pos);
}

// Application

void Keyboard(int key, bool press, bool shift, bool control) {
	if ((press && key == ' ') && jumping == false && (startedGame || endGame)) {
		if (endGame) {
			endGame = false;
			numHearts = 3;
			cactus.SetPosition(vec2(1.2f*aspectRatio, -0.32f));
			fence.SetPosition(vec2(1.5f*aspectRatio, -0.36f));
			level = 1;
			levelBound = 0;
			levelTime = 20.0;
			startTime = clock();
			currentScore = 0;
		}
		jumping = true;
		startJump = clock();
	}
	if ((press && key == ' ') && jumping == false && !startedGame) {
		startedGame = true;
		jumping = true;
		startJump = clock();
		startTime = startJump;
	}
}

void Resize(int width, int height) {
	glViewport(0, 0, width, height);
	aspectRatio = (float) width/height;
	sun.UpdateTransform();
}

int main(int ac, char** av) {
	// Ensures random chance variable for every run 
	srand(time(NULL));

	// init app window and GL context
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "BertGame");

	// sprites
	clouds.Initialize(cloudsImage, 0, false);
	initSprite(sun, sunImage, -.4f, {0.3f, 0.28f}, {1.77f, 0.82f});
	initSprite(freezeClock, clockImage, -.4f, {0.12f, 0.12f}, {0.75f, -0.37f});
	initSprite(ground, groundImage, -.4f, {2.0f, 0.25f}, {0.0f, -0.75f}, false);
	initSprite(cactus, cactusImage, -.8f, {0.13f, 0.18f}, {2.5f, -0.32f});
	initSprite(fence, fenceImage, -.8f, {0.35f, 0.15f}, {2.9f, -0.36f});
	initSprite(bush, bushImage, -.8f, { 0.22f, 0.083f }, { 2.7f, -0.42f });
	initSprite(gameOver, gameImage, -0.2f, {0.6f, 0.3f}, {0.0f, 0.0f});
	initSprite(gameLogo, gameLogoImage, -0.2f, {0.6f, 0.3f}, {0.0f, 0.0f});
	heart.Initialize(heartImage, -.4f);
	heart.SetScale(vec2(.05f, .05f));
	initializeBert();
	levelOutput();

	// callbacks
	RegisterResize(Resize);
	RegisterKeyboard(Keyboard);
	// event loop

	time_t prevTime = clock();
	while (!glfwWindowShouldClose(w)) {
		time_t currentTime = clock();
		float dt = (float)(currentTime - prevTime) / CLOCKS_PER_SEC;
		prevTime = currentTime;

		float elapsedTime = (float)(clock() - startTime) / CLOCKS_PER_SEC;
		if (elapsedTime >= levelTime && levelBound <= 4)
		{
			levelBound++;
			levelTime += 20.0;
			levelOutput();
		}

		ScrollClouds();
		//AdjustGroundLoopDuration();
		ScrollGround();
		Display(dt);
		UpdateStatus();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// terminate
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glfwDestroyWindow(w);
	glfwTerminate();
}