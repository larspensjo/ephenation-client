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

#include <glbinding/gl/functions33.h>
#include <glbinding/gl/enum33.h>
#include <stdio.h>
#include <stdlib.h>

#include <glm/glm.hpp>
#include "../primitives.h"
#include "quad.h"
#include "../ui/Error.h"

using namespace gl33;

Quad gQuad;

// Every coordinate is 2 dimensions
struct vertex {
	float v[2];
};

// =====================================================
/**
 * @brief Every quad defined as two triangles
 */
// =====================================================
static const vertex vertexData[] = {
	{{  0,  0 }},
	{{  1,  0 }},
	{{  0,  1 }},
	{{  1,  1 }},
};

Quad::~Quad() {
	if (glDeleteVertexArrays != 0) {
		glDeleteVertexArrays(1, &fVao);
	}
}

void Quad::Init(void) {
	glGenVertexArrays(1, &fVao);
	glBindVertexArray(fVao);
	glEnableVertexAttribArray(0);
	if (!fBuffer.BindArray(sizeof vertexData, vertexData))
		ErrorDialog("Quad::Init: Data size is mismatch with input array\n");
	vertex *p = 0;
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof (vertex), &p->v);
	glBindVertexArray(0);
	checkError("Quad::Init");
}

void Quad::Draw() {
	if (fVao == 0)
		this->Init();
	glBindVertexArray(fVao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof vertexData / sizeof (vertex));
	gNumDraw++;
	gDrawnQuads += 2;
	glBindVertexArray(0);
}
