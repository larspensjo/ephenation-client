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
#include "DeferredLighting.h"
#include "../ui/Error.h"
#include "../Options.h"
#include "../uniformbuffer.h"
#include "../shadowconfig.h"
#include "../shapes/quad.h"

// This vertex shader will only draw two triangles, giving a full screen.
// The vertex input is 0,0 in one corner and 1,1 in the other.
static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"deferredlighting.Vertex"
};

static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"common.UniformBuffer",
	"common.DoubleResolutionFunction",
	"common.Poissondisk",
	"deferredlighting.Fragment"
};

DeferredLighting::DeferredLighting() {
	fDeadIndex = -1;
	fHeadInWaterIdx = -1; fInsideTeleportIdx = -1;
	fAverageLuminanceIdx = -1;
}

void DeferredLighting::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Initglsw("DeferredLighting", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
}

void DeferredLighting::GetLocations(void) {
	fHeadInWaterIdx = this->GetUniformLocation("Uwater");
	fInsideTeleportIdx = this->GetUniformLocation("Uteleport");
	fDeadIndex = this->GetUniformLocation("Udead");
	fAverageLuminanceIdx = this->GetUniformLocation("UwhitePoint");

	// The following uniforms only need to be initialized once
	glUniform1i(this->GetUniformLocation(RANDOMVEC2POISSON_SAMPLERNAME), 6);
	glUniform1i(this->GetUniformLocation("lightTex"), 5);
	glUniform1i(this->GetUniformLocation("blendTex"), 3);
	glUniform1i(this->GetUniformLocation("normalTex"), 2);
	glUniform1i(this->GetUniformLocation("posTex"), 1);
	glUniform1i(this->GetUniformLocation("diffuseTex"), 0);

	checkError("DeferredLighting::GetLocations");
}

void DeferredLighting::SetWhitePoint(float white) {
	glUniform1f(fAverageLuminanceIdx, white);
}

void DeferredLighting::EnableProgram(void) {
	glUseProgram(this->Program());
}

void DeferredLighting::DisableProgram(void) {
	glUseProgram(0);
}

void DeferredLighting::PlayerDead(bool fl) {
	glUniform1i(fDeadIndex, fl);
}

void DeferredLighting::Draw() {
	glUseProgram(this->Program());
	gQuad.Draw();
	glUseProgram(0);
}

void DeferredLighting::InWater(bool flag) {
	glUniform1i(fHeadInWaterIdx, flag);
}

void DeferredLighting::InsideTeleport(bool flag) {
	glUniform1i(fInsideTeleportIdx, flag);
}
