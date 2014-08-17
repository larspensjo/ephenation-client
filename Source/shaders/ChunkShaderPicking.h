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

#include <glbinding/gl/types.h>

#include "shader.h"

using gl33::GLvoid;
using gl33::GLint;
using gl33::GLsizei;
using gl33::GLenum;

/// A shader program that will draw chunks for picking.
class ChunkShaderPicking : ShaderBase {
public:
	ChunkShaderPicking();

	/// Must be initialized before it is used.
	void Init();

	/// Enable the shader program before running it or updating variables
	void EnableProgram(void);
	void DisableProgram(void);

	/// Define the Model matrix
	void Model(const glm::mat4 &);

	/// Define memory layout for vertex data. A buffer must be bound to do this.
	/// There are lways 3 components.
	void VertexAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer);

	/// Define memory layout for normal data. A buffer must be bound to do this.
	/// There are lways 3 components.
	void NormalAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer);

	/// Enable the vertex attrib. This must be done when there is a VAO.
	void EnableVertexAttribArray(void);
private:
	/// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);
	static const GLchar *fVertexShaderSource[];
	static const GLchar *fFragmentShaderSource[];

	GLint fModelMatrixIndex;

	GLint fNormalIndex;
	GLint fVertexIndex;
};

/// A global instance, to be used everywhere
extern ChunkShaderPicking gChunkShaderPicking;
