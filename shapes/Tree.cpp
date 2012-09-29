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

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "Tree.h"
#include "../simplexnoise1234.h"
#include "../primitives.h"
#include "../textures.h"
#include "../shaders/ChunkShader.h"
#include "../ui/Error.h"
#include "../Options.h"

using glm::vec4;
using glm::vec3;

Tree::Tree() :
	fLeafBufferId(0), fBranchBufferId(0),
	fVaoLeaf(0), fVaoBranch(0) {
}

Tree::~Tree() {
	if (fVaoLeaf != 0) {
		// In case of glew not having run yet.
		glDeleteVertexArrays(1, &fVaoLeaf);
		glDeleteVertexArrays(1, &fVaoBranch);
		glDeleteBuffers(1, &fLeafBufferId);
		glDeleteBuffers(1, &fBranchBufferId);
	}
}

void Tree::InitStatic() {
	if (!sfInitialized) {
		sfInitialized = true;
		switch (Options::fgOptions.fPerformance) {
		default:
		case 1:
			sfBigTree.Init(3, 5, 10.0f);
			sfMediumTree.Init(2, 4, 6.0f);
			sfSmallTree.Init(2, 4, 3.0f);
			break;
		case 2:
			sfBigTree.Init(4, 4, 10.0f);
			sfMediumTree.Init(3, 4, 6.0f);
			sfSmallTree.Init(3, 3, 3.0f);
			break;
		case 3:
			sfBigTree.Init(4, 5, 10.0f);
			sfMediumTree.Init(4, 4, 6.0f);
			sfSmallTree.Init(3, 4, 3.0f);
			break;
		case 4:
			sfBigTree.Init(4, 6, 10.0f);
			sfMediumTree.Init(4, 5, 6.0f);
			sfSmallTree.Init(4, 4, 3.0f);
			break;
		}
	}
}

