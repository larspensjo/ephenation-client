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

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "BarrelDistortion.h"
#include "../primitives.h"

using namespace Shaders;

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"barreldistortion.Vertex",
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"common.UniformBuffer",
	"barreldistortion.Fragment",
};

BarrelDistortion *BarrelDistortion::Make(void) {
	if (!fgSingleton.fInitialized) {
		fgSingleton.fInitialized = true;
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Initglsw("BarrelDistortion", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void BarrelDistortion::GetLocations(void) {
	fModelViewMatrixIndex = this->GetUniformLocation("modelViewMatrix");
	glUniform1i(this->GetUniformLocation("firstTexture"), 0); // Always at texture 0
	fgVertexIndex = this->GetAttribLocation("vertex");
	fgTexCoordIndex = this->GetAttribLocation("texCoord");
}

void BarrelDistortion::EnableVertexAttribArray(void) {
	glEnableVertexAttribArray(fgVertexIndex);
	glEnableVertexAttribArray(fgTexCoordIndex);
}

void BarrelDistortion::EnableProgram(void) {
	glUseProgram(this->Program());
}

void BarrelDistortion::DisableProgram(void) {
	glUseProgram(0);
}

void BarrelDistortion::ModelView(const glm::mat4 &mat) {
	glUniformMatrix4fv(fModelViewMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void BarrelDistortion::VertexAttribPointer(GLenum type, GLint size, GLsizei stride, const GLvoid * pointer) {
	glVertexAttribPointer(fgVertexIndex, size, type, GL_FALSE, stride, pointer);
}

void BarrelDistortion::TextureAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer) {
	glVertexAttribPointer(fgTexCoordIndex, 2, type, GL_FALSE, stride, pointer);
}

BarrelDistortion BarrelDistortion::fgSingleton;
