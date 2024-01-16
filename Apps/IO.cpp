// IO.cpp (c) 2019-2022 Jules Bloomenthal

#include "Draw.h"
#include "IO.h"
#include <fstream>
#include <string.h>

using std::string;
using std::vector;
using std::ios;
using std::ifstream;

// Texture

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

void LoadTexture(unsigned char *pixels, int width, int height, int bpp, unsigned int textureName, bool bgr, bool mipmap) {
	unsigned char *temp = pixels;
	if (false && bpp == 4) {
		int bytesPerImage = 3*width*height;
		temp = new unsigned char[bytesPerImage];
		unsigned char *p = pixels, *t = temp;
		for (int j = 0; j < height; j++)
			for (int i = 0; i < width; i++, p++)
				for (int k = 0; k < 3; k++)
					*t++ = *p++;
	}
	glBindTexture(GL_TEXTURE_2D, textureName);      // bind current texture to textureName
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);          // accommodate width not multiple of 4
	// specify target, format, dimension, transfer data
	if (bpp == 4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, bgr? GL_BGR : GL_RGB, GL_UNSIGNED_BYTE, temp);
	if (mipmap) {
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			// average bounding pixels of bounding mipmaps - should be default but sometimes needed (else alias)
	}
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// if (bpp == 4) delete [] temp;
	glBindTexture(GL_TEXTURE_2D, 0); // **** ???
}

GLuint LoadTexture(unsigned char *pixels, int width, int height, int bpp, bool bgr, bool mipmap) {
	GLuint textureName = 0;
	// allocate GPU texture buffer; copy, free pixels
	glGenTextures(1, &textureName);
	LoadTexture(pixels, width, height, bpp, textureName, bgr, mipmap);
	return textureName;
}

GLuint ReadTexture(const char *filename, bool mipmap, int *n, int *w, int *h) {
	int width, height, nChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename, &width, &height, &nChannels, 0);
	if (!data) {
		printf("ReadTexture: can't open %s (%s)\n", filename, stbi_failure_reason());
		return 0;
	}
	if (n) *n = nChannels;
	if (w) *w = width;
	if (h) *h = height;
	GLuint textureName = 0;
	glGenTextures(1, &textureName);
	LoadTexture(data, width, height, nChannels, textureName, false, mipmap);
	stbi_image_free(data);
	return textureName;
}

unsigned char *GetData(int &width, int &height) {
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
	delete [] fPixels;
	return cPixels;
}

void SavePng(const char *filename) {
	int width, height;
	unsigned char *cPixels = GetData(width, height);
	stbi_write_png(filename, width, height, 3, cPixels, width*3);
	delete [] cPixels;
}

void SaveBmp(const char *filename) {
	int width, height;
	unsigned char *cPixels = GetData(width, height);
	stbi_write_bmp(filename, width, height, 3, cPixels);
	delete [] cPixels;
}

void SaveTga(const char *filename) {
	int width, height;
	unsigned char *cPixels = GetData(width, height);
	stbi_write_tga(filename, width, height, 3, cPixels);
	delete [] cPixels;
}

int ReadGIF(const char *filename, vector<GLuint> &textureNames, int *nChannels, vector<float> *frameDurations) {
	FILE *f = fopen(filename, "rb");
	if (!f) {
		printf("can't open %s\n", filename);
		return false;
	}
	stbi__context s;
	stbi__start_file(&s, f);
	if (!stbi__gif_test(&s)) {
		printf("%s not GIF format\n", filename);
		return 0;
	}
	int width = 0, height = 0, nFrames = 0, nChan = 0;
	int *delays; // delay[i] is display time for frame[i], in 1/100ths (or 1/1000?) of a second
	unsigned char *pdata = (unsigned char *) stbi__load_gif_main(&s, &delays, &width, &height, &nFrames, &nChan, 0);
	fclose(f);
	if (!pdata) {
		printf("error reading %s (%s)\n", filename, stbi_failure_reason());
		return 0;
	}
	if (nChannels)
		*nChannels = nChan;
	if (frameDurations) {
		frameDurations->resize(nFrames);
		for (int i = 0; i < nFrames; i++)
			(*frameDurations)[i] = (float) delays[i]/1000;
	}
	stbi__vertical_flip_slices(pdata, width, height, nFrames, nChan);
	textureNames.resize(nFrames);
	glGenTextures(nFrames, textureNames.data());
	for (int i = 0; i < nFrames; i++) {
		unsigned char *p = pdata+i*width*height*(*nChannels);
		LoadTexture(p, width, height, *nChannels, textureNames[i], false, true);
	}
	stbi_image_free(pdata);
	return nFrames;
}

