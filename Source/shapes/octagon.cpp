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

#include <stdio.h>
#include <stdlib.h>

#include "../primitives.h"
#include "octagon.h"
#include "../ui/Error.h"

Octagon gOctagon;

// Every coordinate is 2 dimensions
struct vertex {
	float v[2];
};

#define SQRT2 1.41421f
#define d (1.0f/(1.0f+SQRT2))

/// The vertices needed in an octagon
static const vertex vertexData[] = {
	{{  0,  0 }}, // Center
	{{  1,  d }},
	{{  d,  1 }},
	{{  -d,  1 }},
	{{  -1,  d }},
	{{  -1,  -d }},
	{{  -d,  -1 }},
	{{  d,  -1 }},
	{{  1,  -d }},
};

/// An octagon is defined as eight triangles
static const unsigned short indices[] = {
	0, 1, 2,
	0, 2, 3,
	0, 3, 4,
	0, 4, 5,
	0, 5, 6,
	0, 6, 7,
	0, 7, 8,
	0, 8, 1,
};

#define NELEM(v) (sizeof v / sizeof v[0])

Octagon::~Octagon() {
	if (glDeleteVertexArrays != 0) {
		glDeleteVertexArrays(1, &fVao);
	}
}

void Octagon::Init(void) {
	glGenVertexArrays(1, &fVao);
	glBindVertexArray(fVao);

	if (!fOpenglBuffer.BindArray(sizeof vertexData, vertexData)) {
		ErrorDialog("Octagon::Init: vertexData size is mismatch with input array\n");
	}

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof (vertex), 0); // GL_ARRAY_BUFFER must be bound when doing this.

	if (!fIndexBuffer.BindElementsArray(sizeof indices, indices)) {
		ErrorDialog("Octagon::Init: indices size is mismatch with input array\n");
	}

	glBindVertexArray(0);
	checkError("Octagon::Init");
}

void Octagon::Draw() {
	if (fVao == 0)
		this->Init();
	glBindVertexArray(fVao);
	glDrawElements(GL_TRIANGLES, NELEM(indices), GL_UNSIGNED_SHORT, 0);
	gNumDraw++;
	gDrawnQuads += 8;
	glBindVertexArray(0);
}
