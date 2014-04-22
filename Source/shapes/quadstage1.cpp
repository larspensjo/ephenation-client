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

#include <math.h>
#include <GL/glew.h>
#include <stdio.h>

#include "quadstage1.h"
#include "../primitives.h"
#include "../shaders/StageOneShader.h"
#include "../ui/Error.h"

QuadStage1 gQuadStage1;

QuadStage1::~QuadStage1() {
	// In case of glew not having run yet.
	if (glDeleteBuffers != 0) {
		glDeleteVertexArrays(1, &fVao);
	}
}

// A square defined as two triangles. The full vertex list defines two quads facing
// in both directions. The first quad is drawn CCW.
static const VertexDataf vertexData[] = {
	//Normal              Text            Vertex                     Int  Amb
	VertexDataf(glm::vec3(0, 0,  1), glm::vec2(1,1), glm::vec3( 0.5,  0.5, 0 ), 100, 200),
	VertexDataf(glm::vec3(0, 0,  1), glm::vec2(0,1), glm::vec3(-0.5,  0.5, 0 ), 100, 200),
	VertexDataf(glm::vec3(0, 0,  1), glm::vec2(0,0), glm::vec3(-0.5, -0.5, 0 ), 100, 200),
	VertexDataf(glm::vec3(0, 0,  1), glm::vec2(1,1), glm::vec3( 0.5,  0.5, 0 ), 100, 200),
	VertexDataf(glm::vec3(0, 0,  1), glm::vec2(0,0), glm::vec3(-0.5, -0.5, 0 ), 100, 200),
	VertexDataf(glm::vec3(0, 0,  1), glm::vec2(1,0), glm::vec3( 0.5, -0.5, 0 ), 100, 200),

	VertexDataf(glm::vec3(0, 0, -1), glm::vec2(1,1), glm::vec3( 0.5,  0.5, 0 ), 100, 200),
	VertexDataf(glm::vec3(0, 0, -1), glm::vec2(0,0), glm::vec3(-0.5, -0.5, 0 ), 100, 200),
	VertexDataf(glm::vec3(0, 0, -1), glm::vec2(0,1), glm::vec3(-0.5,  0.5, 0 ), 100, 200),
	VertexDataf(glm::vec3(0, 0, -1), glm::vec2(1,1), glm::vec3( 0.5,  0.5, 0 ), 100, 200),
	VertexDataf(glm::vec3(0, 0, -1), glm::vec2(1,0), glm::vec3( 0.5, -0.5, 0 ), 100, 200),
	VertexDataf(glm::vec3(0, 0, -1), glm::vec2(0,0), glm::vec3(-0.5, -0.5, 0 ), 100, 200),
};

#define VERTICES (sizeof vertexData/sizeof (VertexDataf))

void QuadStage1::Init(void) {
	glGenVertexArrays(1, &fVao);
	glBindVertexArray(fVao);
	StageOneShader::EnableVertexAttribArray();

	if (!fBuffer.BindArray(sizeof vertexData, vertexData)) {
		ErrorDialog("QuadStage1::Init: Data size is mismatch with input array\n");
	}

	StageOneShader::VertexAttribPointer();

	glBindVertexArray(0);
}

void QuadStage1::DrawSingleSideInstances(int count) const {
	glBindVertexArray(fVao);
	glDrawArraysInstanced(GL_TRIANGLES, 0, VERTICES/2, count);
	glBindVertexArray(0);
	gNumDraw++;
	gDrawnQuads += VERTICES/2*count;
}

void QuadStage1::DrawDoubleSide(void) const {
	glBindVertexArray(fVao);
	glDrawArrays(GL_TRIANGLES, 0, VERTICES);
	gNumDraw++;
	glBindVertexArray(0);
	gDrawnQuads += VERTICES;
}

void QuadStage1::DrawSingleSide(void) const {
	glBindVertexArray(fVao);
	glDrawArrays(GL_TRIANGLES, 0, VERTICES/2);
	gNumDraw++;
	glBindVertexArray(0);
	gDrawnQuads += VERTICES/2;
}

void QuadStage1::DrawLines(void) const {
	glBindVertexArray(fVao);
	glDrawArrays(GL_LINES, 0, VERTICES);
	gNumDraw++;
	glBindVertexArray(0);
	gDrawnQuads += VERTICES;
}
