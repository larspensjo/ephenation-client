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
// A shader program that will draw chunks for picking.
// This is a singleton class.
//

#include "shader.h"

class ChunkShaderPicking : ShaderBase {
public:
	static ChunkShaderPicking *Make(void);
	void EnableProgram(void);
	void DisableProgram(void);

	void Model(const glm::mat4 &);// Define the Model matrix
	void View(const glm::mat4 &);// Define the View matrix
	void Projection(const glm::mat4 &);// Define the projection matrix

	// Define memory layout for data. A buffer must be bound to do this.
	void VertexAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer); // Always 3 components.
	void NormalAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer); // Always 3 components

	void EnableVertexAttribArray(void);
private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);
	ChunkShaderPicking(); // Only allow access through the maker.
	virtual ~ChunkShaderPicking(); // Don't allow destruction as this is a singleton.
	static ChunkShaderPicking fgSingleton; // This is the singleton instance
	static const GLchar *fVertexShaderSource[];
	static const GLchar *fFragmentShaderSource[];

	GLint fProjectionMatrixIndex;
	GLint fViewMatrixIndex;
	GLint fModelMatrixIndex;

	GLint fNormalIndex;
	GLint fVertexIndex;
};
