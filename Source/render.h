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

class BaseObject {
public:
	// Clear the list
	void Clear() { fItems.clear(); }
	int GetCount(void) const { return fItems.size(); }
	glm::vec4 *GetList(void) {
		if (fItems.size() == 0)
			return 0;
		return &fItems[0];
	}
protected:
	std::vector<glm::vec4> fItems;
};

#define MAX_RENDER_SHADOWS 200
#define MAX_RENDER_FOGS 100

// A class that manage special lights.
class LightSources : public BaseObject {
public:
	// Add another light source to the list.
	void AddRed(float x, float y, float z, float radius);
	void AddGreen(float x, float y, float z, float radius);
	void AddBlue(float x, float y, float z, float radius);
private:
	// Add an offset to the radius, coded in the 4:th component, to identify what color it is
	enum { RedOffset = 100, GreenOffset = 200, BlueOffset = 300 };
};

extern LightSources gLightSources;

// A class that manage shadows beneath monsters and players.
class Shadows : public BaseObject {
public:
	// Add another light source to the list. Optional 'limit' will limit how much of the maximal buffer that is used.
	void Add(float x, float y, float z, float radius, float limit = 1.0);
};

extern Shadows gShadows;

// A class that manage shadows beneath monsters and players.
class Fogs : public BaseObject {
public:
	// Add another light source to the list. Optional 'limit' will limit how much of the maximal buffer that is used.
	void Add(float x, float y, float z, int radius, float ambient);
};

extern Fogs gFogs;
