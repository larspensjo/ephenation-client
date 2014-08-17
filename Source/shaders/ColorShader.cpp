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

#include <glbinding/gl/functions33.h>
#include <glbinding/gl/enum33.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ColorShader.h"
#include "../primitives.h"

using namespace gl33;

/// Minimum program for drawing text
/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"common.UniformBuffer",
	"colorshader.Vertex"
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"colorshader.Fragment"
};

ColorShader *ColorShader::Make(void) {
	if (fgSingleton.fProjectionMatrixIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Initglsw("ColorShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void ColorShader::GetLocations(void) {
	fProjectionMatrixIndex = this->GetUniformLocation("UprojectionMatrix");
	fModelViewMatrixIndex = this->GetUniformLocation("UmodelViewMatrix");
	fColorIndex = this->GetUniformLocation("Ucolor");
	ASSERT(this->GetAttribLocation("Avertex") == VERTEX_INDEX);
	ASSERT(this->GetAttribLocation("Acolor") == COLOR_INDEX);
}

void ColorShader::EnableProgram(void) {
	glUseProgram(this->Program());
}

void ColorShader::DisableProgram(void) {
	glUseProgram(0);
}

void ColorShader::Color(const glm::vec4 &color) {
	glUniform4fv(fColorIndex, 1, &color[0]);
}

void ColorShader::Projection(const glm::mat4 &mat) {
	glUniformMatrix4fv(fProjectionMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our projection matrix to the shader
}

void ColorShader::ModelView(const glm::mat4 &mat) {
	glUniformMatrix4fv(fModelViewMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

ColorShader ColorShader::fgSingleton;
