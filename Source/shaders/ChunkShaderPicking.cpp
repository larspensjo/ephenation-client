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
#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ChunkShaderPicking.h"
#include "../primitives.h"


static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"precision mediump float;\n",
	"const float VERTEXSCALING="  STR(VERTEXSCALING) ";" // The is the scaling factor used for vertices
	"uniform mat4 projectionMatrix;\n",
	"uniform mat4 modelMatrix;\n",
	"uniform mat4 viewMatrix;\n",
	"in vec3 normal;\n",
	"in vec3 vertex;\n",
	"out vec3 fragmentNormal;\n",
	"void main(void)\n",
	"{\n",
	"	vec4 vertexScaled = vec4(vec3(vertex) / VERTEXSCALING, 1);"
	// Special scaling applies for the normal in picking mode.
	"   fragmentNormal = normal/255;\n", // The normal will be coded into the color, scaled from range 0..255 to 0..1.
	"	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vertexScaled;\n",
	"}\n",
};

static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"precision mediump float;\n",
	"in vec3 fragmentNormal;\n",
	// "layout(location = 0, index = 0) out vec4 fragColor;\n",
	"void main(void)\n",
	"{\n",
	"	gl_FragColor = vec4(fragmentNormal,1.0);\n",
	"}\n",
};

ChunkShaderPicking *ChunkShaderPicking::Make(void) {
	if (fgSingleton.fNormalIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Init("ChunkShaderPicking", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void ChunkShaderPicking::GetLocations(void) {
	fProjectionMatrixIndex = this->GetUniformLocation("projectionMatrix");
	fViewMatrixIndex = this->GetUniformLocation("viewMatrix");
	fModelMatrixIndex = this->GetUniformLocation("modelMatrix");

	fVertexIndex = this->GetAttribLocation("vertex");
	fNormalIndex = this->GetAttribLocation("normal");
	checkError("DrawText::DrawTextInit");
}

void Projection(glm::mat4 &);

void ChunkShaderPicking::EnableVertexAttribArray(void) {
	glEnableVertexAttribArray(fNormalIndex);
	glEnableVertexAttribArray(fVertexIndex);
}

void ChunkShaderPicking::EnableProgram(void) {
	glUseProgram(this->Program());
}

void ChunkShaderPicking::DisableProgram(void) {
	glUseProgram(0);
}

void ChunkShaderPicking::Model(const glm::mat4 &mat) {
	if (fModelMatrixIndex != -1)
		glUniformMatrix4fv(fModelMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void ChunkShaderPicking::View(const glm::mat4 &mat) {
	if (fViewMatrixIndex != -1)
		glUniformMatrix4fv(fViewMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void ChunkShaderPicking::Projection(const glm::mat4 &mat) {
	if (fProjectionMatrixIndex != -1)
		glUniformMatrix4fv(fProjectionMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void ChunkShaderPicking::VertexAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer) {
	glVertexAttribPointer(fVertexIndex, 3, type, GL_FALSE, stride, pointer);
}

void ChunkShaderPicking::NormalAttribPointer(GLenum type, GLsizei stride, const GLvoid * pointer) {
	glVertexAttribPointer(fNormalIndex, 3, type, GL_FALSE, stride, pointer);
}

ChunkShaderPicking::ChunkShaderPicking() {
	fProjectionMatrixIndex = -1;
	fViewMatrixIndex = -1;
	fModelMatrixIndex = -1;

	fNormalIndex = -1;
	fVertexIndex = -1;
}

ChunkShaderPicking::~ChunkShaderPicking() {
	// TODO Auto-generated destructor stub
}

ChunkShaderPicking ChunkShaderPicking::fgSingleton;
