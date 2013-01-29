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
#include "HealthBar.h"
#include "shaders/ColorShader.h"
#include "textures.h"
#include "primitives.h"
#include "ui/Error.h"

using namespace View;

HealthBar::HealthBar() : fShader(0), fBufferId(0), fVao(0) {
}

HealthBar::~HealthBar() {
	// Shouldn't happen as this is a singleton.
	if (fBufferId != 0)
		glDeleteBuffers(1, &fBufferId);
	if (fVao != 0)
		glDeleteVertexArrays(1, &fVao);
	fBufferId = 0;
	fVao = 0;
}

HealthBar HealthBar::fgSingleton;

// Every coordinate is 3 dimensions and two dimension texture
struct vertex {
	float v[3];
};

// A square defined as two triangles.
static const vertex vertexData[] = {
	{{ 1, 1, 0, }},
	{{ 0, 0, 0, }},
	{{ 0, 1, 0, }},
	{{ 1, 1, 0, }},
	{{ 1, 0, 0, }},
	{{ 0, 0, 0, }},
};

HealthBar *HealthBar::Make(void) {
	if (fgSingleton.fShader == 0) {
		fgSingleton.Init();
	}
	return &fgSingleton;
}

void HealthBar::Init(void) {
	fShader = ColorShader::Make();
	glGenVertexArrays(1, &fVao);
	glBindVertexArray(fVao);
	glEnableVertexAttribArray(fShader->VERTEX_INDEX);
	glGenBuffers(1, &fBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, fBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof vertexData, vertexData, GL_STATIC_DRAW);
	vertex *p = 0;
	glVertexAttribPointer(fShader->VERTEX_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof (vertex), &p->v);
	// check data size in VBO is same as input array, if not return 0 and delete VBO
	int bufferSize = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	glBindVertexArray(0);
	if ((unsigned)bufferSize != sizeof vertexData) {
		glDeleteBuffers(1, &fBufferId);
		ErrorDialog("HealthBar::Init: Data size is mismatch with input array\n");
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void HealthBar::DrawHealth(const glm::mat4 &projection, const glm::mat4 &model, float hp, float dmg, bool fillEnd) const {
	glBindVertexArray(fVao);
	fShader->EnableProgram();
	fShader->Projection(projection);
	glDisable(GL_DEPTH_TEST); // TODO: Should save the old setting
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	const float alpha = 0.4f;

	glm::mat4 newModel = glm::scale(model, glm::vec3(hp, 1.0f, 1.0f));
	fShader->ModelView(newModel);
	fShader->Color(glm::vec4(0.0f, 1.0f, 0.0f, alpha));
	glDrawArrays(GL_TRIANGLES, 0, sizeof vertexData / sizeof (vertex));
	gNumDraw++;

	newModel = glm::scale(glm::translate(model, glm::vec3(hp, 0.0f, 0.0f)), glm::vec3(dmg, 1.0f, 1.0f));
	fShader->ModelView(newModel);
	fShader->Color(glm::vec4(1.0f, 0.0f, 0.0f, alpha));
	glDrawArrays(GL_TRIANGLES, 0, sizeof vertexData / sizeof (vertex));
	gNumDraw++;

	if (fillEnd) {
		// Fill all the way to the end with white.
		float prevhp = hp+dmg;
		newModel = glm::scale(glm::translate(model, glm::vec3(prevhp, 0.0f, 0.0f)), glm::vec3(1.0f-prevhp, 1.0f, 1.0f));
		fShader->ModelView(newModel);
		fShader->Color(glm::vec4(1.0f, 1.0f, 1.0f, alpha));
		glDrawArrays(GL_TRIANGLES, 0, sizeof vertexData / sizeof (vertex));
		gNumDraw++;
	}

	glBindVertexArray(0);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	fShader->DisableProgram();
	checkError("HealthBar::Draw");
}

// Draw the mana bar, using the same projection as for previous health bar
void HealthBar::DrawMana(const glm::mat4 &model, float mana) const {
	glBindVertexArray(fVao);
	fShader->EnableProgram();
	glDisable(GL_DEPTH_TEST); // TODO: Should save the old setting
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	const float alpha = 0.4f;

	fShader->ModelView(glm::scale(model, glm::vec3(mana, 1.0f, 1.0f)));
	fShader->Color(glm::vec4(0.0f, 0.0f, 1.0f, alpha));
	glDrawArrays(GL_TRIANGLES, 0, sizeof vertexData / sizeof (vertex));
	gNumDraw++;

	glBindVertexArray(0);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	fShader->DisableProgram();
	checkError("HealthBar::DrawMana");
}

// Draw the mana bar, using the same projection as for previous health bar
void HealthBar::DrawExp(const glm::mat4 &model, float exp) const {
	glBindVertexArray(fVao);
	fShader->EnableProgram();
	glDisable(GL_DEPTH_TEST); // TODO: Should save the old setting
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	const float alpha = 0.4f;

	fShader->ModelView(glm::scale(model, glm::vec3(exp, 1.0f, 1.0f)));
	fShader->Color(glm::vec4(1.0f, 0.0f, 1.0f, alpha));
	glDrawArrays(GL_TRIANGLES, 0, sizeof vertexData / sizeof (vertex));
	gNumDraw++;

	glBindVertexArray(0);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	fShader->DisableProgram();
	checkError("HealthBar::DrawExp");
}

void HealthBar::DrawSquare(const glm::mat4 &model, float r, float g, float b, float a) const {
	glBindVertexArray(fVao);
	fShader->EnableProgram();
	glDisable(GL_DEPTH_TEST); // TODO: Should save the old setting
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	fShader->Projection(glm::mat4(1));
	fShader->ModelView(model);
	fShader->Color(glm::vec4(r, g, b, a));
	glDrawArrays(GL_TRIANGLES, 0, sizeof vertexData / sizeof (vertex));
	gNumDraw++;

	glBindVertexArray(0);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	fShader->DisableProgram();
	checkError("HealthBar::DrawSquare");
}
