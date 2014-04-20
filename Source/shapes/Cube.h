// Copyright 2012-2014 The Ephenation Authors
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
#include "../OpenglBuffer.h"

/// Class for drawing a cube with a stage 1 shader.
class Cube {
public:
	~Cube();
	// Create a list of triangles representing a cube. The size is a unit size.
	void Init(bool invertedNormals = false);
	void Draw(void) const; // Draw a cube, using the currently bound texture0.
	void DrawLines(void) const; // Only draw the edges of the cube.
private:
	std::vector<TriangleSurfacef> fVisibleTriangles;
	OpenglBuffer fOpenglBuffer;
	GLuint fVao = 0; // Vertex Attribute Object
};

/// The lantern is special as it has normals pointing inwards.
extern Cube gLantern;
