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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "MonsterShader.h"
#include "../primitives.h"
#include "../uniformbuffer.h"

// Minimum program for drawing text
static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	UNIFORMBUFFER
	"uniform mat4 modelViewMatrix;\n",
	"uniform mat3 normalMatrix;\n", // TODO: This can be computed from the model matrix
	"uniform mat4 modelMatrix;\n",
	"uniform int cycle;\n",
	"in vec3 vertex;\n",
	"in vec2 texCoord;\n",
	"in vec3 normal;\n",
	"out vec2 fragmentTexCoord;\n",
	"out vec3 fragNormal;\n",
	"out vec3 position;\n",
	"void main(void)\n",
	"{\n",
	"	fragmentTexCoord = texCoord/180;\n", // This will stretch the texture over several blocks.
	"	fragNormal = normalize(normalMatrix * normal);\n",
	"	gl_Position = UBOProjectionMatrix * modelViewMatrix * vec4(vertex,1.0);\n",
	"	position = vec3(modelMatrix * vec4(vertex, 1.0));\n", // Copy position to the fragment shader
	"}\n",
};

static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"uniform sampler2D firstTexture;\n",
	"uniform vec4 colorAddon;\n", // Allow the color to be tweaked.
	"uniform float sunIntensity = 1;\n",
	"uniform float ambient = 0.2;\n",
	"in vec2 fragmentTexCoord;\n",
	"in vec3 fragNormal;\n",
	"in vec3 position;\n",       // The model coordinate, as given by the vertex shader
	"out vec4 diffuseOutput;\n", // layout(location = 0)
	"out vec4 posOutput;\n",     // layout(location = 1)
	"out vec4 normOutput;\n",    // layout(location = 2)
	"out vec4 blendOutput;\n",   // layout(location = 3)
	"void main(void)\n",
	"{\n",
	"	diffuseOutput = texture(firstTexture, fragmentTexCoord) + colorAddon;\n",
	"   posOutput = vec4(position,sunIntensity);\n", // Set the sun flag in the alpha channel
	"   normOutput = vec4(fragNormal, ambient);\n", // Add some ambient in alpha channel. No normalize() needed as long as all surfaces are flat.
	// "	gl_FragColor = vec4(fragNormal, 1);\n", // Used for debugging texture coordinates.
	"}\n",
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
		fgSingleton.Init("MonsterShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
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
