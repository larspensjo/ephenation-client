// Copyright 2014 The Ephenation Authors
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

#include "OpenglBuffer.h"
#include "assert.h"
#include "primitives.h"

using namespace gl33;

OpenglBuffer::~OpenglBuffer() {
	this->Release();
}

void OpenglBuffer::Release() {
	if (fBufferId == 0)
		return;
	glDeleteBuffers(1, &fBufferId);
	fBufferId = 0;
}

void OpenglBuffer::Init() {
	glGenBuffers(1, &fBufferId);
}

void OpenglBuffer::Bind(GLenum target) {
	fTarget = target;
	glBindBuffer(fTarget, fBufferId);
}

bool OpenglBuffer::BindArray(GLsizeiptr size, const GLvoid *data, GLenum usage) {
	if (fBufferId == 0)
		this->Init();
	this->Bind(GL_ARRAY_BUFFER);
	glBindBuffer(fTarget, fBufferId);
	glBufferData(fTarget, size, data, usage);
	return this->GetSize() == size;
}

bool OpenglBuffer::BindElementsArray(GLsizeiptr size, const GLvoid *data, GLenum usage) {
#ifdef DEBUG
	if (gDebugOpenGL) {
		GLint binding = 0;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &binding);
		ASSERT(binding != 0);
	}
#endif

	if (fBufferId == 0)
		this->Init();
	this->Bind(GL_ELEMENT_ARRAY_BUFFER);
	glBufferData(fTarget, size, data, usage);
	return this->GetSize() == size;
}

void OpenglBuffer::ArraySubData(GLintptr offset, GLsizeiptr size, const GLvoid *data) {
	ASSERT(fBufferId != 0);
	glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

int OpenglBuffer::GetSize() const {
	int bufferSize = 0;
	glGetBufferParameteriv(fTarget, GL_BUFFER_SIZE, &bufferSize);
	return bufferSize;
}
