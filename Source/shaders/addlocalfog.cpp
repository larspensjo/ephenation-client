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
#include <stdlib.h>

#include <glm/glm.hpp>
#include "../primitives.h"
#include "addlocalfog.h"
#include "../ui/Error.h"
#include "../shapes/quad.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"common.UniformBuffer",
	"localfog.Vertex"
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"common.UniformBuffer",
	"localfog.Fragment"
};

AddLocalFog::AddLocalFog() {
	fPointIdx = -1; fAmbientIdx = -1;
}

void AddLocalFog::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Initglsw("AddLocalFog", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	checkError("AddLocalFog::Init");
}

void AddLocalFog::GetLocations(void) {
	fPointIdx = this->GetUniformLocation("Upoint");
	fAmbientIdx = this->GetUniformLocation("Uambient");

	glUniform1i(this->GetUniformLocation("posTex"), 1);

	checkError("AddLocalFog::GetLocations");
}

void AddLocalFog::Draw(const glm::vec4 &pos, float ambient) {
	glUseProgram(this->Program());
	glUniform4fv(fPointIdx, 1, &pos.x);
	glUniform1f(fAmbientIdx, ambient);
	gQuad.Draw();
	glUseProgram(0);
}
