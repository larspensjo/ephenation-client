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
#include "modulatedtextureshader.h"
#include "../primitives.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"common.UniformBuffer",
	"modulatedtexture.Vertex"
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"modulatedtexture.Fragment"
};

ModulatedTextureShader::ModulatedTextureShader() {
	fgProjectionMatrixIndex = -1;
	fModelViewMatrixIndex = -1;
}

void ModulatedTextureShader::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Initglsw("ModulatedTextureShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
}

void ModulatedTextureShader::GetLocations(void) {
	fgProjectionMatrixIndex = this->GetUniformLocation("UprojectionMatrix");
	fModelViewMatrixIndex = this->GetUniformLocation("UmodelViewMatrix");
	glUniform1i(this->GetUniformLocation("UfirstTexture"), 0); // Always at texture 0

	ASSERT(this->GetAttribLocation("Avertex") == VERTEX_INDEX);
	ASSERT(this->GetAttribLocation("AtexCoord") == TEXTURE_INDEX);
	ASSERT(this->GetAttribLocation("AcolorFactor") == COLOR_INDEX);

	checkError("ModulatedTextureShader::GetLocations");
}

void ModulatedTextureShader::EnableProgram(void) {
	glUseProgram(this->Program());
}

void ModulatedTextureShader::DisableProgram(void) {
	glUseProgram(0);
}

void ModulatedTextureShader::Projection(const glm::mat4 &mat) {
	glUniformMatrix4fv(fgProjectionMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void ModulatedTextureShader::ModelView(const glm::mat4 &mat) {
	glUniformMatrix4fv(fModelViewMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}
