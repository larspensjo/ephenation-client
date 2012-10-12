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
#include <glm/gtc/matrix_transform.hpp>
#include "../primitives.h"
#include "skybox.h"
#include "../ui/Error.h"
#include "../uniformbuffer.h"
#include "../shapes/quad.h"
#include "../player.h"
#include "../textures.h"

// This vertex shader will only draw two triangles, limited to the part of the screen
// that can be affected.
// The vertex input is 0,0 in one corner and 1,1 in the other. Draw the quad at z -1, with x and y
// going from -1 to 1 (and then transformed with the model matrix).
static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	UNIFORMBUFFER
	"uniform mat3 UmodelMatrix;\n",         // Only rotation needed
	"layout (location = 0) in vec2 vertex;\n",
	"out vec2 TexCoord;\n",
	"out vec3 position;\n",
	"void main(void)\n",
	"{\n",
	"	mat3 view = mat3(UBOViewMatrix);"   // Only rotation needed
	"	vec3 pos = UmodelMatrix * vec3(vertex*2-1, -1);"
	"	gl_Position = UBOProjectionMatrix * vec4(view * pos, 1);"
	"	position = pos*10000;"               // This is the important thing. That the sky has a coordinate that is very far away.
	"	TexCoord = vertex;"
	"}\n",
};

static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"uniform sampler2D UTextureSampler;\n",
	"in vec3 position;\n",       // The model coordinate, as given by the vertex shader
	"in vec2 TexCoord;\n",

	"layout(location = 0) out vec4 diffuseOutput;\n",
	"layout(location = 1) out vec4 posOutput;\n",
	"layout(location = 2) out vec4 normOutput;\n",

	"void main(void)\n",
	"{\n",
	"   vec4 color = texture(UTextureSampler, TexCoord);\n",
	"	diffuseOutput = color;"
	"	normOutput = vec4(0,0,0,1);"    // Last byte is ambient light
	"	posOutput = vec4(position, 1);" // Last byte is sun intensity
	"}\n",
};

SkyBox::SkyBox() {
	fModelMatrixIdx = -1;
}

void SkyBox::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Init("Skybox", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	checkError("SkyBox::Init");
}

void SkyBox::GetLocations(void) {
	fModelMatrixIdx = this->GetUniformLocation("UmodelMatrix");

	// Always use texture 0.
	glUniform1i(this->GetUniformLocation("UTextureSampler"), 0);

	checkError("SkyBox::GetLocations");
}

void SkyBox::Draw() {
	bool belowGround = false;
	if (gPlayer.BelowGround()) // If a number of blocks below ground, use a dark gray texture instead
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

	glUseProgram(0);
}
