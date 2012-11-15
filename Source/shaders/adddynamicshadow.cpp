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
#include <stdlib.h>

#include <glm/glm.hpp>
#include "../primitives.h"
#include "adddynamicshadow.h"
#include "../ui/Error.h"
#include "../Options.h"
#include "../uniformbuffer.h"
#include "../shadowconfig.h"
#include "../shapes/quad.h"
#include "../contrib/glsw.h"

static const GLchar *vertexShaderSource[] = {
	"#version 330\n",
	"dynamicshadow.Vertex"
};

static const GLchar *fragmentShaderSource[] = {
	"#version 330\n",
	"common.UniformBuffer",
	"common.DoubleResolutionFunction",
	"common.Poissondisk",
	"#define DYNAMIC_SHADOW_MAP_SIZE " STR(DYNAMIC_SHADOW_MAP_SIZE) "\n",
	"#define STATIC_SHADOW_MAP_SIZE " STR(STATIC_SHADOW_MAP_SIZE) "\n",
	"dynamicshadow.Fragment"
};

AddDynamicShadow::AddDynamicShadow() {
	fShadowMapMatrixIdx = -1;
}

void AddDynamicShadow::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Initglsw("AddDynamicShadow", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	checkError("AddDynamicShadow::Init");
}

void AddDynamicShadow::GetLocations(void) {
	fShadowMapMatrixIdx = this->GetUniformLocation("shadowmat");

	// The followig uniforms only need to be initialized once
	glUniform1i(this->GetUniformLocation("shadowmapTex"), 4); // The shadow map has to use GL_TEXTURE4
	glUniform1i(this->GetUniformLocation("normalTex"), 2);
	glUniform1i(this->GetUniformLocation("posTex"), 1);
	glUniform1i(this->GetUniformLocation(RANDOMVEC2POISSON_SAMPLERNAME), 0);

	checkError("AddDynamicShadow::GetLocations");
}

void AddDynamicShadow::Draw(const glm::mat4 &mat) {
	glUseProgram(this->Program());
	glUniformMatrix4fv(fShadowMapMatrixIdx, 1, GL_FALSE, &mat[0][0]);
	gQuad.Draw();
	glUseProgram(0);
}
