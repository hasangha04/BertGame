// ScrollingDodgeGame.cpp

// Main file for 2D side-scrolling dodge game. Main character: Bert

#include <glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include "GLXtras.h"
#include "IO.h"
#include "Sprite.h"

// Application Variables

int		winWidth = 1522, winHeight = 790;

Sprite	clouds;
Sprite  sun;
Sprite  heart1;
Sprite  heart2;
Sprite  heart3;
Sprite  freezeClock;
Sprite  bertNeutral;
Sprite  ground;
string  groundImage = "Ground.png";
string  bertNeutralImage = "Bert-neutral.png";
string  clockImage = "Clock.png";
string  heartImage = "Heart.png";
string  sunImage = "Sun.png";
string	cloudsImage = "Clouds.png";
bool	scrolling = false;

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

float	loopDurationGround = 8, accumulatedTimeGround = 0;
time_t	scrollTimeGround = clock();

void ScrollGround() {
	time_t now = clock();
	if (scrolling) {
		float dt = (float)(now - scrollTimeGround) / CLOCKS_PER_SEC;
		accumulatedTimeGround += dt;
	}
	scrollTimeGround = now;
	float u = accumulatedTimeGround / loopDurationGround;
	ground.uvTransform = Translate(u, 0, 0);
}

// Display

void Display() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	clouds.Display();
	sun.Display();
	heart1.Display();
	heart2.Display();
	heart3.Display();
	//freezeClock.Display();
	bertNeutral.Display();
	ground.Display();
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
	ground.compensateAspectRatio = true;
	sun.Initialize(sunImage, .8f);
	sun.SetScale(vec2(0.3f, 0.28f));
	sun.SetPosition(vec2(0.92f, 0.82f));
	initializeHearts();
	freezeClock.Initialize(clockImage, 0.9f);
	freezeClock.SetScale(vec2(0.075f, 0.075f));
	bertNeutral.Initialize(bertNeutralImage, 0.9f);
	bertNeutral.SetScale(vec2(0.12f, 0.12f));
	bertNeutral.SetPosition(vec2(-0.5f, -0.395f));
	ground.Initialize(groundImage, 0.9f);
	ground.SetScale(vec2(2.0f, 0.25f));
	ground.SetPosition(vec2(0.0f, -0.75f));
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