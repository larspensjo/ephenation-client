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
#include "ChunkShaderPicking.h"
#include "../primitives.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"common.UniformBuffer",
	"#define VERTEXSCALING "  STR(VERTEXSCALING) "\n", // The is the scaling factor used for vertices
	"chunkshaderpicking.Vertex"
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"chunkshaderpicking.Fragment"
};

void ChunkShaderPicking::Init() {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	this->Initglsw("ChunkShaderPicking", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
}

void ChunkShaderPicking::GetLocations(void) {
	fModelMatrixIndex = this->GetUniformLocation("modelMatrix");

	fVertexIndex = this->GetAttribLocation("vertex");
	fNormalIndex = this->GetAttribLocation("normal");
}

void Projection(glm::mat4 &);

void ChunkShaderPicking::EnableVertexAttribArray(void) {
	glEnableVertexAttribArray(fNormalIndex);
	glEnableVertexAttribArray(fVertexIndex);
}

void ChunkShaderPicking::EnableProgram(void) {
	glUseProgram(this->Program());
}

void ChunkShaderPicking::DisableProgram(void) {
	glUseProgram(0);
}

void ChunkShaderPicking::Model(const glm::mat4 &mat) {
	if (fModelMatrixIndex != -1)
		glUniformMatrix4fv(fModelMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void ChunkShaderPicking::VertexAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer) {
	glVertexAttribPointer(fVertexIndex, 3, type, GL_FALSE, stride, pointer);
}

void ChunkShaderPicking::NormalAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer) {
	glVertexAttribPointer(fNormalIndex, 3, type, GL_FALSE, stride, pointer);
}

ChunkShaderPicking::ChunkShaderPicking() {
	fModelMatrixIndex = -1;

	fNormalIndex = -1;
	fVertexIndex = -1;
}

ChunkShaderPicking gChunkShaderPicking;
