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

#pragma once

#include <GL/glew.h>

/// Set up an FBO for one single bitmap target, no depth buffer
///
/// The viewport has to be controlled elsewhere.
class FBOFlat {
public:
	FBOFlat();
	~FBOFlat();

	/// Initialize the FBO.
	/// @param textureId The texture to use with this FBO
	void Attach(GLuint textureId);

	/// Enable the FBO for reading
	void EnableReading() const;

	/// Enable the FBO for writing
	void EnableWriting() const;
private:
	GLuint fbo;
};
