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

#include "RenderTarget.h"
#include "primitives.h"
#include "assert.h"
#include "Debug.h"

using namespace View;
using namespace gl33;

GLsizei RenderTarget::fWidth, RenderTarget::fHeight;
std::vector<GLuint> RenderTarget::fFreeTextures;
int RenderTarget::fNumAllocated;

void RenderTarget::Resize(GLsizei width, GLsizei height) {
	ASSERT(fNumAllocated == 0);
	ReleaseAll();
	fWidth = width;
	fHeight = height;
}

RenderTarget::RenderTarget() {
	ASSERT(fWidth != 0 && fHeight != 0);
	if (!fFreeTextures.empty()) {
		fRendertarget = fFreeTextures.back();
		fFreeTextures.pop_back();
	} else {
		glGenTextures(1, &fRendertarget);
		glBindTexture(GL_TEXTURE_2D, fRendertarget);
		glTexImage2D(GL_TEXTURE_2D, 0, static_cast<int>(GL_RGB), fWidth, fHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<int>(GL_LINEAR));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<int>(GL_LINEAR));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<int>(GL_CLAMP_TO_BORDER)); // Get a black border when outside
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<int>(GL_CLAMP_TO_BORDER));
		LPLOG("Allocate new (%d)", fRendertarget);
	}
	fNumAllocated++;
}

RenderTarget::~RenderTarget() {
	fNumAllocated--;
	fFreeTextures.push_back(fRendertarget);
}

void RenderTarget::ReleaseAll() {
	ASSERT(fNumAllocated == 0);
	// TODO: glDeleteTextures(fFreeTextures.size(), &fFreeTextures[0])
	for (auto target : fFreeTextures)
		glDeleteTextures(1, &target);

	fFreeTextures.clear();
}

void RenderTarget::FramebufferTexture2D(GLenum attachment) const {
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, fRendertarget, 0);
}
