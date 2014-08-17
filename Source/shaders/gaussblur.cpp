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
#include "gaussblur.h"
#include "../ui/Error.h"
#include "../shapes/quad.h"

using namespace gl33;

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"common.UniformBuffer",
	"gaussblur.Vertex",
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"gaussblur.Fragment",
};

GaussianBlur::GaussianBlur() {
	fSigmaIdx = -1;
	fNumBlurPixelsPerSideIdx = -1;
	fBlurMultiplyVecIdx = -1;
}

void GaussianBlur::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Initglsw("GaussianBlur", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
}

void GaussianBlur::GetLocations(void) {
	fSigmaIdx = this->GetUniformLocation("Usigma");
	fNumBlurPixelsPerSideIdx = this->GetUniformLocation("UnumBlurPixelsPerSide");
	fBlurMultiplyVecIdx = this->GetUniformLocation("UblurMultiplyVec");

	glUniform1i(this->GetUniformLocation("UblurSampler"), 0); // Always use active texture 0.
}

void GaussianBlur::BlurHorizontal(int tap, float sigma) const {
	glUseProgram(this->Program());
	glUniform1f(fNumBlurPixelsPerSideIdx, float(tap/2)); // Tap is an odd number >= 5.
	glUniform1f(fSigmaIdx, sigma);
	glUniform2f(fBlurMultiplyVecIdx, 1.0f, 0.0f);
	gQuad.Draw();
}

void GaussianBlur::BlurVertical() const {
	glUniform2f(fBlurMultiplyVecIdx, 0.0f, 1.0f);
	gQuad.Draw();
	glUseProgram(0);
}
