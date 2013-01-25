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
#include "AnimationShader.h"
#include "../primitives.h"
#include "../shadowconfig.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"common.UniformBuffer",
	"common.DoubleResolutionFunction",
	"#define VERTEXSCALING "  STR(VERTEXSCALING) "\n", // The is the scaling factor used for vertices
	"#define NORMALSCALING "  STR(NORMALSCALING) "\n", // The is the scaling factor used for vertices
	"#define TEXTURESCALING "  STR(TEXTURESCALING) "\n", // The is the scaling factor used for vertices
	"animationshader.Vertex"
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"common.UniformBuffer",
	"animationshader.Fragment"
};

AnimationShader *AnimationShader::Make(void) {
	if (fgSingleton.fModelMatrixIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Initglsw("AnimationShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void AnimationShader::PreLinkCallback(GLuint prg) {
	glBindFragDataLocation(prg, 0, "diffuseOutput");
	glBindFragDataLocation(prg, 1, "posOutput");
	glBindFragDataLocation(prg, 2, "normOutput");

	// Ensure that the same index for inputs are always used (to enable the use of the same VAO on other shaders).
	glBindAttribLocation(prg, StageOneShader::Normal, "normal");
	glBindAttribLocation(prg, StageOneShader::Vertex, "vertex");
	glBindAttribLocation(prg, StageOneShader::SkinWeights, "weights");
	glBindAttribLocation(prg, StageOneShader::Joints, "joints");
}

void AnimationShader::GetLocations(void) {
	fModelMatrixIndex = this->GetUniformLocation("modelMatrix");
	fFirstTextureIndex = this->GetUniformLocation("firstTexture");
	fBonesMatrix = this->GetUniformLocation("bonesMatrix");
	fForShadowmap = this->GetUniformLocation("forShadowmap");
	fShadowProjView = this->GetUniformLocation("shadowProjViewMat");
	checkError("AnimationShader::GetLocations");
}

void AnimationShader::Model(const glm::mat4 &mat) {
	glUniformMatrix4fv(fModelMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void AnimationShader::FirstTexture(int ind) {
	glUniform1i(fFirstTextureIndex, 0);
}

void AnimationShader::Bones(const glm::mat4 *p, int num) {
	glUniformMatrix4fv(fBonesMatrix, num, GL_FALSE, &((*p)[0][0]));
}

AnimationShader::AnimationShader() {
	fModelMatrixIndex = -1;
	fFirstTextureIndex = -1;
	fBonesMatrix = -1;
	fForShadowmap = -1;
	fShadowProjView = -1;
}

AnimationShader::~AnimationShader() {
}

void AnimationShader::Shadowmap(bool enable, const glm::mat4 &projectionview) {
	glUniform1i(fForShadowmap, enable);
	glUniformMatrix4fv(fShadowProjView, 1, GL_FALSE, &projectionview[0][0]);
}

AnimationShader AnimationShader::fgSingleton;
