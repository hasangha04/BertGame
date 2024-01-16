// ClearScreen.cpp using OpenGL shader architecture

#include <glad.h>													// OpenGL header file
#include <glfw3.h>													// OpenGL toolkit
#include "GLXtras.h"												// VertexAttribPointer, SetUniform

int winWidth = 1600, winHeight = 800;								// window size, in pixels

GLuint vBuffer = 0;													// GPU vertex buffer ID, valid if > 0
GLuint program = 0;													// shader program ID, valid if > 0

// vertex shader: operations before the rasterizer
const char *vertexShader = R"(
	#version 130
	in vec2 point;													// 2D point from GPU memory
	void main() {
		gl_Position = vec4(point, 0, 1);							// 'built-in' output variable
	}
)";

// pixel shader: operations after the rasterizer
const char *pixelShader = R"(
	#version 130
	out vec4 pColor;
	void main() {
		pColor = vec4(0, 1, 0, 1);									// rgb, alpha
	}
)";

void InitVertexBuffer() {
	vec2 v[] = { {-1, -1}, {1, -1}, {1, 1}, {-1, 1} };				// ccw quad
	glGenBuffers(1, &vBuffer);										// ID for GPU buffer
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);							// enable it
	glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);	// copy
}

void Display() {
	glUseProgram(program);											// ensure correct program
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);							// enable GPU buffer
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);		// connect buffer to vertex shader
	glDrawArrays(GL_QUADS, 0, 4);									// 4 vertices (1 quad)
	glFlush();														// flush OpenGL ops
}

int main() {														// application entry
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Clear");
	program = LinkProgramViaCode(&vertexShader, &pixelShader);		// build shader program
	InitVertexBuffer();												// allocate GPU vertex buffer
	while (!glfwWindowShouldClose(w)) {								// event loop
		Display();
		glfwSwapBuffers(w);											// double-buffer is default
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
