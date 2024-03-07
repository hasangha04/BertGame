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
float	aspectRatio = (float)winWidth / winHeight;

// sprites
Sprite	clouds, sun, heart, freezeClock, bertNeutral, bertRunning, bertDetermined,
bertDead, bertHurt, bertIdle, gameLogo, ground, cactus1, cactus2, cactus3,
fence1, fence2, fence3, bush1, bush2, bush3, gameOver;

// images
string  gameImage = "GameOver.png", gameLogoImage = "bert-game-logo.png";
string  cactusImage = "Cactus.png";
string  fenceImage = "Fence.png";
string  groundImage = "Ground.png";
string  bushImage = "Bush.png";
string  bertDeadImage = "Bert-dead.png";
string  clockImage = "Clock.png";
string  heartImage = "Heart.png";
string  sunImage = "Sun.png";
string	cloudsImage = "Clouds.png";

// animations
vector<string> bertNames = { "Bert-run1-mia.png", "Bert-run2-mia.png" };
vector<string> bertHurts = { "Bert-determined-0.png", "Bert-determined-1.png", "Bert-determined-2.png" };
vector<string> bertIdles = { "Bert-idle_0.png", "Bert-idle_1.png","Bert-idle_2.png", "Bert-idle_3.png", "Bert-idle_4.png" };

// hearts
int		numHearts = 3;
vec2	heartPositions[] = { {-1.86f, 0.9f}, {-1.75f, 0.9f}, {-1.64f, 0.9f} };

// obstacles
Sprite cacti[3] = { cactus1, cactus2, cactus3 };
Sprite bushes[3] = { bush1, bush2, bush3 };
Sprite fences[3] = { fence1, fence2, fence3 };

// gamestate
bool	startedGame = false, scrolling = false, endGame = false;
float	currentScore = 0.0, highScore = 0.0;
bool	bertBlinking = false;
bool	jumping = false;
float	velocityUp = 0.3f, velocityDown = 0.0f, gravity = -0.05f;
bool	bertHit = false, bertPrevHit = false, bertSwitch = false;
bool	loopedGround = false;
bool    clockUsed = false;
bool    clockCoolDown = false;
int 	level = 1;
int		levelBound = 0;
int		chance = 0;
int     bounds[4];
int		levels[] = { 1, 0, 0 };
int numObstacles = 1;

// times
time_t	startTime = clock();
time_t	startClock = clock();
time_t	bertIdleTime = clock(), bertBlinkTime = clock();
time_t	scrollTimeClouds = clock(), scrollTimeGround = clock();
time_t  startJump;
time_t  bertHurtDisplayTime = 0;
time_t  spaceKeyDowntime = 0;
time_t  coolDown = 0;
float   minLoopDurationGround = 1.5f;
float   levelTime = 20.0; // Keeps track of how long a level is
float	loopDurationClouds = 60, loopDurationGround = 3;	// in seconds
int     oldTime = 0;

// probes: locations wrt cactus sprite
vec2	cactusSensors[] = { { .9f, .3f}, { .3f, 0.9f }, { -.2f, 0.9f }, { 0.9f, 0.0f }, { -.9f, -.4f }, { -.9f, .0f } };
const	int nCactusSensors = sizeof(cactusSensors) / sizeof(vec2);
vec3	cactusProbes[nCactusSensors];

vec2	fenceSensors[] = { {-0.99f, -0.14f}, {-0.96f, 0.31f}, {-0.72f, 0.85f}, {-0.35f, 0.84f}, {0.03f, 0.84f}, {0.40f, 0.84f},
	{0.77f, 0.84f}, {0.95f, 0.31f}, {0.98f, -0.15f} };
const	int nFenceSensors = sizeof(fenceSensors) / sizeof(vec2);
vec3	fenceProbes[nFenceSensors];

