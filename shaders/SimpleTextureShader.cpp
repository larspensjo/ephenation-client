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

#include <GL/glew.h>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "SimpleTextureShader.h"
#include "../primitives.h"

// Minimum program for drawing text
static const GLchar *vertexShaderSource[] = {
	"#version 130\n", // This corresponds to OpenGL 3.0
	"precision mediump float;\n",
	"uniform mat4 projectionMatrix;\n",
	"uniform mat4 modelViewMatrix;\n",
	"uniform vec3 textOffsMulti;\n", // This contains both a offset and a multiplier
	"in vec3 vertex;\n",
	"in vec2 texCoord;\n",
	"out vec2 fragmentTexCoord;\n",
	"void main(void)\n",
	"{\n",
	"   float textMult = textOffsMulti.z;\n",
	"	fragmentTexCoord = texCoord*textMult + textOffsMulti.xy;\n",
	"	gl_Position = projectionMatrix * modelViewMatrix * vec4(vertex,1.0);\n",
	"}\n",
};

static const GLchar *fragmentShaderSource[] = {
	"#version 130\n", // This corresponds to OpenGL 3.0
	"precision mediump float;\n",
	"uniform sampler2D firstTexture;\n",
	"uniform float forceTransparent;\n",
	"uniform vec3 colorOffset;\n",
	"in vec2 fragmentTexCoord;\n",
	"void main(void)\n",
	"{\n",
	"	gl_FragColor = texture(firstTexture, fragmentTexCoord) + vec4(colorOffset, 0);\n",
	"   if (gl_FragColor.a < 0.1) discard;\n",
	"   if (forceTransparent < 1) gl_FragColor.a = forceTransparent;\n", // The transparency isn't used unless enabled.
	// "	gl_FragColor = vec4(1.0, 0.5, 0.25, 1.0);\n",
	"}\n",
};

SimpleTextureShader *SimpleTextureShader::Make(void) {
	if (fgSingleton.fgFirstTextureIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Init("SimpleTextureShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void SimpleTextureShader::GetLocations(void) {
	fgProjectionMatrixIndex = this->GetUniformLocation("projectionMatrix");
	fModelViewMatrixIndex = this->GetUniformLocation("modelViewMatrix");
	fgFirstTextureIndex = this->GetUniformLocation("firstTexture");
	fgForceTranspInd = this->GetUniformLocation("forceTransparent");
	fgVertexIndex = this->GetAttribLocation("vertex");
	fgTexCoordIndex = this->GetAttribLocation("texCoord");
	fTextOffsMultiInd = this->GetUniformLocation("textOffsMulti");
	fColorOffsetIdx = this->GetUniformLocation("colorOffset");
	this->TextureOffsetMulti(0.0f, 0.0f, 1.0f);
	checkError("DrawText::DrawTextInit");
}

void Projection(glm::mat4 &);

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
	if (fgProjectionMatrixIndex != -1) {
		glUniformMatrix4fv(fgProjectionMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
	}
	checkError("SimpleTextureShader::Projection");
}

void SimpleTextureShader::ModelView(const glm::mat4 &mat) {
	if (fModelViewMatrixIndex != -1) {
		glUniformMatrix4fv(fModelViewMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
	}
	checkError("SimpleTextureShader::ModelView");
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
	fgFirstTextureIndex = -1;
	fgVertexIndex = -1;
	fgTexCoordIndex = -1;
	fModelViewMatrixIndex = -1;
	fgForceTranspInd = -1;
	fTextOffsMultiInd = -1;
	fColorOffsetIdx = -1;
}

SimpleTextureShader::~SimpleTextureShader() {
	// TODO Auto-generated destructor stub
}

SimpleTextureShader SimpleTextureShader::fgSingleton;
