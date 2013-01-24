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

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "SimpleTextureShader.h"
#include "../primitives.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"#version 130\n", // This corresponds to OpenGL 3.0
	"simpletexture.Vertex",
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"#version 130\n", // This corresponds to OpenGL 3.0
	"simpletexture.Fragment",
};

SimpleTextureShader *SimpleTextureShader::Make(void) {
	if (fgSingleton.fgProjectionMatrixIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Initglsw("SimpleTextureShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void SimpleTextureShader::GetLocations(void) {
	fgProjectionMatrixIndex = this->GetUniformLocation("projectionMatrix");
	fModelViewMatrixIndex = this->GetUniformLocation("modelViewMatrix");
	glUniform1i(this->GetUniformLocation("firstTexture"), 0); // Always at texture 0
	fgForceTranspInd = this->GetUniformLocation("forceTransparent");
	fgVertexIndex = this->GetAttribLocation("vertex");
	fgTexCoordIndex = this->GetAttribLocation("texCoord");
	fTextOffsMultiInd = this->GetUniformLocation("textOffsMulti");
	fColorOffsetIdx = this->GetUniformLocation("colorOffset");
	this->TextureOffsetMulti(0.0f, 0.0f, 1.0f);
}

void SimpleTextureShader::EnableVertexAttribArray(void) {
	glEnableVertexAttribArray(fgVertexIndex);
	glEnableVertexAttribArray(fgTexCoordIndex);
}

void SimpleTextureShader::EnableProgram(void) {
	glUseProgram(this->Program());
}

void SimpleTextureShader::DisableProgram(void) {
	glUseProgram(0);
}

void SimpleTextureShader::Projection(const glm::mat4 &mat) {
	glUniformMatrix4fv(fgProjectionMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void SimpleTextureShader::ModelView(const glm::mat4 &mat) {
	glUniformMatrix4fv(fModelViewMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void SimpleTextureShader::ForceTransparent(float alpha) {
	glUniform1f(fgForceTranspInd, alpha);
}

void SimpleTextureShader::TextureOffsetMulti(float offsX, float offsY, float mult) {
	glUniform3f(fTextOffsMultiInd, offsX, offsY, mult);
}

void SimpleTextureShader::SetColorOffset(const glm::vec3 &offs) {
	glUniform3fv(fColorOffsetIdx, 1, &offs[0]);
}

void SimpleTextureShader::VertexAttribPointer(GLenum type, GLint size, GLsizei stride, const GLvoid * pointer) {
	glVertexAttribPointer(fgVertexIndex, size, type, GL_FALSE, stride, pointer);
}

void SimpleTextureShader::TextureAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer) {
	glVertexAttribPointer(fgTexCoordIndex, 2, type, GL_FALSE, stride, pointer);
}

SimpleTextureShader::SimpleTextureShader() {
	fgProjectionMatrixIndex = -1;
	fgVertexIndex = -1;
	fgTexCoordIndex = -1;
	fModelViewMatrixIndex = -1;
	fgForceTranspInd = -1;
	fTextOffsMultiInd = -1;
	fColorOffsetIdx = -1;
}

SimpleTextureShader SimpleTextureShader::fgSingleton;