vec2	bushSensors[] = { {-0.97f, -0.63f}, {-0.98f, -0.20f}, {-0.92f, 0.02f}, {-0.88f, 0.10f}, {-0.82f, 0.22f}, {-0.47f, 0.32f},
	{-0.38f, 0.51f}, {-0.31f, 0.63f}, {-0.21f, 0.75f}, {-0.11f, 0.83f}, {0.11f, 0.83f}, {0.22f, 0.72f}, {0.31f, 0.61f}, {0.37f, 0.52f},
	{0.43f, 0.40f}, {0.47f, 0.31f}, {0.53f, 0.01f}, {0.61f, 0.01f}, {0.78f, -0.00f}, {0.89f, -0.11f}, {0.94f, -0.22f}, {0.98f, -0.31f} };
const	int nBushSensors = sizeof(bushSensors) / sizeof(vec2);
vec3	bushProbes[nBushSensors];

vec2	clockSensors[] = { {-0.87f, -0.67f}, {-0.85f, -0.47f}, {-0.86f, -0.23f}, {-0.87f, 0.15f}, {-0.98f, 0.36f}, {-0.84f, 0.52f},
	{-0.66f, 0.60f}, {-0.35f, 0.74f}, {-0.25f, 0.84f}, {0.01f, 0.84f}, {0.26f, 0.83f}, {0.34f, 0.75f}, {0.63f, 0.60f}, {0.85f, 0.52f},
	{0.96f, 0.38f}, {0.85f, 0.15f}, {0.85f, -0.23f} };
const	int nClockSensors = sizeof(clockSensors) / sizeof(vec2);
vec3	clockProbes[nClockSensors];

// misc
float	maxJumpHeight;

// Level Selection

void levelOutput()
{
	bounds[0] = 25 - (5 * levelBound);
	bounds[1] = 30 - (5 * levelBound);
	bounds[2] = 45 + (5 * levelBound);
}

int chanceOutput(int chance) {
	if (scrolling) {
		// Level 1: Display cactus
		if (chance >= 0 && chance < bounds[0]) {
			return 1;
		}
		// Level 2: Display bush
		else if (chance >= bounds[0] && chance < bounds[1]) {
			return 2;
		}
		// Level 3: Display fence
		else if (chance >= bounds[1] && chance < bounds[2]) {
			return 3;
		}
		else {
			return 2;
		}
	}
}

// Animation

void ScrollClouds() {
	time_t now = clock();
	float dt = (float)(now - scrollTimeClouds) / CLOCKS_PER_SEC;
	float du = dt / loopDurationClouds;
	scrollTimeClouds = now;
	clouds.uvTransform[0][3] += du;
}

void AdjustGroundLoopDuration()
{
	float elapsedTime = (float)(clock() - startTime) / CLOCKS_PER_SEC;

	loopDurationGround = max(minLoopDurationGround, 3.0f - elapsedTime * 0.05f);
}

