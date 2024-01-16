// GLXtras.h - GLSL/GLFW convenience routines (c) 2019-2023 Jules Bloomenthal

#ifndef GL_XTRAS_HDR
#define GL_XTRAS_HDR

#include "glad.h"
#include <GLFW/glfw3.h>
#include "VecMat.h"

// GLFW
GLFWwindow *InitGLFW(int x, int y, int width, int height, const char *title, bool antiAlias = true);
	// call before other GLFW calls
vec2 MouseCoords();
	// return mouse coords wrt lower left
bool KeyDown(int key);
bool Shift();
bool Control();
typedef void(*MouseButtonCallback)(float x, float y, bool left, bool down);
typedef void(*MouseMoveCallback)(float x, float y, bool leftDown, bool rightDown);
	// x,y with respect to lower-left of window
typedef void(*MouseWheelCallback)(float spin);
typedef void(*ResizeCallback)(int width, int height);
typedef void(*KeyboardCallback)(int key, bool press, bool shift, bool control);
	// ignores GLFW_REPEAT
void RegisterMouseButton(MouseButtonCallback);
void RegisterMouseMove(MouseMoveCallback);
void RegisterMouseWheel(MouseWheelCallback);
void RegisterResize(ResizeCallback);
void RegisterKeyboard(KeyboardCallback);

// Print Info
//#ifndef __APPLE_
int PrintGLErrors(const char *title = NULL);
//#endif
void PrintVersionInfo();
void PrintExtensions();
void PrintProgramLog(int programID);
void PrintProgramAttributes(int programID);
void PrintProgramUniforms(int programID);

// GLSL Compilation
GLuint CompileShaderViaFile(const char *filename, GLint type);
GLuint CompileShaderViaCode(const char **code, GLint type);

// GLSL Linking
GLuint LinkProgramViaCode(const char **vertexCode, const char **pixelCode);
GLuint LinkProgramViaCode(const char **vertexCode,
						  const char **tessellationControlCode,
						  const char **tessellationEvalCode,
						  const char **geometryCode,
						  const char **pixelCode);
#ifndef __APPLE_
GLuint LinkProgramViaCode(const char **computeCode);
#endif
GLuint LinkProgram(GLuint vshader, GLuint pshader);
GLuint LinkProgram(GLuint vshader, GLuint tcshader, GLuint teshader, GLuint gshader, GLuint pshader);
GLuint LinkProgramViaFile(const char *vertexShaderFile, const char *pixelShaderFile);
GLuint LinkProgramViaFile(const char *computeShaderFile);

// Miscellany
int CurrentProgram();
void DeleteProgram(int program);

// Binary Read/Write
void WriteProgramBinary(GLuint program, const char *filename);
bool ReadProgramBinary(GLuint program, const char *filename);
GLuint ReadProgramBinary(const char *filename);

// Uniforms
void SetReport(bool report);
	// if report, print any unknown uniforms or attributes
bool SetUniform(int program, const char *name, bool val);
bool SetUniform(int program, const char *name, int val);
bool SetUniform(int program, const char *name, GLuint val);
	// some compilers confused by int/GLuint distinction
bool SetUniformv(int program, const char *name, int count, int *v);
bool SetUniform(int program, const char *name, float val);
bool SetUniformv(int program, const char *name, int count, float *v);
bool SetUniform(int program, const char *name, vec2 v);
bool SetUniform(int program, const char *name, vec3 v);
bool SetUniform(int program, const char *name, vec4 v);
bool SetUniform(int program, const char *name, vec3 *v);
bool SetUniform(int program, const char *name, vec4 *v);
bool SetUniform3(int program, const char *name, float *v);
bool SetUniform2v(int program, const char *name, int count, float *v);
bool SetUniform3v(int program, const char *name, int count, float *v);
bool SetUniform4v(int program, const char *name, int count, float *v);
bool SetUniform3v(int program, const char *name, int count, float *v, mat4 m);
	// transform array of points *v by m and send to named uniform
bool SetUniform(int program, const char *name, mat3 m);
bool SetUniform(int program, const char *name, mat4 m);
	// if no such named uniform and squawk, print error message

// Attributes
int EnableVertexAttribute(int program, const char *name);
	// find named attribute and enable
void DisableVertexAttribute(int program, const char *name);
	// find named attribute and disable
void VertexAttribPointer(int program, const char *name, GLint ncomponents, GLsizei stride, const GLvoid *offset);
	// find and set named attribute, with given number of components, stride between entries, offset into array
	// this calls glAttribPointer with type = GL_FLOAT and normalize = GL_FALSE

#endif // GL_XTRAS_HDR
