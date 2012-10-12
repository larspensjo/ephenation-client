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

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ColorShader.h"
#include "../primitives.h"

// Minimum program for drawing text
static const GLchar *vertexShaderSource[] = {
	"#version 330\n",
	"uniform mat4 UprojectionMatrix;\n",
	"uniform mat4 UmodelViewMatrix;\n",
	"layout (location=0) in vec3 Avertex;\n", // Location must match VERTEX_INDEX
	"layout (location=1) in vec4 Acolor;"     // Location must match COLOR_INDEX
	"out vec4 color;"
	"void main(void)\n",
	"{\n",
	"	gl_Position = UprojectionMatrix * UmodelViewMatrix * vec4(Avertex,1.0);\n",
	"	color = Acolor;"
	"}\n",
};

static const GLchar *fragmentShaderSource[] = {
	"#version 330\n",
	"precision mediump float;\n",
	"uniform vec4 Ucolor;\n",
	"in vec4 color;"
	"void main(void)\n",
	"{\n",
	"	if (Ucolor.a != 0)"
	"		gl_FragColor = Ucolor;\n",
	"	else"
	"		gl_FragColor = color;\n",
	"}\n",
};

ColorShader *ColorShader::Make(void) {
	if (fgSingleton.fProjectionMatrixIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Init("ColorShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
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

ColorShader::ColorShader() {
	fProjectionMatrixIndex = -1;
	fColorIndex = -1;
	fModelViewMatrixIndex = -1;
}

ColorShader ColorShader::fgSingleton;