void ScrollGround() {
	time_t now = clock();
	if (scrolling) {
		float dt = (float)(now - scrollTimeGround) / CLOCKS_PER_SEC;
		float du = dt / loopDurationGround;
		ground.uvTransform[0][3] += du;
		for (int i = 0; i < 3; i++) {
			cacti[i].position.x -= 2 * du * ground.scale.x * aspectRatio;
			fences[i].position.x -= 2 * du * ground.scale.x * aspectRatio;
			bushes[i].position.x -= 2 * du * ground.scale.x * aspectRatio;
			cacti[i].UpdateTransform();
			fences[i].UpdateTransform();
			bushes[i].UpdateTransform();
		}
		freezeClock.position.x -= 2 * du * ground.scale.x * aspectRatio;
		freezeClock.UpdateTransform();
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

// Generate Obstacles

void GenerateObstacles(int numObstacles) {
	float lastDistance = 0.0f;
	cout << "New obstacles: " << endl;
	for (int i = 0; i < numObstacles; i++) {
		chance = 1 + (rand() % 100);
		level = chanceOutput(chance);
		levels[i] = level;
		cout << level << endl;
	}
	for (int i = 0; i < numObstacles; i++) {
		float minDistance = 5.0f * (1 / loopDurationGround);
		float maxDistance = 8.0f * (1 / loopDurationGround);
		float distanceRatio = (rand() % 100);
		float distance = minDistance + (distanceRatio / 100) * (maxDistance - minDistance);
		switch (levels[i]) {
		case 1:
			cacti[i].SetPosition(vec2(lastDistance + distance * aspectRatio, -.32f));
			bertPrevHit = false;
			bertSwitch = false;
			loopedGround = false;
			break;
		case 2:
			bushes[i].SetPosition(vec2(lastDistance + distance * aspectRatio, -.42f));
			bertPrevHit = false;
			bertSwitch = false;
			loopedGround = false;
			break;
		case 3:
			fences[i].SetPosition(vec2(3.0f + lastDistance + distance * aspectRatio, -.36f));
			bertPrevHit = false;
			bertSwitch = false;
			loopedGround = false;
			break;
		}
		lastDistance += distance;
	}
}

// Gameplay

void UpdateStatus() {
	if (!bertPrevHit && bertHit) {
		bertPrevHit = true;
		numHearts--;
		endGame = numHearts <= 0;
	}

	vec2 lastObstacle;
	if (levels[numObstacles - 1] == 1) {
		lastObstacle = cacti[numObstacles - 1].position;
	}
	else if (levels[numObstacles - 1] == 2) {
		lastObstacle = bushes[numObstacles - 1].position;
	}
	else if (levels[numObstacles - 1] == 3) {
		lastObstacle = fences[numObstacles - 1].position;
	}
	vec2 fc = freezeClock.position;

	if (fc.x < -1.0f * aspectRatio && levelBound >= 3)
	{
		freezeClock.SetPosition(vec2(1.75f * aspectRatio, -0.37f));
	}

	if (lastObstacle.x <= -1.2f * aspectRatio) {
		loopedGround = true;
	}

	if (loopedGround) {
		if (loopDurationGround <= 2.0f) {
			numObstacles = 1 + (rand() % 3);
			GenerateObstacles(numObstacles);
		}
		else if (loopDurationGround <= 2.6f) {
			numObstacles = 1 + (rand() % 2);
			GenerateObstacles(numObstacles);
		}
		else {
			numObstacles = 1;
			GenerateObstacles(1);
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

void displayCactus(int index)
{
	for (int i = 0; i < nCactusSensors; i++)
	{
		//cactusProbes[i] = Probe(cactusSensors[i], cactus.ptTransform);
	}
	cacti[index].Display();
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

void displayBush(int index)
{
	for (int i = 0; i < nBushSensors; i++)
	{
		//bushProbes[i] = Probe(bushSensors[i], bush.ptTransform);
	}
	bushes[index].Display();
	for (vec3 u : bushProbes)
	{
		if (abs(u.z - bertRunning.z) < .05f)
		{
			bertHit = true;
			bertSwitch = true;
		}
	}
}

void displayFence(int index)
{
	for (int i = 0; i < nFenceSensors; i++)
	{
		//fenceProbes[i] = Probe(fenceSensors[i], fence.ptTransform);
	}
	fences[index].Display();
	for (vec3 q : fenceProbes)
	{
		if (abs(q.z - bertRunning.z) < .05f)
		{
			bertHit = true;
			bertSwitch = true;
		}
	}
}

void displayClock()
{
	for (int i = 0; i < nClockSensors; i++)
	{
		clockProbes[i] = Probe(clockSensors[i], freezeClock.ptTransform);
	}
	freezeClock.Display();
	for (vec3 p : clockProbes)
	{
		if (abs(p.z - bertRunning.z) < .05f)
		{
			clockUsed = true;
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

	if (startedGame && !endGame && !bertSwitch) {
		bertRunning.Display();
	}

	if (bertSwitch && numHearts >= 1)
	{
		bertHurt.Display();
		bertHurtDisplayTime += (time_t)dt * 1000;
	}

	if (bertSwitch && (bertHurtDisplayTime > 800))
	{
		bertSwitch = false;
		bertRunning.SetFrame(0);
		bertRunning.autoAnimate = true;
		bertHurtDisplayTime = 0;
	}

	//freezeClock.Display();

	for (int i = 0; i < 3; i++) {
		if (levels[i] == 1) {
			displayCactus(i);
		}
		else if (levels[i] == 2) {
			displayBush(i);
		}
		else if (levels[i] == 3) {
			displayFence(i);
		}
	}

	if (levelBound >= 3 && !clockUsed && !clockCoolDown)
	{
		displayClock();
		if (clockUsed)
		{
			oldTime = loopDurationGround;
			loopDurationGround = 3.5;
			startClock = clock();
		}
	}
	if (clockUsed)
	{
		float elapsedTime = (float)(clock() - startClock) / CLOCKS_PER_SEC;
		if (elapsedTime > 10.0)
		{
			loopDurationGround = oldTime;
			clockUsed = false;
			clockCoolDown = true;
			coolDown = clock();
		}
	}

	// Puts freeze clock on a cool down of 30 seconds before it can be used again
	if (clockCoolDown)
	{
		float coolDownTime = (float)(clock() - startClock) / CLOCKS_PER_SEC;
		if (coolDownTime > 30.0)
		{
			clockCoolDown = false;
		}
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
		highScore = currentScore > highScore ? currentScore : highScore;
	}
	if (startedGame && !endGame) {
		float duration = (float)(clock() - startTime) / CLOCKS_PER_SEC;
		currentScore = (float)(duration * 10);
		highScore = currentScore > highScore ? currentScore : highScore;
	}

	glDisable(GL_DEPTH_TEST);
	Text(winWidth - 550, winHeight - 50, vec3(1, 1, 1), 20, "Current Score: %.0f", currentScore);
	Text(winWidth - 550, winHeight - 100, vec3(1, 1, 1), 20, "High Score: %.0f", highScore);

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

	bertHurt.Initialize(bertHurts, "", -.4f, 0.08f);
	bertHurt.SetScale(vec2(0.12f, 0.12f));
	bertHurt.SetPosition(vec2(-1.0f, -0.395f));
}

void initSprite(Sprite& obj, string img, float z, vec2 scale, vec2 pos, bool compensateAR = true) {
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
			/*cactus.SetPosition(vec2(1.2f * aspectRatio, -0.32f));
			bush.SetPosition(vec2(1.2f * aspectRatio, -0.32f));
			fence.SetPosition(vec2(1.5f*aspectRatio, -0.36f));*/
			level = 1;
			levelBound = 0;
			levelTime = 20.0;
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
	aspectRatio = (float)width / height;
	sun.UpdateTransform();
}

int main(int ac, char** av) {
	// Ensures random chance variable for every run 
	srand(time(NULL));

	// init app window and GL context
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "BertGame");

	// sprites
	clouds.Initialize(cloudsImage, 0, false);
	initSprite(sun, sunImage, -.4f, { 0.3f, 0.28f }, { 1.77f, 0.82f });
	initSprite(freezeClock, clockImage, -.8f, { 0.12f, 0.12f }, { 1.75f, -0.37f });
	initSprite(ground, groundImage, -.4f, { 2.0f, 0.25f }, { 0.0f, -0.75f }, false);
	for (int i = 0; i < 3; i++) {
		initSprite(cacti[i], cactusImage, -.8f, { 0.13f, 0.18f }, { 2.5f, -0.32f });
		initSprite(fences[i], fenceImage, -.8f, { 0.35f, 0.15f }, { 2.9f, -0.36f });
		initSprite(bushes[i], bushImage, -.8f, { 0.22f, 0.083f }, { 2.7f, -0.42f });
	}
	initSprite(gameOver, gameImage, -0.2f, { 0.6f, 0.3f }, { 0.0f, 0.0f });
	initSprite(gameLogo, gameLogoImage, -0.2f, { 0.6f, 0.3f }, { 0.0f, 0.0f });
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
			cout << levelBound << endl;
			levelTime += 20.0;
			levelOutput();
		}

		ScrollClouds();
		/*if (!clockUsed)
		{
			AdjustGroundLoopDuration();
		}*/
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