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

//
// Draw a color on a geometry. The color can either be specified as a single color, or by providing
// a color vertex attribute.
//

#pragma once

#include "shader.h"
#include "../glm/glm.hpp"

class ColorShader : ShaderBase {
public:
	static ColorShader *Make(void);     // Get a reference to the singleton class
	void EnableProgram(void);
	void DisableProgram(void);
	void ModelView(const glm::mat4 &);  // Define the ModelView matrix
	void Color(const glm::vec4 &);      // Define the uniform color to use. Alpha 0 will instead use the color vertex attribute.
	void Projection(const glm::mat4 &); // Define the projection matrix

	// The vertex attribute indices, to enable glEnableVertexAttribArray() and
	// glVertexAttribPointer to be setup elsewhere, as needed.
	// This must match locations used in the shader.
	enum { VERTEX_INDEX = 0, COLOR_INDEX = 1 };
private:
	// Define all uniform and attribute indices.
	virtual void GetLocations(void);
	ColorShader(); // Only allow access through the maker.
	static ColorShader fgSingleton; // This is the singleton instance
	GLint fProjectionMatrixIndex, fColorIndex, fModelViewMatrixIndex;
};
