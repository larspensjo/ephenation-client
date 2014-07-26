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
#include <stdio.h>
#include <stdlib.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../primitives.h"
#include "skybox.h"
#include "../ui/Error.h"
#include "../shapes/quad.h"
#include "../player.h"
#include "../textures.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"common.UniformBuffer",
	"skybox.Vertex",
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"common.UniformBuffer",
	"skybox.Fragment",
};

void SkyBox::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Initglsw("Skybox", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	checkError("SkyBox::Init");
}

void SkyBox::GetLocations(void) {
	fModelMatrixIdx = this->GetUniformLocation("UmodelMatrix");

	// Always use texture 0.
	glUniform1i(this->GetUniformLocation("UTextureSampler"), 0);

	checkError("SkyBox::GetLocations");
}

// ====================================================================================================================
/**
 * @brief This is the Draw method for the Skybox
 *
 * The skybox is created using 6 quads, each going from -1.0 to 1.0. The quads are rotated
 * and a box is formed. The model transformations(the rotations) are done on the CPU as shown
 * below. The view transform is then applied within the shader.
 */
// ====================================================================================================================
void SkyBox::Draw() {
	bool belowGround = false;
	if (Model::gPlayer.BelowGround()) // If a number of blocks below ground, use a dark gray texture instead
		belowGround = true;
	glm::mat4 model(1);
	glm::mat3 rot = glm::mat3(model);
	glUseProgram(this->Program());
	glUniformMatrix3fv(fModelMatrixIdx, 1, GL_FALSE, &rot[0][0]); //
	glBindTexture(GL_TEXTURE_2D, belowGround ? GameTexture::DarkGray : GameTexture::Sky1Id);
	gQuad.Draw();

	glBindTexture(GL_TEXTURE_2D, belowGround ? GameTexture::DarkGray : GameTexture::Sky2Id);
	rot =  glm::mat3(glm::rotate(model, 270.0f, glm::vec3(0.0f, 1.0f, 0.0f)));
	glUniformMatrix3fv(fModelMatrixIdx, 1, GL_FALSE, &rot[0][0]); //
	gQuad.Draw();

	glBindTexture(GL_TEXTURE_2D, belowGround ? GameTexture::DarkGray : GameTexture::Sky3Id);
	rot =  glm::mat3(glm::rotate(model, 180.0f, glm::vec3(0.0f, 1.0f, 0.0f)));
	glUniformMatrix3fv(fModelMatrixIdx, 1, GL_FALSE, &rot[0][0]); //
	gQuad.Draw();

	glBindTexture(GL_TEXTURE_2D, belowGround ? GameTexture::DarkGray : GameTexture::Sky4Id);
	rot =  glm::mat3(glm::rotate(model, 90.0f, glm::vec3(0.0f, 1.0f, 0.0f)));
	glUniformMatrix3fv(fModelMatrixIdx, 1, GL_FALSE, &rot[0][0]); //
	gQuad.Draw();

	glBindTexture(GL_TEXTURE_2D, belowGround ? GameTexture::DarkGray : GameTexture::SkyupId);
	rot =  glm::mat3(glm::rotate(model, 90.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
	glUniformMatrix3fv(fModelMatrixIdx, 1, GL_FALSE, &rot[0][0]); //
	gQuad.Draw();

	glBindTexture(GL_TEXTURE_2D, GameTexture::DarkGray);
	rot =  glm::mat3(glm::rotate(model, -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)));
	glUniformMatrix3fv(fModelMatrixIdx, 1, GL_FALSE, &rot[0][0]); //
	gQuad.Draw();

	glUseProgram(0);
}
