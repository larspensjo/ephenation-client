// Copyright 2014 The Ephenation Authors
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
#include "ParticleShader.h"
#include "../primitives.h"
#include "../ui/Error.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"common.UniformBuffer",
	"common.SimplexNoise",
	"#define VERTEXSCALING "  STR(VERTEXSCALING) "\n", // The is the scaling factor used for vertices
	"#define NORMALSCALING "  STR(NORMALSCALING) "\n", // The is the scaling factor used for normals
	"#define TEXTURESCALING "  STR(TEXTURESCALING) "\n", // The is the scaling factor used for textures
	"ParticleShader.Vertex",
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"common.UniformBuffer",
	"ParticleShader.Fragment",
};

ParticleShader *ParticleShader::Make(void) {
	if (fgSingleton.fModelMatrixIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Initglsw("ParticleShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void ParticleShader::PreLinkCallback(GLuint prg) {
	// Ensure that the same index for inputs are always used (to enable the use of the same VAO on other shaders).
	glBindAttribLocation(prg, StageOneShader::Normal, "normal");
	glBindAttribLocation(prg, StageOneShader::Vertex, "vertex");
	glBindAttribLocation(prg, StageOneShader::Material, "material");
}

void ParticleShader::GetLocations(void) {
	fModelMatrixIndex = this->GetUniformLocation("modelMatrix");
	fTextOffsMultiInd = this->GetUniformLocation("textOffsMulti");

	glUniform1i(this->GetUniformLocation("firstTexture"), 0); // Always 0
	checkError("ParticleShader::GetLocations");
}

void ParticleShader::Model(const glm::mat4 &mat) {
	if (fModelMatrixIndex != -1)
		glUniformMatrix4fv(fModelMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void ParticleShader::TextureOffsetMulti(float offsX, float offsY, float mult) {
	glUniform3f(fTextOffsMultiInd, offsX, offsY, mult);
}

ParticleShader::ParticleShader() {
	fModelMatrixIndex = -1;
	fTextOffsMultiInd = -1;
}

ParticleShader::~ParticleShader() {
	// TODO Auto-generated destructor stub
}

ParticleShader ParticleShader::fgSingleton;
