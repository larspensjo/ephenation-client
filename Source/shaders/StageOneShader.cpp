// Copyright 2012-2014 The Ephenation Authors
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

#include <glm/glm.hpp>
#include "StageOneShader.h"
#include "../primitives.h"

void StageOneShader::VertexAttribPointer(int offset) {
	enum { stride = sizeof(VertexDataf), };
	glVertexAttribPointer(StageOneShader::Normal, 4, GL_BYTE, GL_FALSE, stride, VertexDataf::GetNormalOffset());
	glVertexAttribPointer(StageOneShader::Vertex, 4, GL_SHORT, GL_FALSE, stride, VertexDataf::GetVertexOffset());
	glVertexAttribIPointer(StageOneShader::Material, 1, GL_INT, stride, VertexDataf::GetMaterialOffset());
}

void StageOneShader::VertexAttribPointerSkinWeights(int skinOffset, int jointOffset) {
	// This will map up to 3 weights for every vertex. They are expected to be located with no space in between.
	glVertexAttribPointer(StageOneShader::SkinWeights, 3, GL_FLOAT, GL_FALSE, sizeof (glm::vec3), (void *)skinOffset);
	glVertexAttribPointer(StageOneShader::Joints, 3, GL_FLOAT, GL_FALSE, 3*sizeof (float), (void *)jointOffset);
}

void StageOneShader::EnableVertexAttribArray(bool useBones) {
	glEnableVertexAttribArray(StageOneShader::Normal);
	glEnableVertexAttribArray(StageOneShader::Vertex);
	glEnableVertexAttribArray(StageOneShader::Material);
	if (useBones) {
		glEnableVertexAttribArray(StageOneShader::SkinWeights);
		glEnableVertexAttribArray(StageOneShader::Joints);
	}
}

void StageOneShader::EnableProgram(void) {
	glUseProgram(this->Program());
}

void StageOneShader::DisableProgram(void) {
	glUseProgram(0);
}