// Don't save leafs to 'this', it will be done by the top node (in the Init function).
void Tree::iter(const glm::mat4 &transf, int numIter, int branching, float height) {
	if (numIter == 0) {
		this->AddOneLeaf(transf, height);
	} else {
		this->AddCylinder(2+numIter, glm::scale(transf, vec3(height/20.0f, height, height/20.0f))); // Add one branch
		// Compute the position (tranformation of the child nodes)
		const glm::mat4 id(1);
		const glm::mat4 T = glm::translate(id, vec3(0.0f, height, 0.0f));
		const glm::mat4 S = glm::scale(id, vec3(0.5f, 0.5f, 0.5f));
		float deltaAngle = 360.0f/branching;
		for (int i=0; i<branching; i++) {
			// Use simplex noise as a pure random number generator
			float dip = 45.0f + 35.0f*snoise3(numIter*1239.7123f, i*122.0821f, branching*413.8731f);
			// Spread the branches evenly around the trunk, but with a little random delta angle
			float rot = i * deltaAngle + deltaAngle * snoise3(i*123.12371f, numIter*234.0921f, height*3121.271f) / 2;
			glm::mat4 R1 = glm::rotate(id, rot, vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 R2 = glm::rotate(id, dip, vec3(0.0f, 0.0f, 1.0f));
			this->iter(transf*T*R1*R2*S, numIter-1, branching, height);
		}
	}
}

// Create a tree, and scale it up.
void Tree::Init(int numIter, int branching, float height) {
	// Create the geometry
	iter(glm::mat4(1), numIter, branching, height);

	// Transfer the leaves to a VBO
	glGenVertexArrays(1, &fVaoLeaf);
	glBindVertexArray(fVaoLeaf);
	StageOneShader::EnableVertexAttribArray();
	glGenBuffers(1, &fLeafBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, fLeafBufferId);
	glBufferData(GL_ARRAY_BUFFER, fLeafTriangles.size() * sizeof fLeafTriangles[0], &fLeafTriangles[0], GL_STATIC_DRAW);
	// check data size in VBO is same as input array, if not return 0 and delete VBO
	GLint bufferSize;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	if ((unsigned)bufferSize != fLeafTriangles.size() * sizeof fLeafTriangles[0]) {
		glDeleteBuffers(1, &fLeafBufferId);
		fLeafBufferId = 0;
		ErrorDialog("[Tree::Init leafs] Data size is mismatch with input array\n");
	}
	StageOneShader::VertexAttribPointer();

	// Transfer the tunk and branches to a VBO.
	glGenVertexArrays(1, &fVaoBranch);
	glBindVertexArray(fVaoBranch);
	StageOneShader::EnableVertexAttribArray();
	glGenBuffers(1, &fBranchBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, fBranchBufferId);
	glBufferData(GL_ARRAY_BUFFER, fBranchTriangles.size() * sizeof fBranchTriangles[0], &fBranchTriangles[0], GL_STATIC_DRAW);
	// check data size in VBO is same as input array, if not return 0 and delete VBO
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	if ((unsigned)bufferSize != fBranchTriangles.size() * sizeof fBranchTriangles[0]) {
		glDeleteBuffers(1, &fBranchBufferId);
		fLeafBufferId = 0;
		ErrorDialog("[Tree::Init branches] Data size is mismatch with input array\n");
	}
	StageOneShader::VertexAttribPointer();
	glBindVertexArray(0);

	checkError("Tree::Init");
}

void Tree::Draw(void) const {
	if (fLeafTriangles.size() > 0) {
		glBindTexture(GL_TEXTURE_2D, GameTexture::Branch);
		glBindVertexArray(fVaoLeaf);
		gDrawnQuads += fLeafTriangles.size();
		glDrawArrays(GL_TRIANGLES, 0, fLeafTriangles.size()*3);
		gNumDraw++;
		glBindVertexArray(0);
	}

	if (fBranchTriangles.size() > 0) {
		glBindTexture(GL_TEXTURE_2D, GameTexture::TreeBarkId);
		glBindVertexArray(fVaoBranch);
		gDrawnQuads += fBranchTriangles.size();
		glDrawArrays(GL_TRIANGLES, 0, fBranchTriangles.size()*3);
		gNumDraw++;
		glBindVertexArray(0);
	}
}

void Tree::AddCylinder(int numSegments, const glm::mat4 &transf) {
	// The form of the cylinder is a radius of size 1 and a height of size 1. Use 'transf' to get other dimensions.
	// Using 'transf' as normal transformation only works as long as scaling in 'x' and 'z' is the same. Otherwise,
	// the more correct matrix would be something like the inverse transpose of 'transf'.
	glm::mat3 transfNormal = glm::mat3(transf);
	float deltaAngle = 2*PI/numSegments;
	float radius = 0.5f;

	vec4 v1(radius, 0.0f, 0.0f, 1);
	vec4 v2(radius, 1.0f, 0.0f, 1);

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
		vec4 v3(radius*c, 0.0f, radius*s, 1);
		vec4 v4(radius*c, 1.0f, radius*s, 1);
		vec3 norm1 = transfNormal * vec3(v1);
		vec3 norm2 = transfNormal * vec3(v3);

		tri.v[0].SetVertex(vec3(transf * v1));
		tri.v[0].SetNormal(norm1);
		tri.v[0].SetTexture(float(segment)/numSegments, 0);
		tri.v[1].SetVertex(vec3(transf * v2));
		tri.v[1].SetNormal(norm1);
		tri.v[1].SetTexture(float(segment)/numSegments, 1);
		tri.v[2].SetVertex(vec3(transf * v3));
		tri.v[2].SetNormal(norm2);
		tri.v[2].SetTexture(float(segment+1)/numSegments, 0);
		fBranchTriangles.push_back(tri);

		tri.v[0].SetVertex(vec3(transf * v4));
		tri.v[0].SetNormal(norm2);
		tri.v[0].SetTexture(float(segment+1)/numSegments, 1);
		tri.v[1].SetVertex(vec3(transf * v3));
		tri.v[1].SetNormal(norm2);
		tri.v[1].SetTexture(float(segment+1)/numSegments, 0);
		tri.v[2].SetVertex(vec3(transf * v2));
		tri.v[2].SetNormal(norm1);
		tri.v[2].SetTexture(float(segment+0)/numSegments, 1);
		fBranchTriangles.push_back(tri);
		v1 = v3;
		v2 = v4;
	}
}

