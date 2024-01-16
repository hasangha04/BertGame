// Misc.cpp (c) 2019-2022 Jules Bloomenthal

#include <glad.h>
#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "stb_image.h"
#include "Draw.h"
#include "Misc.h"
#include "Quaternion.h"

// Matrix Misc

vec3 GetColumn(mat4 m, int i) { return vec3(m[0][i], m[1][i], m[2][i]); }
void SetColumn(mat4 &m, int i, vec3 v) { for (int k = 0; k < 3; k++) m[k][i] = v[k]; }

mat4 Interpolate(mat4 m1, mat4 m2, float t) {
	vec3 p1 = GetColumn(m1, 3), p2 = GetColumn(m2, 3), p = p1+t*(p2-p1);
	float s1 = length(GetColumn(m1, 0)), s2 = length(GetColumn(m2, 0)), s = s1+t*(s2-s1);
	Quaternion q1(m1), q2(m2), q;
	q.Slerp(q1, q2, t);
	mat4 m = Scale(s)*q.GetMatrix();
	SetColumn(m, 3, p);
	return m;
}

float SlowInOut(float t, float value0, float value1) {
	float t2 = t*t, t3 = t*t2;
	return t < 0? 0 : t > 1? 1 : value0+(3*t2-t3-t3)*(value1-value0);
}

// Image Misc

unsigned char *ReadPixels(const char *fileName, int &width, int &height, int &nChannels) {
	stbi_set_flip_vertically_on_load(true); // false); changed 11/8/23
	unsigned char *data = stbi_load(fileName, &width, &height, &nChannels, 0);
	if (!data)
		printf("Can't open %s (%s)\n", fileName, stbi_failure_reason());
	return data;
}

// Sphere

float RaySphere(vec3 base, vec3 v, vec3 center, float radius) {
	// return least pos alpha of ray and sphere (or -1 if none)
	vec3 q = base-center;
	float vDot = dot(v, q);
	float sq = vDot*vDot-dot(q, q)+radius*radius;
	if (sq < 0)
		return -1;
	float root = sqrt(sq), a = -vDot-root;
	return a > 0? a : -vDot+root;
}

#ifdef __APPLE__
std::string GetDirectory() { return "unimplemented"; }
#else
std::string GetDirectory() {
	char buf[256];
	GetCurrentDirectoryA(256, buf);
	for (size_t i = 0; i < strlen(buf); i++)
		if (buf[i] == '\\') buf[i] = '/';
	return std::string(buf)+std::string("/");
}
#endif

time_t FileModified(const char *name) {
	struct stat info;
	stat(name, &info);
	return info.st_mtime;
}

bool FileExists(const char *name) {
	return fopen(name, "r") != NULL;
}

namespace Misc {

// Matting

float GetVal(unsigned char *data, int x, int y, int w, int nChannels) {
	unsigned char c = data[nChannels == 1? y*w+x : 3*(y*w+x)];
	return (float) c/255;
}

float Lerp(float a, float b, float t) { return a+t*(b-a); }

unsigned char *MergeFiles(const char *imageName, const char *matteName, int &imageWidth, int &imageHeight) {
	int imageNChannels, matteWidth, matteHeight, matteNChannels;
	unsigned char *imageData = ReadPixels(imageName, imageWidth, imageHeight, imageNChannels);
	unsigned char *matteData = ReadPixels(matteName, matteWidth, matteHeight, matteNChannels);
	printf("image: %i channels, matte: %i channels\n", imageNChannels, matteNChannels);
	if (!imageData || !matteData)
		return NULL;
	bool sameSize = imageWidth == matteWidth && imageHeight == matteHeight;
	unsigned char *oData = new unsigned char[4*imageWidth*imageHeight];
	unsigned char *t = imageData, *m = matteData, *o = oData;
	for (int j = 0; j < imageHeight; j++)
		for (int i = 0; i < imageWidth; i++) {
			for (int k = 0; k < 3; k++)
				*o++ = *t++;
			if (imageNChannels == 4) t++;
			if (sameSize) {
				*o++ = *m++;
				if (matteNChannels == 3) m += 2;
				if (matteNChannels == 4) m += 3;
			}
			else {
				float txf = (float) i/(imageWidth), tyf = (float) j/(imageHeight);
				float x = txf*matteWidth, y = tyf*matteHeight;
				int x0 = (int) floor(x), y0 = (int) floor(y);
				float mxf = x-x0, myf = y-y0;
				bool lerpX = x0 < matteWidth-1, lerpY = y0 < matteHeight-1;
				float v00 = GetVal(matteData, x0, y0, matteWidth, matteNChannels), v10, v11, v01;
				if (lerpX) v10 = GetVal(matteData, x0+1, y0, matteWidth, matteNChannels);
				if (lerpY) v01 = GetVal(matteData, x0, y0+1, matteWidth, matteNChannels);
				if (lerpX && lerpY) v11 = GetVal(matteData, x0+1, y0+1, matteWidth, matteNChannels);
				float v = v00;
				if (lerpX && !lerpY)
					v = Lerp(v00, v10, mxf);
				if (!lerpX && lerpY)
					v = Lerp(v00, v01, myf);
				if (lerpX && lerpY) {
					float v1 = Lerp(v00, v10, mxf), v2 = Lerp(v01, v11, mxf);
					v = Lerp(v1, v2, myf);
				}
				*o++ = (unsigned char) (255.*v);
			}
		}
	delete [] imageData;
	delete [] matteData;
	return oData;
}

} // namespace Misc

