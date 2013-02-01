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
#include "primitives.h"
#include "textures.h"
#include "BuildingBlocks.h"
#include "shaders/SimpleTextureShader.h"
#include "DrawText.h"
#include "errormanager.h"

// Every coordinate is 3 dimensions and two dimension texture
struct vertex {
	char v[3];
	char t[2];
};

// A square defined as two triangles.
static const vertex vertexData[] = {
	{{ 1, 1, -1, }, { 1, 1, }},
	{{ -1, -1, -1, }, { 0, 0, }},
	{{ -1, 1, -1, }, { 0, 1, }},
	{{ 1, 1, -1, }, { 1, 1, }},
	{{ 1, -1, -1, }, { 1, 0, }},
	{{ -1, -1, -1, }, { 0, 0, }},
};

void BuildingBlocks::UpdateSelection(int pos) {
	// const GameTexture *gt = GameTexture::GetGameTexture(0);
	gDrawFont.Enable();
	// gDrawFont.vsfl.prepareSentence(fSelectedBlockText, gt->descr);
	gDrawFont.Disable();
	fCurrentSelection = -pos;
	while (fCurrentSelection < 0)
		fCurrentSelection += GameTexture::fgNumBuildBlocks;
	while (fCurrentSelection >= GameTexture::fgNumBuildBlocks)
		fCurrentSelection -= GameTexture::fgNumBuildBlocks;
	const GameTexture *gt = GameTexture::GetGameTexture(this->fCurrentSelection);
	gDrawFont.vsfl.prepareSentence(fSelectedBlockText, gt->descr);
	// printf("BuildingBlocks::UpdateSelection to %d\n", fCurrentSelection);
}

int BuildingBlocks::CurrentBlockType(void) const {
	const GameTexture *gt = GameTexture::GetGameTexture(this->fCurrentSelection);
	return gt->blType;
}

// The selected block will be highlighted at this position
static const int sgDisplayOffset = 3;

void BuildingBlocks::Draw(glm::mat4 &projection) {
	float scale = 1.0f/fNumToDisplay/2.0f;
	float w = gViewport[2];
	float screenRatio = gViewport[2] / gViewport[3];
	glm::mat4 model(1.0f);
	float frac = (w/2.0f-50.0f)/w*2.0f;
	model = glm::translate(model, glm::vec3(frac, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(scale/screenRatio, scale, scale));
	glBindVertexArray(fVao);
	fShader->EnableProgram();
	// Draw things...
	fShader->Projection(glm::mat4(1.0f));
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST); // TODO: Should save the old setting

	fShader->ForceTransparent(0.3f); // Default, for all but one of them
	for (int i=0; i<this->fNumToDisplay; i++) {
		int ind = i + this->fCurrentSelection - sgDisplayOffset;
		while (ind < 0)
			ind += GameTexture::fgNumBuildBlocks;
		while (ind >= GameTexture::fgNumBuildBlocks)
			ind -= GameTexture::fgNumBuildBlocks;
		const GameTexture *gt = GameTexture::GetGameTexture(ind);
		glBindTexture(GL_TEXTURE_2D, gt->id);
		fShader->ModelView(glm::translate(model, glm::vec3(0.0f, 9.0f-i*3, 0.0f))); // View and projection is identity matrix
		if (ind == this->fCurrentSelection) {
			fShader->ForceTransparent(1.0f);
			glDisable(GL_BLEND);
			fShader->ForceTransparent(0.3f);
		} else {
			glEnable(GL_BLEND);
		}
		glDrawArrays(GL_TRIANGLES, 0, sizeof vertexData / sizeof (vertex));
		gNumDraw++;
	}
	glBindVertexArray(0);
	fShader->ForceTransparent(1.0f); // Restore default
	fShader->DisableProgram();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	gDrawFont.Enable();
	gDrawFont.UpdateProjection();
	const GameTexture *gt = GameTexture::GetGameTexture(this->fCurrentSelection);
	int textWidth = gDrawFont.vsfl.numberOfPixels(gt->descr); // TODO: Should not be done for every frame.
	gDrawFont.SetOffset(gViewport[2]-80-textWidth, gViewport[3]/2.0f-12);
	gDrawFont.vsfl.renderSentence(fSelectedBlockText);
	gDrawFont.Disable();

	glDisable(GL_BLEND);

	checkError("BuildingBlocks::Draw");
}

BuildingBlocks::BuildingBlocks() {
	fNumToDisplay = 0;
	fShader = 0;
	fSelectedBlockText = 0;
	fCurrentSelection = 0;
}

BuildingBlocks *BuildingBlocks::Make(int numToDisplay) {
	if (fgSingleton.fShader == 0) {
		fgSingleton.Init(numToDisplay);
	}
	return &fgSingleton;
}

void BuildingBlocks::Init(int numToDisplay) {
	fNumToDisplay = numToDisplay;
	fShader = SimpleTextureShader::Make();
	glGenVertexArrays(1, &fVao);
	glBindVertexArray(fVao); // Has to be done before enabling program, where the vertex attrib pointers are enabled.
	fShader->EnableVertexAttribArray();
	glGenBuffers(1, &fBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, fBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof vertexData, vertexData, GL_STATIC_DRAW);
	vertex *p = 0;
	fShader->TextureAttribPointer(GL_UNSIGNED_BYTE, sizeof (vertex), &p->t);
	fShader->VertexAttribPointer(GL_BYTE, 2, sizeof (vertex), &p->v);
	glBindVertexArray(0);
	// check data size in VBO is same as input array, if not return 0 and delete VBO
	int bufferSize = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	if ((unsigned)bufferSize != sizeof vertexData) {
		glDeleteBuffers(1, &fBufferId);
		auto &ss = View::gErrorManager.GetStream(false, false);
		ss << "[BuildingBlocks::Init] Data size is mismatch with input array";
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	fShader->DisableProgram();
	fSelectedBlockText = gDrawFont.vsfl.genSentence();
	BuildingBlocks::UpdateSelection(0); // Last thing
}

BuildingBlocks BuildingBlocks::fgSingleton;
