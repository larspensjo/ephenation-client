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

#pragma once

#include <GL/glew.h>

#include "glm/glm.hpp"

class UniformBuffer {
public:
	UniformBuffer();
	~UniformBuffer();

	void Init();

	// Update all data
	void Update(void);

	// Call once for each program during initialization
	void UniformBlockBinding(GLuint program, GLuint idx);

	void Camera(const glm::vec4 &);
private:
	void BindBufferBase(void);

	GLuint fUBOBuffer;

	glm::vec4 fCamera;
};

extern UniformBuffer gUniformBuffer;

// The uniform buffer object is defined here, to be used by the shaders. That way,
// it can be ensured that all shaders get the same definition.
// By convention, all names begin with the prefix UBO.
#define UNIFORMBUFFER \
	"layout(std140) uniform GlobalData {"\
	"    mat4 UBOProjectionMatrix;"\
	"    mat4 UBOProjectionviewMatrix;"\
	"    mat4 UBOViewMatrix;"\
	"    vec4 UBOCamera;"\
	"    float UBOViewingDistance;"\
	"    int UBOPerformance;"\
	"    int UBODynamicshadows;"\
	"    int UBOWindowHeight;"\
	"    int UBOToggleTesting;"\
	"    float UBOexposure;"\
	"    float UBOambientLight;"\
	"};\n"
