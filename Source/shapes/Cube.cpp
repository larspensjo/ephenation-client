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

#include <math.h>
#include <GL/glew.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

#include "Cube.h"
#include "../primitives.h"
#include "../shaders/StageOneShader.h"
#include "../ui/Error.h"

Cube gLantern;

Cube::Cube() : fBufferId(0), fVao(0) {
}

Cube::~Cube() {
	if (glDeleteBuffers != 0) {
		// In case of glew not having run yet.
		glDeleteBuffers(1, &fBufferId);
		glDeleteVertexArrays(1, &fVao);
	}
}

// The following logic is copied from Cylinder, which explains some algorithms.
// TODO: Should be redesigned using a predefined table.
void Cube::Init(bool invertedNormals) {
	const int numSegments = 4;
	float deltaAngle = 2*M_PI/numSegments;
	float radius = 0.5f;
	float normalFactor = 1.0f;

	if (invertedNormals)
		normalFactor = -1.0f;

	glm::vec3 v1(radius, 0.0f, 0.0f);
	glm::vec3 v2(radius, 1.0f, 0.0f);
	glm::vec3 top[4]; // The top 4 coordinates
	glm::vec3 bottom[4]; // The bottom 4 coordinates

	TriangleSurfacef tri;
	tri.v[0].SetIntensity(255); tri.v[1].SetIntensity(255); tri.v[2].SetIntensity(255); // Same for all triangles
	tri.v[0].SetAmbient(0.30f*255); tri.v[1].SetAmbient(0.30f*255); tri.v[2].SetAmbient(0.30f*255); // Same for all triangles. Use 30% ambient light
	for (int segment=0; segment < numSegments; segment++) {
		top[segment] = v2;
		bottom[segment] = v1;
		float angle = (segment+1) * deltaAngle;
		float s, c;
#if defined(WIN32) || defined (__APPLE__)
		s = sin(angle);
		c = cos(angle);
#else
		sincosf(angle, &s, &c);
#endif
		glm::vec3 v3(radius*c, 0.0f, radius*s);
		glm::vec3 v4(radius*c, 1.0f, radius*s);
		glm::vec3 norm1 = normalFactor * glm::normalize(glm::vec3(v1.x, v1.y, 0));
		tri.v[0].SetVertex(v1);
		tri.v[0].SetNormal(norm1);
		tri.v[0].SetTexture(0, 0);
		tri.v[1].SetVertex(v2);
		tri.v[1].SetNormal(norm1);
		tri.v[1].SetTexture(0, 1);
		tri.v[2].SetVertex(v3);
		tri.v[2].SetNormal(norm1);
		tri.v[2].SetTexture(1, 0);
		fVisibleTriangles.push_back(tri);

		// Second triangle, same normals
		tri.v[0].SetVertex(v4);
		tri.v[0].SetTexture(1, 1);
		tri.v[1].SetVertex(v3);
		tri.v[1].SetTexture(1, 0);
		tri.v[2].SetVertex(v2);
		tri.v[2].SetTexture(0, 1);
		fVisibleTriangles.push_back(tri);
		v1 = v3;
		v2 = v4;
	}

	// Define the top 2 triangles.
	tri.v[0].SetVertex(top[0]);
	tri.v[0].SetNormal(normalFactor*glm::vec3(0,1,0));
	tri.v[0].SetTexture(1, 0);
	tri.v[1].SetVertex(top[2]);
	tri.v[1].SetNormal(normalFactor*glm::vec3(0,1,0));
	tri.v[1].SetTexture(0, 1);
	tri.v[2].SetVertex(top[1]);
	tri.v[2].SetNormal(normalFactor*glm::vec3(0,1,0));
	tri.v[2].SetTexture(1, 1);
	fVisibleTriangles.push_back(tri);

	tri.v[0].SetVertex(top[0]);
	tri.v[0].SetTexture(1, 0);
	tri.v[1].SetVertex(top[3]);
	tri.v[1].SetTexture(0, 0);
	tri.v[2].SetVertex(top[2]);
	tri.v[2].SetTexture(0, 1);
	fVisibleTriangles.push_back(tri);

	// Define the bottom 2 triangles.
	tri.v[0].SetVertex(bottom[0]);
	tri.v[0].SetNormal(normalFactor*glm::vec3(0,-1,0));
	tri.v[0].SetTexture(1, 0);
	tri.v[1].SetVertex(bottom[1]);
	tri.v[1].SetNormal(normalFactor*glm::vec3(0,-1,0));
	tri.v[1].SetTexture(1, 1);
	tri.v[2].SetVertex(bottom[2]);
	tri.v[2].SetNormal(normalFactor*glm::vec3(0,-1,0));
	tri.v[2].SetTexture(0, 1);
	fVisibleTriangles.push_back(tri);

	tri.v[0].SetVertex(bottom[0]);
	tri.v[0].SetTexture(1, 0);
	tri.v[1].SetVertex(bottom[2]);
	tri.v[1].SetTexture(0, 1);
	tri.v[2].SetVertex(bottom[3]);
	tri.v[2].SetTexture(0, 0);
	fVisibleTriangles.push_back(tri);

	glGenVertexArrays(1, &fVao);
	glBindVertexArray(fVao);
	StageOneShader::EnableVertexAttribArray();
	glGenBuffers(1, &fBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, fBufferId);
	glBufferData(GL_ARRAY_BUFFER, fVisibleTriangles.size() * sizeof fVisibleTriangles[0], &fVisibleTriangles[0], GL_STATIC_DRAW);
	// check data size in VBO is same as input array, if not return 0 and delete VBO
	int bufferSize = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	if ((unsigned)bufferSize != fVisibleTriangles.size() * sizeof fVisibleTriangles[0]) {
		glDeleteBuffers(1, &fBufferId);
		fBufferId = 0;
		ErrorDialog("Cube::Init: Data size is mismatch with input array\n");
	}

	StageOneShader::VertexAttribPointer();

	glBindVertexArray(0);
	// DumpTriangles(fVisibleTriangles, fVisibleTriangles.size());
}

void Cube::Draw(void) const {
	glBindVertexArray(fVao);
	glDrawArrays(GL_TRIANGLES, 0, fVisibleTriangles.size()*3);
	gNumDraw++;
	glBindVertexArray(0);
	gDrawnQuads += fVisibleTriangles.size();
}

void Cube::DrawLines(void) const {
	glBindVertexArray(fVao);
	glDrawArrays(GL_LINES, 0, fVisibleTriangles.size()*3);
	gNumDraw++;
	glBindVertexArray(0);
	gDrawnQuads += fVisibleTriangles.size();
	checkError("Cube::DrawLines");
}
