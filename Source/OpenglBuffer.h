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

class OpenglBuffer
{
public:
	~OpenglBuffer();
	bool BindArray(GLsizeiptr size, const GLvoid *data, GLenum usage = GL_STATIC_DRAW);
	void ArraySubData(GLintptr offset, GLsizeiptr size, const GLvoid *data);
	int GetArraySize() const;
	void Release();
private:
	void Init();
	GLuint fBufferId = 0;
};
