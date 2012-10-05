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
#include <stdio.h>

#include "Cylinder.h"
#include "../primitives.h"
#include "../shaders/ChunkShader.h"
#include "../ui/Error.h"

Cylinder::Cylinder() : fBufferId(0), fVao(0) {
}

Cylinder::~Cylinder() {
	if (glDeleteBuffers != 0) {
		// In case of glew not having run yet.
		glDeleteBuffers(1, &fBufferId);
		glDeleteVertexArrays(1, &fVao);
	}
}

void Cylinder::Init(StageOneShader *shader, int numSegments) {
	float deltaAngle = 2*M_PI/numSegments;
	float radius = 0.5f;

	glm::vec3 v1(radius, 0.0f, 0.0f);
	glm::vec3 v2(radius, 1.0f, 0.0f);

	TriangleSurfacef tri;
	tri.v[0].SetIntensity(255); tri.v[1].SetIntensity(255); tri.v[2].SetIntensity(255); // Same for all triangles
	tri.v[0].SetAmbient(0.30f*255); tri.v[1].SetAmbient(0.30f*255); tri.v[2].SetAmbient(0.30f*255); // Same for all triangles. Use 30% ambient light
	for (int segment=0; segment < numSegments; segment++) {
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
		// glm::vec3 l1 = glm::vec3(v3 - v1);
		// glm::vec3 l2 = glm::vec3(v2 - v1);
		glm::vec3 norm1 = glm::normalize(glm::vec3(v1.x, v1.y, 0));
		glm::vec3 norm2 = glm::normalize(glm::vec3(v3.x, v3.y, 0));
		tri.v[0].SetVertex(v1);
		tri.v[0].SetNormal(norm1);
		tri.v[0].SetTexture(float(segment)/numSegments, 0);
		tri.v[1].SetVertex(v3);
		tri.v[1].SetNormal(norm2);
		tri.v[1].SetTexture(float(segment+1)/numSegments, 0);
		tri.v[2].SetVertex(v2);
		tri.v[2].SetNormal(norm1);
		tri.v[2].SetTexture(float(segment)/numSegments, 1);
		fVisibleTriangles.push_back(tri);

		tri.v[0].SetVertex(v4);
		tri.v[0].SetNormal(norm2);
		tri.v[0].SetTexture(float(segment+1)/numSegments, 1);
		tri.v[1].SetVertex(v2);
		tri.v[1].SetNormal(norm1);
		tri.v[1].SetTexture(float(segment)/numSegments, 1);
		tri.v[2].SetVertex(v3);
		tri.v[2].SetNormal(norm2);
		tri.v[2].SetTexture(float(segment+1)/numSegments, 0);
		fVisibleTriangles.push_back(tri);
		v1 = v3;
		v2 = v4;
	}

	glGenVertexArrays(1, &fVao);
	glBindVertexArray(fVao);
	shader->EnableVertexAttribArray();
	glGenBuffers(1, &fBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, fBufferId);
	glBufferData(GL_ARRAY_BUFFER, fVisibleTriangles.size() * sizeof fVisibleTriangles[0], &fVisibleTriangles[0], GL_STATIC_DRAW);
	// check data size in VBO is same as input array, if not return 0 and delete VBO
	int bufferSize = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	if ((unsigned)bufferSize != fVisibleTriangles.size() * sizeof fVisibleTriangles[0]) {
		glDeleteBuffers(1, &fBufferId);
		fBufferId = 0;
		ErrorDialog("Cylinder::Init: Data size is mismatch with input array\n");
	}

	shader->VertexAttribPointer();
	glBindVertexArray(0);
	// DumpTriangles(fVisibleTriangles, fSizeVisibleTriangles/3);
}

void Cylinder::Draw(StageOneShader *shader) const {
	glFrontFace(GL_CW); // Cylinders are drawn in clock wise order
	glBindVertexArray(fVao);
	glDrawArrays(GL_TRIANGLES, 0, fVisibleTriangles.size()*3);
	gNumDraw++;
	glBindVertexArray(0);
	gDrawnQuads += fVisibleTriangles.size();
	glFrontFace(GL_CCW);
}
