// Misc.h (c) 2019-2022 Jules Bloomenthal

#ifndef MISC_HDR
#define MISC_HDR

#include <string.h>
#include "VecMat.h"

// Matrix Misc

mat4 Interpolate(mat4 m1, mat4 m2, float t);

float SlowInOut(float t, float value0 = 0, float value1 = 1);

// Files

std::string GetDirectory();
time_t FileModified(const char *name);
bool FileExists(const char *name);

// Intersections

float RaySphere(vec3 base, vec3 v, vec3 center, float radius);
	// return least pos alpha of ray and sphere (or -1 if none)
	// v presumed unit length

// Image

unsigned char *ReadPixels(const char *fileName, int &width, int &height, int &nChannels);

unsigned char *MergeFiles(const char *imageName, const char *matteName, int &width, int &height);
	// allocate width*height pixels, set them from an image and matte file, return pointer
	// pixels returned are 4 bytes (rgba)
	// this memory should be freed by the caller

// Bump map

unsigned char *GetNormals(unsigned char *depthPixels, int width, int height, float depthIncline = 1);
	// return normal pixels (3 bytes/pixel) that correspond with depth pixels (presumed 3 bytes/pixel)
	// the memory returned should be freed by the caller
	// depthIncline is ratio of distance represented by z range 0-1 to distance represented by width of image

#endif
