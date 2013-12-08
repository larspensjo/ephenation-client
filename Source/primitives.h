// Copyright 2012-2013 The Ephenation Authors
//
// This file is part of Ephenation.
//
// Ephenation is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// Ephenation is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Ephenation.  If not, see <http://www.gnu.org/licenses/>.
//

/// @file primitives.h
/// Manage various global types and variables.
/// @todo The number of global variables should be minimized.

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "assert.h"

/// Help to encode data into the three RGB bytes.
/// The data is used in picking mode to identify chunk (relative to the player), coordinates in chunk and the facing direction.
union PickingData {
	unsigned char rgb[3];
	struct {
		unsigned int x:5, y:5, z:5, facing:3, dx:2, dy:2, dz:2;
	} bitmap;
};

#define VERTEXSCALING 1000
#define NORMALSCALING 127
#define TEXTURESCALING 100

/// Vertex data used for the first phase of the deferred shading.
/// A voxel based system
/// use a lot of data, so it is important to pack it well. The current implementation uses only 20 bytes,
/// which includes data for:
/// * Vertex coordinate
/// * Normal
/// * Texture coordiate
/// * Light intensity
/// * Ambient light
/// Specifically, the vertex coordinate can be packed efficiently as it is not the world absolute coordinate. It is the coordinate
/// relative to the chunk that the player is in.
struct VertexDataf {
private:
	static float fract(float f) { return f; }
	static char ConvertNormal(float f) { int ret = int(floorf(f*NORMALSCALING+0.5f)); ASSERT(ret <= 127 && ret >= -128); return ret; }
	static short ConvertVertex(float f) { int ret = short(floorf(f*VERTEXSCALING+0.5f)); /* ASSERT(ret <= 32767 && ret >= -32786);*/ return ret; }
	static char ConvertTexture(float t) { int ret = char(floorf(fract(t)*TEXTURESCALING+0.5f)); ASSERT(ret <= 127 && ret >= -128); return ret;}
	/// The normal vector is coded into the first 3 bytes.
	/// 4:th byte is used for sun intensity and ambient light. Bit 0 to 3 is sun intensity, 4 to 7 is ambient light
	char fNormal[4];
	short fVertex[3];
	char fTexture[2];
public:
	void SetTexture(float x, float y) { fTexture[0] = ConvertTexture(x); fTexture[1] = ConvertTexture(y); }
	void SetTexture(const glm::vec2 &texture) { this->SetTexture(texture.x, texture.y); }
	glm::vec2 GetTexture(void) const { return glm::vec2(fTexture[0]/TEXTURESCALING, fTexture[1]/TEXTURESCALING); }
	static void *GetTextureOffset(void) { VertexDataf *p = 0; return &p->fTexture[0]; }
	/// Take a value from 0 to 255
	void SetIntensity(unsigned char intensity) { fNormal[3] = ((intensity>>4) & 0x0F) | (unsigned(fNormal[3]) & 0xF0); }
	static void *GetIntensityOffset(void) { VertexDataf *p = 0; return &p->fNormal[3]; }
	unsigned char GetIntensity(void) const { return (unsigned(fNormal[3]) & 0x0F) << 4; }
	/// Take a value from 0 to 255
	void SetAmbient(unsigned char ambient) { fNormal[3] = (unsigned(fNormal[3]) & 0x0F) | (ambient & 0xF0); }
	/// Setting normals for picking mode. We want to transfer data as it is, with no scaling.
	void SetNormal(PickingData data) { fNormal[0] = data.rgb[0]; fNormal[1] = data.rgb[1]; fNormal[2] = data.rgb[2]; }
	void SetNormal(const glm::vec3 &n) { fNormal[0] = ConvertNormal(n.x); fNormal[1] = ConvertNormal(n.y); fNormal[2] = ConvertNormal(n.z); }
	glm::vec3 GetNormal(void) const { return glm::vec3(float(fNormal[0])/NORMALSCALING, float(fNormal[1])/NORMALSCALING, float(fNormal[2])/NORMALSCALING); }
	static void *GetNormalOffset(void) { VertexDataf *p = 0; return &p->fNormal[0]; }

	void SetVertex(float x, float y, float z) {
		fVertex[0] = ConvertVertex(x); fVertex[1] = ConvertVertex(y); fVertex[2] = ConvertVertex(z);
	}
	void SetVertex(const glm::vec3 &v) {
		this->SetVertex(v.x, v.y, v.z);
	}
	glm::vec3 GetVertex() {
		return glm::vec3(float(fVertex[0])/VERTEXSCALING, float(fVertex[1])/VERTEXSCALING, float(fVertex[2])/VERTEXSCALING);
	}
	/// Used to make it possible to sort, based on vertex coordinates.
	bool LessVertex(const VertexDataf *other) {
		if (fVertex[0] < other->fVertex[0])
			return true;
		if (fVertex[0] > other->fVertex[0])
			return false;
		if (fVertex[1] < other->fVertex[1])
			return true;
		if (fVertex[1] > other->fVertex[1])
			return false;
		if (fVertex[2] < other->fVertex[2])
			return true;
		return false;
	}
	static void *GetVertexOffset(void) { VertexDataf *p = 0; return &p->fVertex[0]; }
	VertexDataf(const glm::vec3 &normal, const glm::vec2 &texture, const glm::vec3 vertex, unsigned char intensity, unsigned char ambient) {
		this->SetVertex(vertex);
		this->SetNormal(normal);
		this->SetAmbient(ambient);
		this->SetIntensity(intensity);
		this->SetTexture(texture);
	}
	VertexDataf() {}
};

struct TriangleSurfacef {
	VertexDataf v[3];
};

/// If 'ignore', then no check will be done to save execution time
void checkError(const char *functionName, bool ignore = true);
void dumpGraphicsMemoryStats(void);
void DumpTriangles(TriangleSurfacef *t, int num);
const char *FrameBufferError(unsigned error);

extern glm::mat4 gProjectionMatrix;
extern glm::mat4 gViewMatrix; /// Store the view matrix
extern glm::vec4 gViewport; /// The current viewport
extern float gDesktopAspectRatio;
extern int gDrawnQuads;
extern int gNumDraw;
extern int gDebugOpenGL;
extern int gVerbose;
extern bool gShowFramework;

/// Updated once every frame.
///Time accuracy will thus be lower, but it will save performance to use this instead of glfwGetTime().
extern double gCurrentFrameTime;
extern double gLastPing, gCurrentPing;
extern bool gShowPing;
extern bool gAdminTP; /// Admin can enter TP mode anytime.
extern int gIgnoreOpenGLErrors; /// Don't report errors
extern bool gToggleTesting; /// Used for SW development

/// List of textures that can be shown for debugging.
struct DebugTexture {
	DebugTexture(unsigned id, const char *comment) : id(id), comment(comment) {}
	unsigned id;
	const char *comment;
};
extern std::vector<DebugTexture> gDebugTextures;

/// Convert an argument to a string. This two-step approach is required for #defined values,
/// or the argument name is given as a string instead of the value of the argument.
#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
