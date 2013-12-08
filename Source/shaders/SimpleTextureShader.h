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

// Shader used for drawing a texture, and not much else.

#include "shader.h"

class SimpleTextureShader : ShaderBase {
public:
	static SimpleTextureShader *Make(void);
	void EnableProgram(void);
	void DisableProgram(void);
	void ModelView(const glm::mat4 &); // Define the ModelView matrix
	void Projection(const glm::mat4 &); // Define the projection matrix
	void ForceTransparent(float alpha); // This will change alpha to a lower value for all colors.
	void SetCompensateDistortion(bool flag) { 	glUniform1i(fCompensateDistInd, flag); }
	void TextureOffsetMulti(float offsX, float offsY, float mult);
	// Define memory layout for the vertices. A buffer must be bound to do this.
	void VertexAttribPointer(GLenum type, GLint size, GLsizei stride, const GLvoid * pointer);
	// Define memory layout for the textures. A buffer must be bound to do this.
	void TextureAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer);

	void EnableVertexAttribArray(void);
	void SetColorOffset(const glm::vec3 &);
private:
	// Define all uniform and attribute indices.
	virtual void GetLocations(void);
	SimpleTextureShader(); // Only allow access through the maker.
	static SimpleTextureShader fgSingleton; // This is the singleton instance
	static const GLchar *fVertexShaderSource[];
	static const GLchar *fFragmentShaderSource[];
	GLint fgProjectionMatrixIndex, fgVertexIndex, fgTexCoordIndex, fModelViewMatrixIndex, fColorOffsetIdx;
	GLint fgForceTranspInd, fTextOffsMultiInd;
	GLint fCompensateDistInd = -1;
};
