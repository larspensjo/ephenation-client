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

#include "../primitives.h"

class ChunkShader;

// Class for creating a cube. The result is a list of triangles.
class Cube {
public:
	Cube();
	~Cube();
	// Create a list of triangles representing a cube. The size is a unit size.
	void Init(ChunkShader *shader);
	void Draw(void) const; // Draw a cube, using the currently bound texture0.
	void DrawLines(void) const; // Only draw the edges of the cube.
private:
	std::vector<TriangleSurfacef> fVisibleTriangles;
	GLuint fBufferId;
	GLuint fVao; // Vertex Attribute Object
};

extern Cube gLantern;
