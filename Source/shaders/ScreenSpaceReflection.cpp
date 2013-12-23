// Copyright 2013 The Ephenation Authors
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
#include "ScreenSpaceReflection.h"
#include "../ui/Error.h"
#include "../shadowconfig.h"
#include "../shapes/quad.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"ScreenSpaceReflection.Vertex",
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"common.UniformBuffer",
	"common.SimplexNoise",
	"ScreenSpaceReflection.Fragment",
};

void ScreenSpaceReflection::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Initglsw("ScreenSpaceReflection", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	checkError("ScreenSpaceReflection::Init");
}

void ScreenSpaceReflection::GetLocations(void) {
	// The following uniforms only need to be initialized once
	glUniform1i(this->GetUniformLocation("normalTex"), 2);
	glUniform1i(this->GetUniformLocation("posTex"), 1);
	glUniform1i(this->GetUniformLocation("colTex"), 0);

	checkError("ScreenSpaceReflection::GetLocations");
}

void ScreenSpaceReflection::Draw(void) {
	glUseProgram(this->Program());
	gQuad.Draw();
	glUseProgram(0);
}
