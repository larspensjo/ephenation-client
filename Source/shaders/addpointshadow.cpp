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
#include "addpointshadow.h"
#include "../ui/Error.h"
#include "../shapes/quad.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"common.UniformBuffer",
	"common.GetTileFromSphere",
	"addpointshadow.Vertex",
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"common.UniformBuffer",
	"common.DistanceAlphaBlending",
	"addpointshadow.Fragment",
};

AddPointShadow::AddPointShadow() {
	fPointIdx = -1; fSelectionIdx = -1;
	fPreviousMode = 0;
}

void AddPointShadow::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Initglsw("AddPointShadow", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
}

void AddPointShadow::GetLocations(void) {
	fPointIdx = this->GetUniformLocation("Upoint");
	fSelectionIdx = this->GetUniformLocation("Umode");

	glUniform1i(this->GetUniformLocation("posTex"), 1);
}

void AddPointShadow::DrawBlueLight(const glm::vec4 &pos) {
	glUseProgram(this->Program());
	if (fPreviousMode != 4) {
		// Remember which was the previous state, to minimize updates
		fPreviousMode = 4;
		glUniform1i(fSelectionIdx, fPreviousMode);
	}
	glUniform4fv(fPointIdx, 1, &pos.x);
	gQuad.Draw();
	glUseProgram(0);
}

void AddPointShadow::DrawGreenLight(const glm::vec4 &pos) {
	glUseProgram(this->Program());
	if (fPreviousMode != 3) {
		// Remember which was the previous state, to minimize updates
		fPreviousMode = 3;
		glUniform1i(fSelectionIdx, fPreviousMode);
	}
	glUniform4fv(fPointIdx, 1, &pos.x);
	gQuad.Draw();
	glUseProgram(0);
}

void AddPointShadow::DrawRedLight(const glm::vec4 &pos) {
	glUseProgram(this->Program());
	if (fPreviousMode != 2) {
		// Remember which was the previous state, to minimize updates
		fPreviousMode = 2;
		glUniform1i(fSelectionIdx, fPreviousMode);
	}
	glUniform4fv(fPointIdx, 1, &pos.x);
	gQuad.Draw();
	glUseProgram(0);
}

void AddPointShadow::DrawMonsterSelection(const glm::vec4 &pos) {
	glUseProgram(this->Program());
	if (fPreviousMode != 1) {
		// Remember which was the previous state, to minimize updates
		fPreviousMode = 1;
		glUniform1i(fSelectionIdx, fPreviousMode);
	}
	glUniform4fv(fPointIdx, 1, &pos.x);
	gQuad.Draw();
	glUseProgram(0);
}

void AddPointShadow::DrawPointShadow(const glm::vec4 &pos) {
	glUseProgram(this->Program());
	if (fPreviousMode != 0) {
		// Remember which was the previous state, to minimize updates
		fPreviousMode = 0;
		glUniform1i(fSelectionIdx, fPreviousMode);
	}
	glUniform4fv(fPointIdx, 1, &pos.x);
	gQuad.Draw();
	glUseProgram(0);
}