void Tree::AddOneLeaf(const glm::mat4 &transf, float height) {
	const float w = 1.3f; // Width of leaf
	const float l = w*2;  // Length of leaf
	// Create a double sided leaf, visible in both directions
	vec3 v1 = vec3(transf*vec4(0.0f*height, 0.0f*height, 0.0f, 1.0f));
	vec3 v2 = vec3(transf*vec4(-w*height, w*height, 0.0f, 1.0f));
	vec3 v3 = vec3(transf*vec4(w*height, w*height, 0.0f, 1.0f));
	vec3 v4 = vec3(transf*vec4(0.0f*height, l*height, 0.0f, 1.0f));
	//
	//   v4
	// v2  v3
	//   v1
	//
	TriangleSurfacef tri;
	vec3 norm = glm::normalize(glm::cross(vec3(v1-v3), vec3(v1-v2)));
	tri.v[0].SetIntensity(255); tri.v[1].SetIntensity(255); tri.v[2].SetIntensity(255);
	tri.v[0].SetAmbient(0.35f*255); tri.v[1].SetAmbient(0.35f*255); tri.v[2].SetAmbient(0.35f*255); // Same for all triangles. Use 30% ambient light

	// Front side of leaf
	tri.v[0].SetVertex(v1);
	tri.v[1].SetVertex(v3);
	tri.v[2].SetVertex(v2);
	tri.v[0].SetNormal(norm);
	tri.v[1].SetNormal(norm);
	tri.v[2].SetNormal(norm);
	tri.v[0].SetTexture(0.0f, 0.0f);
	tri.v[1].SetTexture(1.0f, 0.0f);
	tri.v[2].SetTexture(0.0f, 1.0f);
	fLeafTriangles.push_back(tri);

	tri.v[0].SetVertex(v2);
	tri.v[1].SetVertex(v3);
	tri.v[2].SetVertex(v4);
	tri.v[0].SetNormal(norm);
	tri.v[1].SetNormal(norm);
	tri.v[2].SetNormal(norm);
	tri.v[0].SetTexture(0.0f, 1.0f);
	tri.v[1].SetTexture(1.0f, 0.0f);
	tri.v[2].SetTexture(1.0f, 1.0f);
	fLeafTriangles.push_back(tri);

	// Back side of leaf
	const float epsilon = -0.2f;
	// Create a leaf, visible in both directions
	v1 = vec3(transf*vec4(0.0f*height, 0.0f*height, epsilon, 1.0f));
	v2 = vec3(transf*vec4(-w*height, w*height, epsilon, 1.0f));
	v3 = vec3(transf*vec4(w*height, w*height, epsilon, 1.0f));
	v4 = vec3(transf*vec4(0.0f*height, l*height, epsilon, 1.0f));
	// v1.z = epsilon; v2.z = epsilon; v3.z = epsilon; v4.z = epsilon;
	tri.v[0].SetVertex(v1);
	tri.v[1].SetVertex(v2);
	tri.v[2].SetVertex(v3);
	tri.v[0].SetNormal(-norm);
	tri.v[1].SetNormal(-norm);
	tri.v[2].SetNormal(-norm);
	tri.v[0].SetTexture(0.0f, 0.0f);
	tri.v[1].SetTexture(0.0f, 1.0f);
	tri.v[2].SetTexture(1.0f, 0.0f);
	fLeafTriangles.push_back(tri);

	tri.v[0].SetVertex(v2);
	tri.v[1].SetVertex(v4);
	tri.v[2].SetVertex(v3);
	tri.v[0].SetNormal(-norm);
	tri.v[1].SetNormal(-norm);
	tri.v[2].SetNormal(-norm);
	tri.v[0].SetTexture(0.0f, 1.0f);
	tri.v[1].SetTexture(1.0f, 1.0f);
	tri.v[2].SetTexture(1.0f, 0.0f);
	fLeafTriangles.push_back(tri);
}

Tree Tree::sfBigTree, Tree::sfMediumTree, Tree::sfSmallTree;
bool Tree::sfInitialized;
