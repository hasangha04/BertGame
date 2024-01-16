// 10-Demo-ScrollingSprite.cpp

#include <glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include "GLXtras.h"
#include "IO.h"
#include "Sprite.h"

// Application Variables

int		winWidth = 1200, winHeight = 600;

Sprite	background;
Sprite  sun;
string  sunImage = "C:/Users/hasan/OneDrive/Desktop/University/Winter 2024/Graphics/Apps/Sun.png";
string	scrollImage = "C:/Users/hasan/OneDrive/Desktop/University/Winter 2024/Graphics/Apps/Clouds.png";
bool	scrolling = true, vertically = false;

float	loopDuration = 10, accumulatedVTime = 0, accumulatedHTime = 0;
time_t	scrollTime = clock();

// Display

void Scroll() {
	time_t now = clock();
	if (scrolling) {
		float dt = (float)(now-scrollTime)/CLOCKS_PER_SEC;
		(vertically? accumulatedVTime : accumulatedHTime) += dt;
	}
	scrollTime = now;
	float v = accumulatedVTime/loopDuration, u = accumulatedHTime/loopDuration;
	background.uvTransform = Translate(u, v, 0);
}

void Display() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	background.Display();
	sun.Display();
	glFlush();
}

// Application

void Resize(int width, int height) {
	glViewport(0, 0, winWidth = width, winHeight = height);
	sun.UpdateTransform();
}

int main(int ac, char **av) {
	// init app window and GL context
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Dodge!!");
	background.Initialize(scrollImage, .7f);
	sun.compensateAspectRatio = true;
	background.compensateAspectRatio = true;
	sun.Initialize(sunImage, .8f);
	sun.SetScale(vec2(0.3f, 0.3f));
	sun.SetPosition(vec2(0.92f, 0.82f));
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
