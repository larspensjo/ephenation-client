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

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "ColorShader.h"
#include "../primitives.h"

// Minimum program for drawing text
static const GLchar *vertexShaderSource[] = {
	"#version 130\n", // This corresponds to OpenGL 3.0
	"precision mediump float;\n",
	"uniform mat4 projectionMatrix;\n",
	"uniform mat4 modelViewMatrix;\n",
	"in vec3 vertex;\n",
	"void main(void)\n",
	"{\n",
	"	gl_Position = projectionMatrix * modelViewMatrix * vec4(vertex,1.0);\n",
	"}\n",
};

static const GLchar *fragmentShaderSource[] = {
	"#version 130\n", // This corresponds to OpenGL 3.0
	"precision mediump float;\n",
	"uniform sampler2D firstTexture;\n",
	"uniform vec4 color;\n",
	"void main(void)\n",
	"{\n",
	"	gl_FragColor = color;\n",
	"}\n",
};

ColorShader *ColorShader::Make(void) {
	if (fgSingleton.fVertexIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Init("ColorShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void ColorShader::GetLocations(void) {
	fProjectionMatrixIndex = this->GetUniformLocation("projectionMatrix");
	fModelViewMatrixIndex = this->GetUniformLocation("modelViewMatrix");
	fFirstTextureIndex = this->GetUniformLocation("firstTexture");
	fColorIndex = this->GetUniformLocation("color");
	fVertexIndex = this->GetAttribLocation("vertex");
	checkError("DrawText::DrawTextInit");
}

void Projection(glm::mat4 &);

void ColorShader::EnableVertexAttribArray(void) {
	glEnableVertexAttribArray(fVertexIndex);
}

void ColorShader::EnableProgram(void) {
	glUseProgram(this->Program());
}

void ColorShader::DisableProgram(void) {
	glUseProgram(0);
}

void ColorShader::Color(const glm::vec4 &color) {
	if (fColorIndex != -1) {
		glUniform4fv(fColorIndex, 1, &color[0]); // Send our modelView matrix to the shader
	}
	checkError("ColorShader::Color");
}

void ColorShader::Projection(const glm::mat4 &mat) {
	if (fProjectionMatrixIndex != -1) {
		glUniformMatrix4fv(fProjectionMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
	}
	checkError("ColorShader::Projection");
}

void ColorShader::ModelView(const glm::mat4 &mat) {
	if (fModelViewMatrixIndex != -1) {
		glUniformMatrix4fv(fModelViewMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
	}
	checkError("ColorShader::ModelView");
}

void ColorShader::VertexAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer) {
	glVertexAttribPointer(fVertexIndex, 3, type, GL_FALSE, stride, pointer);
}

ColorShader::ColorShader() {
	fProjectionMatrixIndex = -1;
	fFirstTextureIndex = -1;
	fVertexIndex = -1;
	fColorIndex = -1;
	fModelViewMatrixIndex = -1;
}

ColorShader::~ColorShader() {
	// TODO Auto-generated destructor stub
}

ColorShader ColorShader::fgSingleton;
