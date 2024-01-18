// 10-Demo-ScrollingSprite.cpp

#include <glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include "GLXtras.h"
#include "IO.h"
#include "Sprite.h"

// Application Variables

int		winWidth = 1522, winHeight = 790;

Sprite	background;
Sprite  sun;
Sprite  heart1;
Sprite heart2;
Sprite heart3;
string  heartImage = "Heart.png";
string  sunImage = "Sun.png";
string	scrollImage = "Clouds.png";
bool	scrolling = true, vertically = false;

float	loopDuration = 40, accumulatedVTime = 0, accumulatedHTime = 0;
time_t	scrollTime = clock();

// Display

void Scroll() {
	time_t now = clock();
	if (scrolling) {
		float dt = (float)(now - scrollTime) / CLOCKS_PER_SEC;
		(vertically ? accumulatedVTime : accumulatedHTime) += dt;
	}
	scrollTime = now;
	float v = accumulatedVTime / loopDuration, u = accumulatedHTime / loopDuration;
	background.uvTransform = Translate(u, v, 0);
}

void Display() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	background.Display();
	sun.Display();
	heart1.Display();
	heart2.Display();
	heart3.Display();
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
	background.Initialize(scrollImage, .7f);
	sun.compensateAspectRatio = true;
	//heart.compensateAspectRatio = true;
	background.compensateAspectRatio = true;
	sun.Initialize(sunImage, .8f);
	sun.SetScale(vec2(0.3f, 0.28f));
	sun.SetPosition(vec2(0.92f, 0.82f));
	initializeHearts();
	// callbacks
	RegisterResize(Resize);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Scroll();
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// terminate
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glfwDestroyWindow(w);
	glfwTerminate();
}
