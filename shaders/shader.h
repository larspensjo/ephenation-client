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

//
// This is an abstract shader class. Inherit it to create new shader programs.
//

class ShaderBase {
public:
	ShaderBase();

	// Initialize the shader program, and call back to setup uniform and attribute indices.
	void Init(const char *debug, int vertexShaderLines, const char **vertexShaderSource, int fragmentShaderLines, const char **fragmentShaderSource);
protected:
	GLuint Program(void) const { return this->fProgram; }
	// Define all uniform and attribute indices. This function must be overridden.
	virtual void GetLocations(void) = 0; // Every support class must override this
	virtual void PreLinkCallback(GLuint prg) {};  // A callback called before linkage, that can optionally be overrided
	GLint GetUniformLocation(const char *) const;
	GLint GetAttribLocation(const char *) const;
	GLuint GetUniformBlockIndex(const char *) const;
private:
	GLuint fProgram;

	void compileAndCheck(GLuint shader);
	GLuint compileShaderSource(GLenum type, GLsizei count, const GLchar **string);
	void linkAndCheck(const char *debug, GLuint program);
	GLuint createProgram(const char *debug, GLuint vertexShader, GLuint fragmentShader);
};
