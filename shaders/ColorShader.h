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

#include "shader.h"

class ColorShader : ShaderBase {
public:
	static ColorShader *Make(void);
	void EnableProgram(void);
	void DisableProgram(void);
	void ModelView(const glm::mat4 &);  // Define the ModelView matrix
	void Color(const glm::vec4 &);      // Define the uniform color to use
	void Projection(const glm::mat4 &); // Define the projection matrix
	// Define memory layout for the vertices. A buffer must be bound to do this.
	void VertexAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer);

	void EnableVertexAttribArray(void);
private:
	// Define all uniform and attribute indices.
	virtual void GetLocations(void);
	ColorShader(); // Only allow access through the maker.
	virtual ~ColorShader();
	static ColorShader fgSingleton; // This is the singleton instance
	static const GLchar *fVertexShaderSource[];
	static const GLchar *fFragmentShaderSource[];
	GLint fProjectionMatrixIndex, fFirstTextureIndex, fVertexIndex, fColorIndex, fModelViewMatrixIndex;
};