// Normals

void SetVertexNormals(vector<vec3> &points, vector<int3> &triangles, vector<vec3> &normals) {
	// size normals array and initialize to zero
	int nverts = (int) points.size();
	normals.resize(nverts, vec3(0, 0, 0));
	// accumulate each triangle normal into its three vertex normals
	for (int i = 0; i < (int) triangles.size(); i++) {
		int3 &t = triangles[i];
		vec3 &p1 = points[t.i1], &p2 = points[t.i2], &p3 = points[t.i3];
		vec3 n(normalize(cross(p2-p1, p3-p2)));
		normals[t.i1] += n;
		normals[t.i2] += n;
		normals[t.i3] += n;
	}
	for (int i = 0; i < nverts; i++)
		normals[i] = normalize(normals[i]);
}

// Standardize

mat4 NDCfromMinMax(vec3 min, vec3 max, float scale = 1) {
	// matrix to transform min/max to -1/+1 (uniformly)
	float maxrange = 0;
	for (int k = 0; k < 3; k++)
		if ((max[k]-min[k]) > maxrange)
			maxrange = max[k]-min[k];
	float s = scale*2.f/maxrange;
	vec3 center = .5f*(max+min);
	return Scale(s)*Translate(-center);
}

mat4 StandardizeMat(vec3 *points, int npoints, float scale) {
	vec3 min, max;
	Bounds(points, npoints, min, max);
	return NDCfromMinMax(min, max, scale);
}

void Standardize(vec3 *points, int npoints, float scale) {
	mat4 m = StandardizeMat(points, npoints, scale);
	for (int i = 0; i < npoints; i++)
		points[i] = Vec3(m*vec4(points[i]));
}

// ASCII support

bool ReadWord(char* &ptr, char *word, int charLimit) {
	ptr += strspn(ptr, " \t");                  // skip white space
	int nChars = (int) strcspn(ptr, " \t");     // get # non-white-space characters
	if (!nChars)
		return false;                           // no non-space characters
	int nRead = charLimit-1 < nChars? charLimit-1 : nChars;
	strncpy(word, ptr, nRead);
	word[nRead] = 0;                            // strncpy does not null terminate
	ptr += nChars;
	return true;
}

// STL

char *Lower(char *word) {
	for (char *c = word; *c; c++)
		*c = tolower(*c);
	return word;
}

bool ReadSTL(const char *filename, vector<vec3> &points, vector<vec3> &normals, vector<int3> &triangles) {
	vector<VertexSTL> vertices;
	int nTriangles = ReadSTL(filename, vertices);
	if (vertices.size() != 3*nTriangles) printf("%i triangles, %i vertices\n", nTriangles, (int) vertices.size());
	triangles.resize(nTriangles);
	for (int i = 0; i < nTriangles; i++)
		triangles[i] = int3(3*i, 3*i+1, 3*i+2);
	points.resize(3*nTriangles);
	normals.resize(3*nTriangles);
	for (int i = 0; i < 3*nTriangles; i++) {
		VertexSTL v = vertices[i];
		points[i] = v.point;
		normals[i] = v.normal;
	}
	return nTriangles > 0;
}

