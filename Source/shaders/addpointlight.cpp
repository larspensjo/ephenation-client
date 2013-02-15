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
#include "addpointlight.h"
#include "../ui/Error.h"
#include "../shapes/octagon.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"common.UniformBuffer",
	"common.GetTileFromSphere",
	"pointlight.Vertex"
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"pointlight.Fragment"
};

AddPointLight::AddPointLight() {
	fLampIdx = -1;
}

void AddPointLight::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Initglsw("AddPointLight", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	checkError("AddPointLight::Init");
}

void AddPointLight::GetLocations(void) {
	fLampIdx = this->GetUniformLocation("Upoint");

	glUniform1i(this->GetUniformLocation("normalTex"), 2);
	glUniform1i(this->GetUniformLocation("posTex"), 1);

	checkError("AddPointLight::GetLocations");
}

void AddPointLight::Draw(const glm::vec3 &pos, float strength) {
	glUseProgram(this->Program());
	const glm::vec4 point(pos.x, pos.y, pos.z, strength);
	glUniform4fv(fLampIdx, 1, &point.x);
	gOctagon.Draw();
	glUseProgram(0);
}
