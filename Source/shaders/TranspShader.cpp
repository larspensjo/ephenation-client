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

#include <GL/glew.h>
#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "TranspShader.h"
#include "../primitives.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"common.UniformBuffer",
	"#define VERTEXSCALING "  STR(VERTEXSCALING) "\n",   // The is the scaling factor used for vertices
	"#define TEXTURESCALING "  STR(TEXTURESCALING) "\n", // The is the scaling factor used for textures
	"transparent.Vertex",
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"common.UniformBuffer",
	"transparent.Fragment",
};

void TranspShader::Init(void) {
	if (fModelMatrixIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		ShaderBase::Initglsw("TranspShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
}

void TranspShader::PreLinkCallback(GLuint prg) {
	glBindFragDataLocation(prg, 0, "blendOutput");

	// Ensure that the same index for inputs are always used (to enable the use of the same VAO on other shaders).
	glBindAttribLocation(prg, StageOneShader::Vertex, "vertex");
}

void TranspShader::GetLocations(void) {
	fModelMatrixIndex = this->GetUniformLocation("modelMatrix");
	fDepthDependingAlpha = this->GetUniformLocation("depthDependingAlpha");
	fScreenSizeIdx = this->GetUniformLocation("screenSize");
	fTimeIdx = this->GetUniformLocation("time");

	GLint firstTextureIndex = this->GetUniformLocation("firstTexture");
	GLint deptPositionIdx = this->GetUniformLocation("posTexture");
	glUniform1i(firstTextureIndex, 0);
	glUniform1i(deptPositionIdx, 1);
	checkError("TranspShader::GetLocations");
}

void TranspShader::Model(const glm::mat4 &mat) {
	glUniformMatrix4fv(fModelMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void TranspShader::View(float time) {
	glUniform1f(fTimeIdx, time);
}

void TranspShader::Projection(float w, float h) {
	glUniform2f(fScreenSizeIdx, w, h);
}

void TranspShader::DrawingWater(bool flag) {
	glUniform1i(fDepthDependingAlpha, flag);
}

TranspShader::TranspShader() {
	fModelMatrixIndex = -1;
	fDepthDependingAlpha = -1;
	fScreenSizeIdx = -1;
	fTimeIdx = -1;
}

TranspShader::~TranspShader() {
	// TODO Auto-generated destructor stub
}

TranspShader gTranspShader;
