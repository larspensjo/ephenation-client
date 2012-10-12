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

// Shader used for drawing a texture with modulated colors. The
// color from the texture will be multiplied by a color factor.

#include "shader.h"

class ModulatedTextureShader : ShaderBase {
public:
	ModulatedTextureShader();
	void EnableProgram(void);
	void DisableProgram(void);
	void ModelView(const glm::mat4 &); // Define the ModelView matrix
	void Projection(const glm::mat4 &); // Define the projection matrix
	void Init(void);

	// The vertex attribute indices, for use with glEnableVertexAttribArray() and
	// glVertexAttribPointer() to be setup elsewhere.
	// This must match the locations used in the shader.
	enum { VERTEX_INDEX = 0, TEXTURE_INDEX = 1, COLOR_INDEX = 2 };
private:
	virtual void GetLocations(void); // Define all uniform and attribute indices.
	GLint fgProjectionMatrixIndex, fModelViewMatrixIndex;
};
