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

//
// A class to manage buffers and shaders for the deferred lighting.
// The class is used for several stages:
// 1. A Frame Buffer Object to use as a target for the first rendering.
// 2. A shader to use for the second rendering phase, from the FBO to the screen.
// 3. Create a square used for projecting the final result.
//

#include <glbinding/gl/types.h>

#include "shader.h"

using gl::GLvoid;

/// Compute a bitmap with downsampled luminance
class DownSamplingLuminance : public ShaderBase {
public:
	DownSamplingLuminance();

	// Initialize the FBO to a specified size, and allocate buffers.
	void Init(void);

	void EnableProgram(void);
	void DisableProgram(void);

	void VertexAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer); // Always 3 components.

	void Draw();
private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);
};
