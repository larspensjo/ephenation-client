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

#include <glm/glm.hpp>

// Shader used for drawing a texture, and not much else.

#include "shader.h"

namespace Shaders {

/// A shader that implements a barrell distortion
class BarrelDistortion : ShaderBase {
public:
	static BarrelDistortion *Make(void);
	void EnableProgram(void);
	void DisableProgram(void);

	/// Define the ModelView matrix
	void ModelView(const glm::mat4 &);

	/// Set the constants that control the effects
	void SetOVRConstants(const float *dist, float lensCenter);

	/// Define memory layout for the vertices. A buffer must be bound to do this.
	void VertexAttribPointer(GLenum type, GLint size, GLsizei stride, const GLvoid * pointer);

	/// Define memory layout for the textures. A buffer must be bound to do this.
	void TextureAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer);

	void EnableVertexAttribArray(void);

	void ConfigureEye(bool left = true);
private:
	// Define all uniform and attribute indices.
	virtual void GetLocations(void) override;
	static BarrelDistortion fgSingleton; // This is the singleton instance
	GLint fgVertexIndex = -1, fgTexCoordIndex = -1, fModelViewMatrixIndex = -1, fOVRDistortionIdx = -1, fLensCenterIdx = -1;
	bool fInitialized = false;
	glm::vec2 fOvrLensCenter = glm::vec2(0.15f, 0.0f); // Best guess.
};

}
