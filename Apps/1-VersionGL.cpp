// 2-VersionGL.cpp: determine GL and GLSL versions

#include "glad.h"
#include <glfw3.h>
#include <stdio.h>
#include "GLXtras.h"

int AppEnd(const char *msg) {
	if (msg) printf("%s\n", msg);
	getchar();
	glfwTerminate();
	return msg == NULL? 0 : 1;
}

int main() {
	if (!InitGLFW(0, 0, 1, 1, ""))
		return AppEnd("can't open window\n");
	printf("GL vendor: %s\n", glGetString(GL_VENDOR));
	printf("GL renderer: %s\n", glGetString(GL_RENDERER));
	printf("GL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	return AppEnd(NULL);
}
