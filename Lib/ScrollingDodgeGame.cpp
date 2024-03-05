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
		bertDead, bertHurt, bertIdle, gameLogo, ground, cactus, fence, gameOver;

// images
string  gameImage = "GameOver.png", gameLogoImage = "bert-game-logo.png";
string  cactusImage = "Cactus.png";
string  fenceImage = "Fence.png";
string  groundImage = "Ground.png";
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
float	velocityUp = 0.3f, velocityDown = 0.0f, gravity = -0.05f;
bool	bertHit = false, bertPrevHit = false, bertSwitch = false;

// times
time_t	startTime = clock();
time_t	bertIdleTime = clock(), bertBlinkTime = clock();
time_t	scrollTimeClouds = clock(), scrollTimeGround = clock();
time_t  startJump, endJump;
time_t  bertHurtDisplayTime = 0;
time_t spaceKeyDowntime = 0;
float	loopDurationClouds = 60, loopDurationGround = 5;	// in seconds

// probes: locations wrt cactus sprite
vec2	cactusSensors[] = { { .9f, .3f}, { .3f, 0.9f }, { -.2f, 0.9f }, { 0.9f, 0.0f }, { -.9f, -.4f }, { -.9f, .0f } };
const	int nCactusSensors = sizeof(cactusSensors) / sizeof(vec2);
vec3	cactusProbes[nCactusSensors];

// misc
float	maxJumpHeight = -0.395f;
int		level = 0;
float   levelTime = 20.0;
int bounds[4];

void levelOutput()
{
	bounds[0] = 25 - (5 * level);
	bounds[1] = 30 - (5 * level);
	bounds[2] = 45 + (5 * level);
	bounds[3] = 0 + (5 * level);
}

// Animation

void ScrollClouds() {
	time_t now = clock();
	float dt = (float)(now - scrollTimeClouds) / CLOCKS_PER_SEC;
	float du = dt/loopDurationClouds;
	scrollTimeClouds = now;
	clouds.uvTransform[0][3] += du;
}

const float minLoopDurationGround = 1.5f;

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
		// move cactus to keep pace with ground
		cactus.position.x -= 2 * du * ground.scale.x * aspectRatio;
			// position in +/-1 space, but u is in 0,1, so scale by 2
			// also scale by ground scale and aspect ratio
		cactus.UpdateTransform();
	}
	scrollTimeGround = now;
}