int ReadSTL(const char *filename, vector<VertexSTL> &vertices) {
	// the facet normal should point outwards from the solid object; if this is zero,
	// most software will calculate a normal from the ordered triangle vertices using the right-hand rule
	class Helper {
	public:
		bool status = false;
		int nTriangles = 0;
		vector<VertexSTL> *verts;
		vector<string> vSpecs;                              // ASCII only
		Helper(const char *filename, vector<VertexSTL> *verts) : verts(verts) {
			char line[1000], word[1000], *ptr = line;
			ifstream inText(filename, ios::in);             // text default mode
			inText.getline(line, 10);
			bool ascii = ReadWord(ptr, word, 10) && !strcmp(Lower(word), "solid");
			// bool ascii = ReadWord(ptr, word, 10) && !_stricmp(word, "solid");
			ascii = false; // hmm!
			if (ascii)
				status = ReadASCII(inText);
			inText.close();
			if (!ascii) {
				FILE *inBinary = fopen(filename, "rb");     // inText.setmode(ios::binary) fails
				if (inBinary) {
					nTriangles = 0;
					status = ReadBinary(inBinary);
					fclose(inBinary);
				}
				else
					status = false;
			}
		}
		bool ReadASCII(ifstream &in) {
			printf("can't read ASCII STL\n");
			return true;
		}
		bool ReadBinary(FILE *in) {
				  // # bytes      use                  significance
				  // -------      ---                  ------------
				  //      80      header               none
				  //       4      unsigned long int    number of triangles
				  //      12      3 floats             triangle normal
				  //      12      3 floats             x,y,z for vertex 1
				  //      12      3 floats             vertex 2
				  //      12      3 floats             vertex 3
				  //       2      unsigned short int   attribute (0)
				  // endianness is assumed to be little endian
			// in.setmode(ios::binary); doc says setmode good, but compiler says no
			// sizeof(bool)=1, sizeof(char)=1, sizeof(short)=2, sizeof(int)=4, sizeof(float)=4
			char buf[81];
			int nTriangle = 0;
			if (fread(buf, 1, 80, in) != 80) // header
				return false;
			if (fread(&nTriangles, sizeof(int), 1, in) != 1)
				return false;
			while (!feof(in)) {
				vec3 v[3], n;
				if (nTriangle == nTriangles)
					break;
			//	if (nTriangles > 5000 && nTriangle && nTriangle%1000 == 0)
			//		printf("\rread %i/%i triangles", nTriangle, nTriangles);
				if (fread(&n.x, sizeof(float), 3, in) != 3)
					printf("\ncan't read triangle %d normal\n", nTriangle);
				for (int k = 0; k < 3; k++)
					if (fread(&v[k].x, sizeof(float), 3, in) != 3)
						printf("\ncan't read vid %i\n", (int) verts->size());
				vec3 a(v[1]-v[0]), b(v[2]-v[1]);
				vec3 ntmp = cross(a, b);
				if (dot(ntmp, n) < 0) {
					vec3 vtmp = v[0];
					v[0] = v[2];
					v[2] = vtmp;
				}
				for (int k = 0; k < 3; k++)
					verts->push_back(VertexSTL((float *) &v[k].x, (float *) &n.x));
				unsigned short attribute;
				if (fread(&attribute, sizeof(short), 1, in) != 1)
					printf("\ncan't read attribute\n");
				nTriangle++;
			}
			printf("\r\t\t\t\t\t\t\r");
			return true;
		}
	};
	Helper h(filename, &vertices);
	return h.nTriangles;
} // end ReadSTL

// ASCII OBJ

#include <map>

static const int LineLim = 10000, WordLim = 1000;

struct CompareS { bool operator() (const string &a, const string &b) const { return (a < b); } };

typedef std::map<string, Mtl, CompareS> MtlMap;
	// string is key, Mtl is value

