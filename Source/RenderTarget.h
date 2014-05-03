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

#pragma once

#include <GL/glew.h>
#include <vector>

namespace View {

/// Manage render targets
///
/// These objects can only be created dynamically after an OpenGL context is available.
class RenderTarget
{
public:
	RenderTarget();
	~RenderTarget();

	static void Resize(GLsizei width, GLsizei height);

	/// Setup this texture as a render target.
	/// There must be a FBO bound before calling this function.
	void FramebufferTexture2D(GLenum attachment);

	GLuint GetTexture() const { return fRendertarget; }
private:
	static void ReleaseAll();
    RenderTarget(const RenderTarget&) = delete; // No support for copying the object

	GLuint fRendertarget = 0; // The texture
	static GLsizei fWidth, fHeight; // All targets have to have the same size (for now)
	static std::vector<GLuint> fFreeTextures;
	static int fNumAllocated;
};

}
