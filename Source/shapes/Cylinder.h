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

class StageOneShader;

// Class for creating a cylinder. The result is a list of triangles
class Cylinder {
public:
	~Cylinder();
	// Create a list of triangles representing a cylinder. The size is a unit size, but transformed using 'transf'.
	// Argument is number of squares used to make the cylinder
	void Init(StageOneShader *shader, int numSegments);
	void Draw(StageOneShader *shader) const;
private:
	std::vector<TriangleSurfacef> fVisibleTriangles; // One list for each block type
	OpenglBuffer fOpenglBuffer;
	GLuint fVao = 0;
};
