// Copyright 2014 The Ephenation Authors
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

// Shader used for drawing a texture, and not much else.

#include "shader.h"

namespace Shaders {

class BarrelDistortion : ShaderBase {
public:
	static BarrelDistortion *Make(void);
	void EnableProgram(void);
	void DisableProgram(void);
	void ModelView(const glm::mat4 &); // Define the ModelView matrix
	// Define memory layout for the vertices. A buffer must be bound to do this.
	void VertexAttribPointer(GLenum type, GLint size, GLsizei stride, const GLvoid * pointer);
	// Define memory layout for the textures. A buffer must be bound to do this.
	void TextureAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer);

	void EnableVertexAttribArray(void);
private:
	// Define all uniform and attribute indices.
	virtual void GetLocations(void);
	static BarrelDistortion fgSingleton; // This is the singleton instance
	GLint fgVertexIndex = -1, fgTexCoordIndex = -1, fModelViewMatrixIndex = -1;
	bool fInitialized = false;
};

}
