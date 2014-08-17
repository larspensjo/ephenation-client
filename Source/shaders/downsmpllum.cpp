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

#include <glbinding/gl/functions33.h>
#include <stdio.h>
#include <stdlib.h>

#include <glm/glm.hpp>
#include "../primitives.h"
#include "downsmpllum.h"
#include "../ui/Error.h"
#include "../Options.h"
#include "../uniformbuffer.h"
#include "../shadowconfig.h"
#include "../shapes/quad.h"

using namespace gl33;

/// This vertex shader will only draw two triangles, giving a full screen.
/// The vertex input is 0,0 in one corner and 1,1 in the other.
/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"downsmpllum.Vertex"
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"common.UniformBuffer",
	"common.DoubleResolutionFunction",
	"common.SunDirection",
	"downsmpllum.Fragment"
};

DownSamplingLuminance::DownSamplingLuminance() {
}

void DownSamplingLuminance::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Initglsw("DownSamplingLuminance", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
}

void DownSamplingLuminance::GetLocations(void) {
	// The following uniforms only need to be initialized once
	glUniform1i(this->GetUniformLocation("lightTex"), 5);
	glUniform1i(this->GetUniformLocation("blendTex"), 3);
	glUniform1i(this->GetUniformLocation("normalTex"), 2);
	glUniform1i(this->GetUniformLocation("posTex"), 1);
	glUniform1i(this->GetUniformLocation("diffuseTex"), 0);

	checkError("DownSamplingLuminance::GetLocations");
}

void DownSamplingLuminance::EnableProgram(void) {
	glUseProgram(this->Program());
}

void DownSamplingLuminance::DisableProgram(void) {
	glUseProgram(0);
}

void DownSamplingLuminance::Draw() {
	glUseProgram(this->Program());
	gQuad.Draw();
	glUseProgram(0);
}
