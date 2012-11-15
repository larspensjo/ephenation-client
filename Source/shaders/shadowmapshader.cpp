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
#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shadowmapshader.h"
#include "../primitives.h"
#include "../uniformbuffer.h"
#include "../shadowconfig.h"

//
// The shadowmap is based on coordinates that are transformed from infinite world to bitmap size.
// The transformation equation is a*x/(b*x+1), which is similar to HDR. This gives a value from 0 to 'w' (bitmap size).
// 'a' is the shadow resolution on near objects. That is, the number of pixels per block.
// 'b' has to be set to 2a/w.
// The formula below is based on a bitmap 128x128, which means w=128, and will scale automatically to bitmaps of other sizes.

static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"common.UniformBuffer",
	"common.DoubleResolutionFunction",
	"#define VERTEXSCALING "  STR(VERTEXSCALING) "\n", // The is the scaling factor used for vertices
	"#define TEXTURESCALING "  STR(TEXTURESCALING) "\n", // The is the scaling factor used for vertices
	"shadowmapshader.Vertex"
};

// Normally, the fragment shader wouldn't have to do anything, and it would still work. However, there are textures with
// transparent areas, and these should not generate shadows. For example, the leaves on the trees are quads with leaves on.
static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"shadowmapshader.Fragment"
};

std::unique_ptr<ShadowMapShader> ShadowMapShader::Make(void) {
	std::unique_ptr<ShadowMapShader> shader(new ShadowMapShader);
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	shader->Initglsw("ShadowMapShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	return shader;
}

void ShadowMapShader::PreLinkCallback(GLuint prg) {
	// glBindFragDataLocation(prg, 0, "diffuseOutput");

	// Ensure that the same index for inputs are always used (to enable the use of the same VAO on other shaders).
	// TODO: Remove the ones not used by this shader.
	glBindAttribLocation(prg, StageOneShader::Normal, "normal");
	glBindAttribLocation(prg, StageOneShader::Vertex, "vertex");
	glBindAttribLocation(prg, StageOneShader::SkinWeights, "weights");
	glBindAttribLocation(prg, StageOneShader::Joints, "joints");
}

void ShadowMapShader::GetLocations(void) {
	fProjectionViewMatrixIdx = this->GetUniformLocation("projectionViewMatrix");
	fModelMatrixIndex = this->GetUniformLocation("modelMatrix");
	fTextOffsMultiInd = this->GetUniformLocation("textOffsMulti");
}

void ShadowMapShader::Model(const glm::mat4 &mat) {
	glUniformMatrix4fv(fModelMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void ShadowMapShader::ProjectionView(const glm::mat4 &mat) {
	glUniformMatrix4fv(fProjectionViewMatrixIdx, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void ShadowMapShader::TextureOffsetMulti(float offsX, float offsY, float mult) {
	glUniform3f(fTextOffsMultiInd, offsX, offsY, mult);
}

ShadowMapShader::ShadowMapShader() : fProjectionViewMatrixIdx(-1), fModelMatrixIndex(-1), fTextOffsMultiInd(-1) {
}
