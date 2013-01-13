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
#include "ChunkShader.h"
#include "../primitives.h"
#include "../uniformbuffer.h"
#include "../ui/Error.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"common.UniformBuffer",
	"#define VERTEXSCALING "  STR(VERTEXSCALING) "\n", // The is the scaling factor used for vertices
	"#define NORMALSCALING "  STR(NORMALSCALING) "\n", // The is the scaling factor used for vertices
	"#define TEXTURESCALING "  STR(TEXTURESCALING) "\n", // The is the scaling factor used for vertices
	"chunkshader.Vertex"
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"common.UniformBuffer",
	"chunkshader.Fragment"
};

ChunkShader *ChunkShader::Make(void) {
	if (fgSingleton.fModelMatrixIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Initglsw("ChunkShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void ChunkShader::PreLinkCallback(GLuint prg) {
	// Ensure that the same index for inputs are always used (to enable the use of the same VAO on other shaders).
	glBindAttribLocation(prg, StageOneShader::Normal, "normal");
	glBindAttribLocation(prg, StageOneShader::Vertex, "vertex");
}

void ChunkShader::GetLocations(void) {
	fModelMatrixIndex = this->GetUniformLocation("modelMatrix");
	fFirstTextureIndex = this->GetUniformLocation("firstTexture");
	fTextOffsMultiInd = this->GetUniformLocation("textOffsMulti");

	checkError("ChunkShader::GetLocations");
}

void ChunkShader::Model(const glm::mat4 &mat) {
	if (fModelMatrixIndex != -1)
		glUniformMatrix4fv(fModelMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void ChunkShader::FirstTexture(int ind) {
	if (fFirstTextureIndex != -1)
		glUniform1i(fFirstTextureIndex, 0);
}

void ChunkShader::TextureOffsetMulti(float offsX, float offsY, float mult) {
	glUniform3f(fTextOffsMultiInd, offsX, offsY, mult);
}

ChunkShader::ChunkShader() {
	fModelMatrixIndex = -1;
	fFirstTextureIndex = -1;
	fTextOffsMultiInd = -1;
}

ChunkShader::~ChunkShader() {
	// TODO Auto-generated destructor stub
}

ChunkShader ChunkShader::fgSingleton;
