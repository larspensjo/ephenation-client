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

#include <string>
#include <stdlib.h>
#include <GL/glew.h>

#include "vsfl/vsfl.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "DrawText.h"
#include "primitives.h"
#include "shaders/SimpleTextureShader.h"
#include "Options.h"

// TODO: This class should be merged into the VSFLFont class.

DrawFont::DrawFont() {
	fShader = 0;
	fProjectionMatrixNeedUpdate = false;
	fShaderEnabled = false;
}

DrawFont::~DrawFont() {
	fShader = 0; // Singleton, shall not be deleted.
}

void DrawFont::Init(const std::string &fontName) {
	vsfl.loadFont(fontName);
	fShader = SimpleTextureShader::Make();
	vsfl.setShader(DrawFont::fShader);
}

void DrawFont::UpdateProjection(void) {
	if (fShaderEnabled) {
		glm::mat4 projectionMatrix = glm::ortho((float)gViewport[0], (float)(gViewport[0] + gViewport[2]), (float)(gViewport[1] + gViewport[3]), (float)gViewport[1], 0.0f, 1.0f);
		fShader->Projection(projectionMatrix);
		fProjectionMatrixNeedUpdate = false;
	} else {
		// Can't do it now, save it for later.
		fProjectionMatrixNeedUpdate = true;
	}
}

void DrawFont::SetOffset(float row, float col) {
	// The font size use a simple scaling, not different font definitions.
	float scale = Options::fgOptions.fFontSize / 12.0f;
	glm::mat4 mat = glm::translate(glm::mat4(1), glm::vec3(row, col, 0.0f));
	mat = glm::scale(mat, glm::vec3(scale, scale, 1.0f));
	fShader->ModelView(mat);
}

void DrawFont::Enable(void) {
	fShader->EnableProgram();
	fShaderEnabled = true;
	if (fProjectionMatrixNeedUpdate) {
		this->UpdateProjection();
	}
}

void DrawFont::Disable(void) {
	fShaderEnabled = false;
	fShader->DisableProgram();
}

DrawFont gDrawFont;