MtlMap ReadMaterial(const char *filename) {
	MtlMap mtlMap;
	char line[LineLim], word[WordLim];
	Mtl m;
	FILE *in = fopen(filename, "r");
	string key;
	Mtl value;
	if (in)
		for (int lineNum = 0;; lineNum++) {
			line[0] = 0;
			fgets(line, LineLim, in);                   // \ line continuation not supported
			if (feof(in))                               // hit end of file
				break;
			if (strlen(line) >= LineLim-1) {            // getline reads LineLim-1 max
				printf("line %d too long\n", lineNum);
				continue;
			}
			line[strlen(line)-1] = 0;							// remove carriage-return
			char *ptr = line;
			if (!ReadWord(ptr, word, WordLim) || *word == '#')
				continue;
			Lower(word);
			if (!strcmp(word, "newmtl") && ReadWord(ptr, word, WordLim)) {
				key = string(word);
				value.name = string(word);
			}
			if (!strcmp(word, "kd")) {
				if (sscanf(ptr, "%g%g%g", &value.kd.x, &value.kd.y, &value.kd.z) != 3)
					printf("bad line %d in material file", lineNum);
				else
					mtlMap[key] = value;
			}
		}
	// else printf("can't open %s\n", filename);
	return mtlMap;
}

struct CompareVid {
	bool operator() (const int3 &a, const int3 &b) const {
		return (a.i1==b.i1? (a.i2==b.i2? a.i3 < b.i3 : a.i2 < b.i2) : a.i1 < b.i1);
	}
};

typedef std::map<int3, int, CompareVid> VidMap;
	// int3 is key, int is value