// Bump map

unsigned char *GetNormals(unsigned char *depthPixels, int width, int height, float depthIncline) {
	class Helper { public:
		unsigned char *depthPixels, *bumpPixels;
		int width = 0, height = 0;
		float xscale = 1, yscale = 1, depthIncline = 1;
		float GetDepth(int i, int j) {
			unsigned char *v = depthPixels+3*(j*width+i); // depth pixels presumed 3 bytes/pixel, with r==g==b
			return ((float) *v)/255.f;
		}
		float Dz(int i1, int j1, int i2, int j2) {
			return GetDepth(i2, j2)-GetDepth(i1, j1);
		}
		vec3 Normal(int i, int j) {
			int i1 = i > 0? i-1 : i, i2 = i < width-1? i+1 : i;
			int j1 = j > 0? j-1 : j, j2 = j < height-1? j+1 : j;
			vec3 vx((float)(i2-i1)*xscale, 0, depthIncline*Dz(i1, j, i2, j));
			vec3 vy(0, (float)(j2-j1)*yscale, depthIncline*Dz(i, j1, i, j2));
			vec3 v = cross(vx, vy);
			v = normalize(v);
			if (v.z < 0) printf("Normal: v=(%3.2f, %3.2f, %3.2f)!\n", v.x, v.y, v.z);
			return v; // v.z < 0? -v : v;
		}
		Helper(unsigned char *depthPixels, int width, int height, float depthIncline) :
			depthPixels(depthPixels), width(width), height(height), depthIncline(depthIncline) {
				// xscale/yscale assumes quad maps to canonical (0,0) to (1,1)
				xscale = 1/(float)width;
				yscale = 1/(float)height;
				int bytesPerPixel = 3, bytesPerImage = width*height*bytesPerPixel; // returned pixels 3 bytes/pixel
				unsigned char *n = new unsigned char[bytesPerImage];
				bumpPixels = n;
				for (int j = 0; j < height; j++)                  // row
					for (int i = 0; i < width; i++) {             // column
						vec3 v = Normal(i, j);
						*n++ = (unsigned char) (127.5f*(v[0]+1)); // red in [-1,1]
						*n++ = (unsigned char) (127.5f*(v[1]+1)); // grn in [-1,1]
						*n++ = (unsigned char) (255.f*v[2]);      // blu in [0,1]
					}
		}
	} h(depthPixels, width, height, depthIncline);
	return h.bumpPixels;
}