void jumpingBert() { 
	vec2 p = bertRunning.position;
	if (bertSwitch) { p = bertHurt.position; }
	float jumpHeight = p.y;
	if (jumping && jumpHeight >= -0.395f) {
		bertRunning.autoAnimate = false;
		bertRunning.SetFrame(0);
		bertHurt.autoAnimate = false;
		bertHurt.SetFrame(0);
		float jumpTime = (float)(clock() - startJump) / CLOCKS_PER_SEC;
		maxJumpHeight += velocityUp * jumpTime;
		velocityUp += gravity * jumpTime;
		bertRunning.SetPosition(vec2(-1.0f, maxJumpHeight));
		bertHurt.SetPosition(vec2(-1.0f, maxJumpHeight));
		endJump = clock();
	}
	else { 
		if (jumping) {
			float jumpTime = (float)(clock() - startJump) / CLOCKS_PER_SEC;
			jumpHeight += velocityDown * jumpTime;
			velocityDown += gravity * jumpTime;
			bertRunning.SetPosition(vec2(-1.0f, jumpHeight));
			bertHurt.SetPosition(vec2(-1.0f, jumpHeight));
		}
		if (jumpHeight <= -0.395f) {
			velocityUp = 0.3f;
			velocityDown = 0.0f;
			maxJumpHeight = -0.395f;
			bertRunning.autoAnimate = true;
			bertRunning.SetPosition(vec2(-1.0f, -0.395f));
			bertHurt.autoAnimate = true;
			bertHurt.SetPosition(vec2(-1.0f, -0.395f));
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
	vec2 p = cactus.position;
	if (p.x < -1.2f*aspectRatio) 
	{
		cactus.SetPosition(vec2(1.2f*aspectRatio, -.32f));
		bertPrevHit = false;
		bertSwitch = false;
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

	if (startedGame && !endGame && !bertSwitch) {
		bertRunning.Display();
	}

	if (bertSwitch && numHearts >= 1) 
	{
		bertHurt.Display();
		bertHurtDisplayTime += dt * 1000; 
	}
	
	if (bertSwitch && (bertHurtDisplayTime > 800)) 
	{ 
		bertSwitch = false;
		bertRunning.SetFrame(0); 
		bertRunning.autoAnimate = true; 
		bertHurtDisplayTime = 0; 
	}

	// probe first, then display cactus
	if (scrolling) {
		for (int i = 0; i < nCactusSensors; i++)
			cactusProbes[i] = Probe(cactusSensors[i], cactus.ptTransform);
		cactus.Display();
		bertHit = false;
		for (vec3 p : cactusProbes)
			if (abs(p.z - bertRunning.z) < .05f)
			{
				bertHit = true;
				bertSwitch = true;
			}
	}

	//freezeClock.Display();

	int chance = (1 + (rand() % 100));
	if (0 <= chance && chance < bounds[3])
	{
		// Extra output (150%)
	}
	if (bounds[3] <= chance && chance < bounds[0])
	{
		// No output (0%)
	}
	if (bounds[0] <= chance && chance < bounds[1])
	{
		// Half output (50%)
	}
	if (bounds[1] <= chance && chance < bounds[2])
	{
		// Regular output (100%)
	}
	// Regular output (100%)

	if (endGame) {
		gameOver.Display();
		bertDead.Display();
		scrolling = false;
		bertPrevHit = false;
		bertSwitch = false;
		level = 0;
		levelTime = 20.0;
		highScore = currentScore > highScore? currentScore : highScore;
	}
	if (startedGame && !endGame) {
		float duration = (float) (clock()-startTime)/CLOCKS_PER_SEC;
		currentScore = (float) (duration*10);
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
	if (key == GLFW_KEY_SPACE)
		spaceKeyDowntime = press ? clock() : 0;
	if ((press && key == ' ') && jumping == false && (startedGame || endGame)) {
		jumping = true;
		if (endGame) {
			endGame = false;
			numHearts = 3;
			cactus.SetPosition(vec2(1.2f*aspectRatio, -0.32f));
			startTime = clock();
			currentScore = 0;
		}
		startJump = clock();
	}
	if ((press && key == ' ') && jumping == false && !startedGame) {
		jumping = true;
		startedGame = true;
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
	// init app window and GL context
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "BertGame");

	// sprites
	clouds.Initialize(cloudsImage, 0, false);
	initSprite(sun, sunImage, -.4f, {0.3f, 0.28f}, {1.77f, 0.82f});
	initSprite(freezeClock, clockImage, -.4f, {0.12f, 0.12f}, {0.75f, -0.37f});
	initSprite(ground, groundImage, -.4f, {2.0f, 0.25f}, {0.0f, -0.75f}, false);
	initSprite(cactus, cactusImage, -.8f, {0.13f, 0.18f}, {2.5f, -0.32f});
	initSprite(fence, fenceImage, -.8f, {0.35f, 0.15f}, {2.5f, -0.32f});
	initSprite(gameOver, gameImage, -0.2f, {0.6f, 0.3f}, {0.0f, 0.0f});
	initSprite(gameLogo, gameLogoImage, -0.2f, {0.6f, 0.3f}, {0.0f, 0.0f});
	heart.Initialize(heartImage, -.4f);
	heart.SetScale(vec2(.05f, .05f));
	initializeBert();

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
		if (elapsedTime >= levelTime && level <= 5)
		{
			level++;
			levelTime += 20.0;
			levelOutput();
		}

		ScrollClouds();
		AdjustGroundLoopDuration();
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