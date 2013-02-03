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

#include "uniformbuffer.h"
#include "Options.h"
#include "primitives.h"
#include "render.h"
#include "player.h"

UniformBuffer gUniformBuffer;

UniformBuffer::UniformBuffer() : fUBOBuffer(0) {
}

UniformBuffer::~UniformBuffer() {
	if (fUBOBuffer != 0)
		glDeleteBuffers(1, &fUBOBuffer);
}

// This data description must match the content of the UBO.
// The content and layout must match UNIFORMBUFFER.
// "bool" didn't work, probably because of alignment definitions of layout(std140).
struct Data {
	glm::mat4 projectionmatrix;
	glm::mat4 projectionviewmatrix;
	glm::mat4 viewmatrix;
	glm::vec4 camera; // Only three floats needed, but std140 will align to 4 anyway.
	float viewingdistance;
	int performance;
	int dynamicshadows;
	int windowheight; // Height of window, in pixels.
	int toggleTesting;
	int belowGround; // True when player is below ground
	float exposure;
	float ambientLight;
};

// Use index 0 always
GLuint g_iGlobalMatricesBindingIndex = 0;

void UniformBuffer::Init(void) {
	glGenBuffers(1, &fUBOBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, fUBOBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(Data), NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	this->BindBufferBase();
	checkError("UniformBuffer::Init");
}

void UniformBuffer::Update(void) const {
	Data data;
	data.performance = gOptions.fPerformance;
	if (gOptions.fDynamicShadows)
		data.dynamicshadows = 1;
	else if (gOptions.fStaticShadows)
		data.dynamicshadows = 2;
	else
		data.dynamicshadows = 0;
	data.projectionmatrix = gProjectionMatrix;
	data.viewmatrix = gViewMatrix;
	data.viewingdistance = maxRenderDistance;
	data.camera = fCamera;
	data.projectionviewmatrix = gProjectionMatrix * gViewMatrix;
	data.windowheight = gViewport[3];
	data.toggleTesting = gToggleTesting;
	data.exposure = gOptions.fExposure;
	data.ambientLight = gOptions.fAmbientLight / 200.0f;
	data.belowGround = Model::gPlayer.BelowGround();

	glBindBuffer(GL_UNIFORM_BUFFER, fUBOBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Data), &data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::UniformBlockBinding(GLuint program, GLuint idx) {
	glUniformBlockBinding(program, idx, g_iGlobalMatricesBindingIndex);
}

// There was a call in chunk::ReleaseOpenGLBuffers of glDeleteBuffers() that destroyed
// this binding for AMD only. It works for NVIDIA.
void UniformBuffer::BindBufferBase(void) const {
	glBindBufferBase(GL_UNIFORM_BUFFER, g_iGlobalMatricesBindingIndex, fUBOBuffer);
}

void UniformBuffer::Camera(const glm::vec4 &c) {
	fCamera = c;
}