bool ReadAsciiObj(const char      *filename,
				  vector<vec3>    &points,
				  vector<int3>    &triangles,
				  vector<vec3>    *normals,
				  vector<vec2>    *textures,
				  vector<Group>   *triangleGroups,
				  vector<Mtl>     *triangleMtls,
				  vector<int4>    *quads,
				  vector<int2>	  *segs) {
	// read 'object' file (Alias/Wavefront .obj format); return true if successful;
	// polygons are assumed simple (ie, no holes and not self-intersecting);
	// some file attributes are not supported by this implementation;
	// obj format indexes vertices from 1
	FILE *in = fopen(filename, "r");
	int nVlines = 0, nNlines = 0, nTlines = 0, nFlines = 0;
	if (!in)
		return false;
	vec2 t;
	vec3 v;
	int group = 0;
	char line[LineLim], word[WordLim];
	bool hashedTriangles = false;	// true if any triangle vertex specified with different point/normal/texture id
	bool hashedVertices = false;	// true if point/normal/texture arrays different (non-zero) size
	vector<vec3> tmpVertices, tmpNormals;
	vector<vec2> tmpTextures;
	VidMap vidMap;
	MtlMap mtlMap;
	int nQuadsConvertedToTris = 0;
	for (int lineNum = 0;; lineNum++) {
		line[0] = 0;
		fgets(line, LineLim, in);                           // \ line continuation not supported
		if (feof(in))                                       // hit end of file
			break;
		if (strlen(line) >= LineLim-1) {                    // getline reads LineLim-1 max
			printf("line %d too long\n", lineNum);
			return false;
		}
		line[strlen(line)-1] = 0;							// remove carriage-return
		char *ptr = line;
		if (!ReadWord(ptr, word, WordLim))
			continue;
		Lower(word);
		if (*word == '#') {
			continue;
		}
		else if (!strcmp(word, "mtllib")) {
			if (ReadWord(ptr, word, WordLim)) {
				char name[100];
				const char *p = strrchr(filename, '/'); //-filename, count = 0;
				if (p) {
					size_t nchars = p-filename;
					strncpy(name, filename, nchars+1);
					name[nchars+1] = 0;
					strcat(name, word);
				}
				else
					strcpy(name, word);
				mtlMap = ReadMaterial(name);
				if (false) {
					int count = 0;
					for (MtlMap::iterator iter = mtlMap.begin(); iter != mtlMap.end(); iter++) {
						string s = (string) iter->first;
						Mtl m = (Mtl) iter->second;
						printf("m[%i].name=%s,.kd=(%3.2f,%3.2f,%3.2f),s=%s\n", count++, m.name.c_str(), m.kd.x, m.kd.y, m.kd.z, s.c_str());
					}
				}					
			}
		}
		else if (!strcmp(word, "usemtl")) {
			if (ReadWord(ptr, word, WordLim)) {
				MtlMap::iterator it = mtlMap.find(string(word));
				if (it == mtlMap.end())
					printf(""); // "no such material: %s\n", word);
				else {
					Mtl m = it->second;
					m.startTriangle = (int) triangles.size();
					if (triangleMtls)
						triangleMtls->push_back(m);
				}
			}
		}
		else if (!strcmp(word, "g")) {
			char *s = strchr(ptr, '(');
			if (s) *s = 0;
			if (triangleGroups) {
				// printf("adding to triangleGroups: name = %s\n", ptr);
				triangleGroups->push_back(Group((int) triangles.size(), string(ptr)));
			}
		}
		else if (!strcmp(word, "v")) {                      // read vertex coordinates
			nVlines++;
			if (sscanf(ptr, "%g%g%g", &v.x, &v.y, &v.z) != 3) {
				printf("bad line %d in object file", lineNum);
				return false;
			}
			tmpVertices.push_back(vec3(v.x, v.y, v.z));
		}
		else if (!strcmp(word, "vn")) {                     // read vertex normal
			nNlines++;
			if (sscanf(ptr, "%g%g%g", &v.x, &v.y, &v.z) != 3) {
				printf("bad line %d in object file", lineNum);
				return false;
			}
			tmpNormals.push_back(vec3(v.x, v.y, v.z));
		}
		else if (!strcmp(word, "vt")) {                     // read vertex texture
			nTlines++;
			if (sscanf(ptr, "%g%g", &t.x, &t.y) != 2) {
				printf("bad line %d in object file", lineNum);
				return false;
			}
			tmpTextures.push_back(vec2(t.x, t.y));
		}
		else if (!strcmp(word, "f")) {                      // read triangle or polygon
			nFlines++;
			size_t nvids = tmpVertices.size(), ntids = tmpTextures.size(), nnids = tmpNormals.size();
			if ((ntids && ntids != nvids) || (nnids && nnids != nvids))
				hashedVertices = true;
			static vector<int> vids;
			vids.resize(0);
			while (ReadWord(ptr, word, WordLim)) {          // read arbitrary # face vid/tid/nid
				// set texture and normal pointers to preceding /
				char *tPtr = strchr(word+1, '/');           // pointer to /, or null if not found
				char *nPtr = tPtr? strchr(tPtr+1, '/') : NULL;
				// use of / is optional (ie, '3' is same as '3/3/3')
				// convert to vid, tid, nid indices (vertex, texture, normal)
				int vid = atoi(word);
				if (!vid)                                   // atoi returns 0 if failure to convert
					break;
				int tid = tPtr && *++tPtr != '/'? atoi(tPtr) : vid;
				int nid = nPtr && *++nPtr != 0? atoi(nPtr) : vid;
				// standard .obj is indexed from 1, mesh indexes from 0
				vid--;
				tid--;
				nid--;
				if (vid < 0 || tid < 0 || nid < 0) {        // atoi conversion failure
					printf("bad format on line %d\n", lineNum);
					break;
				}
				if ((tid >= 0 && tid != vid) || (nid >= 0 && nid != vid))
					hashedTriangles = true;
				bool hashed = hashedVertices || hashedTriangles;
				if (!hashedVertices && !hashedTriangles) {
					vids.push_back(vid);
				}
				if (hashedVertices || hashedTriangles) {
					int3 key(vid, tid, nid);
					VidMap::iterator it = vidMap.find(key);
					// following can fail on early vertices
					// to support OBJ must support triangle vid1/tid1/nid1, vid2/tid2/nid2, vid3/tid3/nid3
					// which would mean changing current implementation
					// instead, test for hashed vertices (ie, has vid ever not equaled tid or nid?)
					// but we add specific test for simple impl
					// need a straightforward implementation for when
					// vid=tid=nid and/or there is no tid, no nid
					// printf("incoming vid = %i, ", vid);
					if (it == vidMap.end()) {
						size_t nvrts = (int) points.size();
						vidMap[key] = nvrts;
						points.push_back(tmpVertices[vid]); // *** suspect
						if (normals && (int) tmpNormals.size() > nid)
							normals->push_back(tmpNormals[nid]);
						if (textures && (int) tmpTextures.size() > tid)
							textures->push_back(tmpTextures[tid]);
						vids.push_back(nvrts);
						// printf("pushed %i\n", nvrts);
					}
					else {
						vids.push_back(it->second);
						// printf("pushed second = %i\n", it->second);
					}
				}
			}
			int nids = (int) vids.size();
			if (nids == 3) {
				int id1 = vids[0], id2 = vids[1], id3 = vids[2];
				if (normals && (int) normals->size() > id1) {
					vec3 p1, p2, p3;
					if (hashedVertices || hashedTriangles) { p1 = points[id1]; p2 = points[id2]; p3 = points[id3]; }
					else { p1 = tmpVertices[id1]; p2 = tmpVertices[id2]; p3 = tmpVertices[id3]; }
					vec3 a(p2-p1), b(p3-p2), n(cross(a, b));
					if (dot(n, (*normals)[id1]) < 0) {
						// reverse triangle order to correspond with vertex normal
						int tmp = id1;
						id1 = id3;
						id3 = tmp;
					}
				}
				// create triangle
				int3 id(id1, id2, id3);
				triangles.push_back(id);
				// printf("triangle[%i] = (%i, %i, %i)\n", triangles.size()-1, id1+1, id2+1, id3+1);
			}
			else if (nids == 4 && quads)
				quads->push_back(int4(vids[0], vids[1], vids[2], vids[3]));
			else if (nids == 2 && segs)
				segs->push_back(int2(vids[0], vids[1]));
			else
				// create polygon as nvids-2 triangles
				for (int i = 1; i < nids-1; i++) {
					triangles.push_back(int3(vids[0], vids[i], vids[(i+1)%nids]));
				}
			if (nids == 4 && !quads) nQuadsConvertedToTris++;
		} // end "f"
		else if (*word == 0 || *word == '\n')               // skip blank line
			continue;
		else {                                              // unrecognized attribute
			// printf("unsupported attribute in object file: %s", word);
			continue; // return false;
		}
	} // end read til end of file
//	if (nQuadsConvertedToTris) printf("(%i quads converted to triangles)\n", nQuadsConvertedToTris);
//	printf("hashedVertices = %s, hashedTriangles = %s\n", hashedVertices? "true" : "false", hashedTriangles? "true" : "false");
	if (!hashedVertices && !hashedTriangles) {
		int nPoints = (int) tmpVertices.size();
		points.resize(nPoints);
		for (int i = 0; i < nPoints; i++)
			points[i] = tmpVertices[i];
		if (normals) {
			int nNormals = (int) tmpNormals.size();
			normals->resize(nNormals);
			for (int i = 0; i < nNormals; i++)
				(*normals)[i] = tmpNormals[i];
		}
		if (textures) {
			int nTextures = (int) tmpTextures.size();
			textures->resize(nTextures);
			for (int i = 0; i < nTextures; i++)
				(*textures)[i] = tmpTextures[i];
		}
	}
	if (triangleGroups) {
		int nGroups = (int) triangleGroups->size();
		for (int i = 0; i < nGroups; i++) {
			int next = i < nGroups-1? (*triangleGroups)[i+1].startTriangle : (int) triangles.size();
			(*triangleGroups)[i].nTriangles = next-(*triangleGroups)[i].startTriangle;
		}
	}
	if (triangleMtls) {
		int nMtls = (int) triangleMtls->size();
		for (int i = 0; i < nMtls; i++) {
			int next = i < nMtls-1? (*triangleMtls)[i+1].startTriangle : (int) triangles.size();
			(*triangleMtls)[i].nTriangles = next-(*triangleMtls)[i].startTriangle;
		}
	}
//	printf("%i vlines, %i nlines, %i tlines, %i flines\n", nVlines, nNlines, nTlines, nFlines);
	return true;
} // end ReadAsciiObj