/* Wayside

// center/scale for unit size models

void UpdateMinMax(vec3 p, vec3 &min, vec3 &max) {
	for (int k = 0; k < 3; k++) {
		if (p[k] < min[k]) min[k] = p[k];
		if (p[k] > max[k]) max[k] = p[k];
	}
}

float GetScaleCenter(vec3 &min, vec3 &max, float scale, vec3 &center) {
	center = .5f*(min+max);
	float maxrange = 0;
	for (int k = 0; k < 3; k++)
		if ((max[k]-min[k]) > maxrange)
			maxrange = max[k]-min[k];
	return scale*2.f/maxrange;
}

// standardize STL models

void MinMax(vector<VertexSTL> &points, vec3 &min, vec3 &max) {
	min.x = min.y = min.z = FLT_MAX;
	max.x = max.y = max.z = -FLT_MAX;
	for (int i = 0; i < (int) points.size(); i++)
		UpdateMinMax(points[i].point, min, max);
}

void Standardize(vector<VertexSTL> &vertices, float scale) {
	vec3 min, max, center;
	MinMax(vertices, min, max);
	float s = GetScaleCenter(min, max, scale, center);
	for (int i = 0; i < (int) vertices.size(); i++) {
		vec3 &v = vertices[i].point;
		v = s*(v-center);
	}
}

// Targa Image File

GLuint LoadTargaTexture(const char *targaFilename, bool mipmap) {
	int width, height, bytesPerPixel;
	unsigned char *pixels = ReadTarga(targaFilename, &width, &height, &bytesPerPixel);
	GLuint textureName = LoadTexture(pixels, width, height, bytesPerPixel, true, mipmap); // Targa is BGR
	delete [] pixels;
	return textureName;
}

unsigned char *ReadTarga(const char *filename, int *width, int *height, int *bytesPerPixel) {
	// open targa file, read header, return pointer to pixels
	FILE *in = fopen(filename, "rb");
	if (in) {
		char tgaHeader[18]; // short tgaHeader[9];
		fread(tgaHeader, sizeof(tgaHeader), 1, in);
		int w = (int) *((short *) (tgaHeader+12));
		int h = (int) *((short *) (tgaHeader+14));
		// allocate, read pixels
		*width = w;
		*height = h;
		int bitsPerPixel = tgaHeader[16];
		int bytesPP = bitsPerPixel/8;
		if (bytesPerPixel)
			*bytesPerPixel = bytesPP;
		int bytesPerImage = w*h*(bytesPP);
		unsigned char *pixels = new unsigned char[bytesPerImage];
		fread(pixels, bytesPerImage, 1, in);
		fclose(in);
		return pixels;
	}
	printf("can't open %s\n", filename);
	return NULL;
}

bool TargaSize(const char *filename, int &width, int &height) {
	FILE *in = fopen(filename, "rb");
	if (in) {
		char tgaHeader[18];
		fread(tgaHeader, sizeof(tgaHeader), 1, in);
		width = (int) *((short *) (tgaHeader+12));
		height = (int) *((short *) (tgaHeader+14));
		fclose(in);
		return true;
	}
	return false;
}

bool WriteTarga(const char *filename, unsigned char *pixels, int width, int height) {
	FILE *out = fopen(filename, "wb");
	if (!out) {
		printf("can't save %s\n", filename);
		return false;
	}
	short tgaHeader[9];
	tgaHeader[0] = 0;
	tgaHeader[1] = 2;
	tgaHeader[2] = 0;
	tgaHeader[3] = 0;
	tgaHeader[4] = 0;
	tgaHeader[5] = 0;
	tgaHeader[6] = width;
	tgaHeader[7] = height;
	tgaHeader[8] = 24; // *** assumed bits per pixel
	fwrite(tgaHeader, sizeof(tgaHeader), 1, out);
	fwrite(pixels, 3*width*height, 1, out);
	fclose(out);
	return true;
}

bool WriteTarga(const char *filename) {
	int width, height;
	ViewportSize(width, height);
	int npixels = width*height;
	unsigned char *cPixels = new unsigned char[3*npixels], *c = cPixels;
	float *fPixels = new float[3*npixels*sizeof(float)], *f = fPixels;
	glReadPixels(0, 0, width, height, GL_BGR, GL_FLOAT, fPixels);   // Targa is BGR ordered
	for (int i = 0; i < npixels; i++) {
		*c++ = (unsigned char) (255.f*(*f++));
		*c++ = (unsigned char) (255.f*(*f++));
		*c++ = (unsigned char) (255.f*(*f++));
	}
	bool ok = WriteTarga(filename, (unsigned char *) cPixels, width, height);
	delete [] fPixels;
	delete [] cPixels;
	return ok;
}
*/
