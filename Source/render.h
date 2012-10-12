// Copyright 2012 The Ephenation Authors
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

#pragma once

#include <vector>

#include <glm/glm.hpp>

#define RENDER_Y_VIEWING_ANGLE 60.0f

class StageOneShader;

class Image;

#define MAXRENDERDISTANCE 160.0f // Don't allow a value above this
extern float maxRenderDistance;

enum DL_Type {
	DL_OnlyTransparent,	// Only draw blocks that are semi transparent
	DL_NoTransparent,	// Only draw blocks that are solid
	DL_Picking			// Picking mode, draw blocks that can be picked in build mode
};

// Draw all the blocks in the world
void DrawLandscape(StageOneShader *shader, DL_Type);

// Like DrawLandscape, but used for finding shadows. Draw from top to bottom.
// If 'forceload' is true, then missing chunks will be requested to be loaded.
void DrawLandscapeTopDown(StageOneShader *shader, int width, int height, bool forceload = true, DL_Type dlType = DL_NoTransparent);

// Special version of the chunk drawing that draws the chunks that may cast a shadow.
void DrawLandscapeForShadows(StageOneShader *shader);

void ComputeRelativeChunksSortedDistances();

extern float _angleHor;
extern float _angleVert;

// Manage a list of all objects that shall be drawn. The list is constructed dymanically during drawing of the chunk,
// but the obejects are not drawn until after the chunk. Some of them require special shaders, and some of them has
// special effects on lighting. The list is cleared and restarted every frame.
struct DrawObjectList {
	glm::vec3 pos;
	unsigned char type;
	DrawObjectList(glm::vec3 p, unsigned char t) : pos(p), type(t) {}
};

extern std::vector<DrawObjectList> gDrawObjectList;

#define MAX_RENDER_SHADOWS 200
#define MAX_RENDER_FOGS 100

// A class that manage shadows beneath monsters and players.
class Shadows {
public:
	void Clear(void); // Clear the list
	// Add another light source to the list. Optional 'limit' will limit how much of the maximal buffer that is used.
	void Add(float x, float y, float z, float radius, float limit = 1.0);
	Shadows() { this->Clear(); }
	int GetCount(void) const { return fNumShadows; }
	glm::vec4 *GetList(void) {
		return &fShadows[0];
	}
private:
	int fNumShadows;
	glm::vec4 fShadows[MAX_RENDER_SHADOWS];
};

extern Shadows gShadows;

// A class that manage shadows beneath monsters and players.
class Fogs {
public:
	void Clear(void); // Clear the list
	// Add another light source to the list. Optional 'limit' will limit how much of the maximal buffer that is used.
	void Add(float x, float y, float z, int radius, float ambient);
	Fogs() { this->Clear(); }
	int GetCount(void) const { return fNumFogs; }
	glm::vec4 *GetList(void) {
		return &fFogs[0];
	}
private:
	int fNumFogs;
	glm::vec4 fFogs[MAX_RENDER_SHADOWS];
};

extern Fogs gFogs;
