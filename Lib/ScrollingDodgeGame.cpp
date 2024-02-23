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
		bertDead, bertIdle, gameLogo, ground, cactus, gameOver;

// images
//string	dir = "C:/Users/Jules/Code/GG-Projects/2024/2-Dodge/";
string  gameImage = "GameOver.png", gameLogoImage = "bert-game-logo.png";
string  cactusImage = "Cactus.png";
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
vector<string> bertIdles = {"Bert-idle_0.png", "Bert-idle_1.png","Bert-idle_2.png", "Bert-idle_3.png", "Bert-idle_4.png"};

// hearts
int		numHearts = 3;
vec2	heartPositions[] = { {-1.86f, 0.9f}, {-1.75f, 0.9f}, {-1.64f, 0.9f} };

// gamestate
bool	startedGame = false, scrolling = false, endGame = false;
int		currentScore = 0, highScore = 0;
bool	bertBlinking = false;
bool	jumping = false;
bool	bertHit = false, bertPrevHit = false;

// times
time_t	startTime = clock();
time_t	bertIdleTime = clock(), bertBlinkTime = clock();
time_t	scrollTimeClouds = clock(), scrollTimeGround = clock();
time_t  startJump, endJump;
float	loopDurationClouds = 60, loopDurationGround = 5;	// in seconds

// probes: locations wrt cactus sprite
vec2	cactusSensors[] = { { .9f, .3f}, { .3f, 0.9f }, { -.2f, 0.9f }, { 0.9f, 0.0f }, { -.9f, -.4f }, { -.9f, .0f } };
const	int nCactusSensors = sizeof(cactusSensors) / sizeof(vec2);
vec3	cactusProbes[nCactusSensors];

// misc
float	maxJumpHeight;

// Animation

void ScrollClouds() {
	time_t now = clock();
	float dt = (float)(now - scrollTimeClouds) / CLOCKS_PER_SEC;
	float du = dt/loopDurationClouds;
	scrollTimeClouds = now;
	clouds.uvTransform[0][3] += du;
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

void UpdateStatus() {
	if (!bertPrevHit && bertHit) {
		bertPrevHit = true;
		numHearts--;
		endGame = numHearts <= 0;
	}
	vec2 p = cactus.position;
	if (p.x < -1.2f*aspectRatio) {
		cactus.SetPosition(vec2(1.2f*aspectRatio, -.32f));
		bertPrevHit = false;
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

void Display() {
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
	if (startedGame && !endGame)
		bertRunning.Display();

	jumpingBert();

	// probe first, then display cactus
	if (scrolling) {
		for (int i = 0; i < nCactusSensors; i++)
			cactusProbes[i] = Probe(cactusSensors[i], cactus.ptTransform);
		cactus.Display();
		bertHit = false;
		for (vec3 p : cactusProbes)
			if (abs(p.z - bertRunning.z) < .05f)
				bertHit = true;
	}

	if (endGame) {
		gameOver.Display();
		bertDead.Display();
		scrolling = false;
		highScore = currentScore > highScore? currentScore : highScore;
	}
	if (startedGame && !endGame) {
		float duration = (float) (clock()-startTime)/CLOCKS_PER_SEC;
		currentScore += (int) (duration*10);
	}

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
	// init app window and GL context
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "BertGame");

	// sprites
	clouds.Initialize(cloudsImage, 0, false);
	initSprite(sun, sunImage, -.4f, {0.3f, 0.28f}, {0.92f, 0.82f});
	//freezeClock = initSprite(freezeClock, clockImage, -.4f, 0.075f, 0.075f, 0.0f, 0.0f);
	//bertNeutral = initSprite(bertNeutral, bertNeutralImage, -.4f, 0.12f, 0.12f, -0.5f, -0.395f);
	//bertDetermined = initSprite(bertDetermined, bertDeterminedImage, -.4f, 0.12f, 0.12f, -0.5f, -0.395f);
	initSprite(ground, groundImage, -.4f, {2.0f, 0.25f}, {0.0f, -0.75f}, false);
	initSprite(cactus, cactusImage, -.8f, {0.13f, 0.18f}, {2.5f, -0.32f});
	initSprite(gameOver, gameImage, -0.2f, {0.6f, 0.3f}, {0.0f, 0.0f});
	initSprite(gameLogo, gameLogoImage, -0.2f, {0.6f, 0.3f}, {0.0f, 0.0f});
	heart.Initialize(heartImage, -.4f);
	heart.SetScale(vec2(.05f, .05f));
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