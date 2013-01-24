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
#include <glm/gtc/matrix_inverse.hpp>
#include "MonsterShader.h"
#include "../primitives.h"

/// Using GLSW to define shader
static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"common.UniformBuffer",
	"monster.Vertex",
};

/// Using GLSW to define shader
static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"monsters.Fragment",
};

MonsterShader::MonsterShader() {
	fgFirstTextureIndex = -1;
	fgVertexIndex = -1;
	fgTexCoordIndex = -1;
	fModelViewMatrixIndex = -1;
	fModelMatrixIdx = -1;
	fCycleIndex = -1;
	fNormalIndex = -1;
	fNormalMatrixIdx = -1;
	fColorAddonIdx = -1;
	fSunIdx = -1;
	fAmbientIdx = -1;
}

MonsterShader::~MonsterShader() {
}

MonsterShader MonsterShader::fgSingleton;

MonsterShader *MonsterShader::Make(void) {
	if (fgSingleton.fgVertexIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Initglsw("MonsterShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void MonsterShader::PreLinkCallback(GLuint prg) {
	glBindFragDataLocation(prg, 0, "diffuseOutput");
	glBindFragDataLocation(prg, 1, "posOutput");
	glBindFragDataLocation(prg, 2, "normOutput");
	glBindFragDataLocation(prg, 3, "blendOutput");
}

void MonsterShader::GetLocations(void) {
	fModelViewMatrixIndex = ShaderBase::GetUniformLocation("modelViewMatrix");
	fModelMatrixIdx = ShaderBase::GetUniformLocation("modelMatrix");
	fgFirstTextureIndex = ShaderBase::GetUniformLocation("firstTexture");
	fNormalMatrixIdx = ShaderBase::GetUniformLocation("normalMatrix");
	fColorAddonIdx = ShaderBase::GetUniformLocation("colorAddon");
	fSunIdx = ShaderBase::GetUniformLocation("sunIntensity");
	fAmbientIdx = ShaderBase::GetUniformLocation("ambient");

	fgVertexIndex = ShaderBase::GetAttribLocation("vertex");
	fgTexCoordIndex = ShaderBase::GetAttribLocation("texCoord");
	fCycleIndex = ShaderBase::GetAttribLocation("cycle");
	fNormalIndex = ShaderBase::GetAttribLocation("normal");
	checkError("MonsterShader::GetLocations");
}

void MonsterShader::EnableVertexAttribArray(void) {
	glEnableVertexAttribArray(fgVertexIndex);
	if (fgTexCoordIndex != -1)
		glEnableVertexAttribArray(fgTexCoordIndex);
	glEnableVertexAttribArray(fNormalIndex);
}

void MonsterShader::EnableProgram(void) {
	glUseProgram(ShaderBase::Program());
}

void MonsterShader::DisableProgram(void) {
	glUseProgram(0);
}

void MonsterShader::ModelView(const glm::mat4 &model, const glm::mat4 &view, float sun, float ambient) {
	if (fModelMatrixIdx != -1) {
		glUniformMatrix4fv(fModelMatrixIdx, 1, GL_FALSE, &model[0][0]); // Send the modelView matrix to the shader
	}
	if (fModelViewMatrixIndex != -1) {
		glm::mat4 mat = view * model;
		glUniformMatrix4fv(fModelViewMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send the modelView matrix to the shader
	}
	// This is possible if it is an orthogonal matrix. Otherwise, an inverse transpose would be needed.
	glm::mat3 normalMatrix = glm::mat3(model);
	if (fNormalMatrixIdx != -1)
		glUniformMatrix3fv(fNormalMatrixIdx, 1, GL_FALSE, &normalMatrix[0][0]); // Send the normal matrix to the shader

	if (fSunIdx != -1)
		glUniform1f(fSunIdx, sun);

	if (fAmbientIdx != -1)
		glUniform1f(fAmbientIdx, ambient);
}

void MonsterShader::Cycle(unsigned char c) {
	if (fCycleIndex != -1)
		glUniform1ui(fCycleIndex, c);
}

void MonsterShader::ColorAddon(const float *v) {
	// The color addon is actually 4 floats, but only three are needed (skipping the alpha).
	if (fColorAddonIdx != -1)
		glUniform4fv(fColorAddonIdx, 1, v);
}

void MonsterShader::VertexAttribPointer(GLsizei stride, const GLvoid *offset) {
	glVertexAttribPointer(fgVertexIndex, 3, GL_FLOAT, GL_FALSE, stride, offset);
}

void MonsterShader::TextureAttribPointer(GLsizei stride, const GLvoid *offset) {
	if (fgTexCoordIndex != -1)
		glVertexAttribPointer(fgTexCoordIndex, 2, GL_FLOAT, GL_FALSE, stride, offset);
}

void MonsterShader::NormalAttribPointer(GLsizei stride, const GLvoid *offset) {
	glVertexAttribPointer(fNormalIndex, 3, GL_FLOAT, GL_FALSE, stride, offset);
}
