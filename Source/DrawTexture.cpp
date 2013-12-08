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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "DrawTexture.h"
#include "shaders/SimpleTextureShader.h"
#include "textures.h"
#include "primitives.h"
#include "errormanager.h"

DrawTexture::DrawTexture() : fShader(0), fBufferId(0), fVao(0) {
}

DrawTexture::~DrawTexture() {
	// Shouldn't happen as this is a singleton.
	if (fBufferId != 0)
		glDeleteBuffers(1, &fBufferId);
	if (fVao != 0)
		glDeleteVertexArrays(1, &fVao);
	fBufferId = 0;
	fVao = 0;
}

DrawTexture DrawTexture::fgSingleton;

// Every coordinate is 3 dimensions and two dimension texture
struct vertex {
	float v[2];
	float t[2];
};

// A square defined as two triangles.
static const vertex vertexData[] = {
	{{ 1, 1, }, {1,1}},
	{{ 0, 0, }, {0,0}},
	{{ 0, 1, }, {0,1}},
	{{ 1, 1, }, {1,1}},
	{{ 1, 0, }, {1,0}},
	{{ 0, 0, }, {0,0}},
};

DrawTexture *DrawTexture::Make(void) {
	if (fgSingleton.fShader == 0) {
		fgSingleton.Init();
	}
	return &fgSingleton;
}

void DrawTexture::Init(void) {
	fShader = SimpleTextureShader::Make();
	glGenVertexArrays(1, &fVao);
	glBindVertexArray(fVao);
	fShader->EnableVertexAttribArray();
	glGenBuffers(1, &fBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, fBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof vertexData, vertexData, GL_STATIC_DRAW);
	vertex *p = 0;
	fShader->VertexAttribPointer(GL_FLOAT, 2, sizeof (vertex), &p->v);
	fShader->TextureAttribPointer(GL_FLOAT, sizeof (vertex), &p->t);
	// check data size in VBO is same as input array, if not return 0 and delete VBO
	int bufferSize = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	glBindVertexArray(0);
	if ((unsigned)bufferSize != sizeof vertexData) {
		glDeleteBuffers(1, &fBufferId);
		auto &ss = View::gErrorManager.GetStream(false, false);
		ss << "[BuildingBlocks::Init] Data size is mismatch with input array";
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DrawTexture::Draw(const glm::mat4 &projection, const glm::mat4 &model, float alpha) const {
	glBindVertexArray(fVao);
	fShader->EnableProgram();
	fShader->Projection(projection);
	fShader->ModelView(model);
	fShader->ForceTransparent(alpha);

	this->DrawBasic();

	fShader->ForceTransparent(1.0f);
	fShader->DisableProgram();
	glBindVertexArray(0);
}

void DrawTexture::DrawScreen(bool compensateDistortion) const {
	glBindVertexArray(fVao);
	fShader->EnableProgram();
	fShader->Projection(glm::mat4(1));
	glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(2,2,1));
	glm::mat4 transl = glm::translate(glm::mat4(1), glm::vec3(-1, -1, 0));
	fShader->ModelView(transl * scale);
	fShader->ForceTransparent(1.0f);
	if (compensateDistortion)
		fShader->SetCompensateDistortion(true);

	this->DrawBasic();

	if (compensateDistortion)
		fShader->SetCompensateDistortion(false); // Immediately reset
	fShader->DisableProgram();
	glBindVertexArray(0);
}

void DrawTexture::DrawOffset(const glm::mat4 &projection, const glm::mat4 &model, float offsX, float offsY, float mult) const {
	glBindVertexArray(fVao);
	fShader->EnableProgram();
	fShader->Projection(projection);
	fShader->ModelView(model);

	fShader->TextureOffsetMulti(offsX, offsY, mult);
	this->DrawBasic();
	fShader->TextureOffsetMulti(0.0f, 0.0f, 1.0f); // Restore to default

	fShader->DisableProgram();
	glBindVertexArray(0);
}

void DrawTexture::DrawDepth(const glm::mat4 &projection, const glm::mat4 &model, float offsX, float offsY, float mult) const {
	glBindVertexArray(fVao);
	fShader->EnableProgram();
	fShader->Projection(projection);
	fShader->ModelView(model);

	fShader->TextureOffsetMulti(offsX, offsY, mult);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glDrawArrays(GL_TRIANGLES, 0, sizeof vertexData / sizeof (vertex));
	gNumDraw++;
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	fShader->TextureOffsetMulti(0.0f, 0.0f, 1.0f); // Restore to default

	fShader->DisableProgram();
	glBindVertexArray(0);
}

void DrawTexture::DrawBasic() const {
	glDisable(GL_DEPTH_TEST); // TODO: Should save the old setting
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glDrawArrays(GL_TRIANGLES, 0, sizeof vertexData / sizeof (vertex));
	gNumDraw++;
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}