bool WriteAsciiObj(const char    *filename,
				   vector<vec3>  &points,
				   vector<vec3>  &normals,
				   vector<vec2>  &uvs,
				   vector<int3>  *triangles,
				   vector<int4>  *quads,
				   vector<int2>  *segs,
				   vector<Group> *triangleGroups) {
	FILE *file = fopen(filename, "w");
	if (!file) {
		printf("can't write %s\n", filename);
		return false;
	}
	int nPoints = (int) points.size(), nNormals = (int) normals.size(), nUvs = (int) uvs.size(), nTriangles = triangles? (int) triangles->size() : 0;
	if (nPoints) {
		fprintf(file, "# %i vertices\n", nPoints);
		for (int i = 0; i < nPoints; i++)
			fprintf(file, "v %f %f %f \n", points[i].x, points[i].y, points[i].z);
		fprintf(file, "\n");
	}
	if (nNormals) {
		fprintf(file, "# %i normals\n", nNormals);
		for (int i = 0; i < nNormals; i++)
			fprintf(file, "vn %f %f %f \n", normals[i].x, normals[i].y, normals[i].z);
		fprintf(file, "\n");
	}
	if (nUvs) {
		fprintf(file, "# %i textures\n", nUvs);
		for (int i = 0; i < nUvs; i++)
			fprintf(file, "vt %f %f \n", uvs[i].x, uvs[i].y);
		fprintf(file, "\n");
	}
	// write triangles, quads (adding 1 to all vertex indices per OBJ format)
	if (triangles) {
		// non-grouped triangles
		size_t nNonGrouped = triangleGroups && triangleGroups->size()? (*triangleGroups)[0].startTriangle : nTriangles; 
		if (nTriangles) fprintf(file, "# %i triangles\n", nTriangles);
		for (size_t i = 0; i < nNonGrouped; i++)
			fprintf(file, "f %d %d %d \n", 1+(*triangles)[i].i1, 1+(*triangles)[i].i2, 1+(*triangles)[i].i3);
		if (triangleGroups)
			for (size_t i = 0; i < triangleGroups->size(); i++) {
				Group g = (*triangleGroups)[i];
				if (g.nTriangles) {
					fprintf(file, "g %s (%i triangles)\n", g.name.c_str(), g.nTriangles);
					for (int t = g.startTriangle; t < g.startTriangle+g.nTriangles; t++)
						fprintf(file, "f %d %d %d \n", 1+(*triangles)[t].i1, 1+(*triangles)[t].i2, 1+(*triangles)[t].i3);
				}
			}
	//	else
	//		for (size_t i = 0; i < triangles->size(); i++)
	//			fprintf(file, "f %d %d %d \n", 1+(*triangles)[i].i1, 1+(*triangles)[i].i2, 1+(*triangles)[i].i3);
		fprintf(file, "\n");
	}
	if (quads)
		for (size_t i = 0; i < quads->size(); i++)
			fprintf(file, "f %d %d %d %d \n", 1+(*quads)[i].i1, 1+(*quads)[i].i2, 1+(*quads)[i].i3, 1+(*quads)[i].i4);
	if (segs)
		for (size_t i = 0; i < segs->size(); i++)
			fprintf(file, "f %d %d \n", 1+(*segs)[i].i1, 1+(*segs)[i].i2);
	fclose(file);
	return true;
}
