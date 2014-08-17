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

#include <glbinding/gl/functions33.h>
#include <glbinding/gl/enum33.h>

#include "fboflat.h"
#include "errormanager.h"
#include "primitives.h"

using namespace gl33;

FBOFlat::FBOFlat() : fbo(0) {
}

FBOFlat::~FBOFlat() {
	if (fbo != 0) {
		glDeleteFramebuffers(1, &fbo);
	}
}

void FBOFlat::AttachTexture(GLuint textureId) {
	if (fbo == 0) {
		// Only needed first time
		glGenFramebuffers(1, &fbo);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		auto &ss = View::gErrorManager.GetStream(false, false);
		ss << "FBOFlat::AttachTexture: FrameBuffer incomplete: " << FrameBufferError(fboStatus) << " (" << fboStatus << ")";
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBOFlat::AttachDepthBuffer(GLuint textureId) {
	if (fbo == 0) {
		// Only needed first time
		glGenFramebuffers(1, &fbo);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureId, 0);
	glDrawBuffer(GL_NONE); // Need to disable this as there is no color buffer attached.
	glReadBuffer(GL_NONE);
	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		auto &ss = View::gErrorManager.GetStream(false, false);
		ss << "FBOFlat::AttachDepthBuffer: FrameBuffer incomplete: " << FrameBufferError(fboStatus) << fboStatus;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBOFlat::EnableReading() const {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
}

void FBOFlat::EnableWriting(GLenum buf) const {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	GLenum windows[] = { buf };
	glDrawBuffers(1, windows);
}
