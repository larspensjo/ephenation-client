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

//
// Provide data and functionality common to shaders.
//

#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>

/// Manage a uniform buffer
/// See common.glsl, with the shader definition of the buffer
class UniformBuffer {
public:
	/// Clear all data
	UniformBuffer();

	/// Free buffers
	~UniformBuffer();

	/// Do the actual buffer allocation
	void Init();

	/// Update all data in the uniform buffer
	/// It is a const function as no parameters in the class are changed.
	void Update(void) const;

	/// Call once for each program during initialization
	void UniformBlockBinding(GLuint program, GLuint idx);

	/// Set the camera data.
	/// It will not be used until Update() has been called.
	void Camera(const glm::vec4 &);
private:
	void BindBufferBase(void) const;

	GLuint fUBOBuffer;

	glm::vec4 fCamera;
};

extern UniformBuffer gUniformBuffer;
