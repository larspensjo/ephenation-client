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
#include "AnimationShader.h"
#include "../primitives.h"
#include "../uniformbuffer.h"
#include "../shadowconfig.h"

static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	UNIFORMBUFFER
	DOUBLERESOLUTIONFUNCTION
	"const float VERTEXSCALING="  STR(VERTEXSCALING) ";" // The is the scaling factor used for vertices
	"const float NORMALSCALING="  STR(NORMALSCALING) ";" // The is the scaling factor used for vertices
	"const float TEXTURESCALING="  STR(TEXTURESCALING) ";" // The is the scaling factor used for vertices
	"uniform mat4 modelMatrix;\n",
	"uniform mat4 bonesMatrix[64];"
	"uniform bool forShadowmap = false;"
	"uniform mat4 shadowProjViewMat;"
	"in vec4 normal;\n",
	"in vec4 vertex;\n", // First 3 are vertex coordinates, the 4:th is texture data coded as two scaled bytes
	"in vec3 weights;\n",
	"in vec3 joints;\n",
	"out vec3 fragmentNormal;\n",
	"out vec2 fragmentTexCoord;\n",
	"out float extIntensity;\n",
	"out float extAmbientLight;\n",
	"out vec3 position;\n",
	"void main(void)\n",
	"{\n",
	"	vec4 vertexScaled = vec4(vec3(vertex) / VERTEXSCALING, 1);"
	"	int t1 = int(vertex[3]);" // Extract the texture data
	"	vec2 tex = vec2(t1&0xFF, t1>>8);"
	"	vec2 textureScaled = tex / TEXTURESCALING;"
	"	vec3 normalScaled = normal.xyz / NORMALSCALING;"
	"	int intens2 = int(normal[3]);" // Bit 0 to 3 is sun intensity, 4 to 7 is ambient light
	"	if (intens2 < 0) intens2 += 256;"
	"	mat4 animationMatrix = weights[0] * bonesMatrix[int(joints[0])] + weights[1] * bonesMatrix[int(joints[1])] + weights[2] * bonesMatrix[int(joints[2])];",
	"	mat4 newModel = modelMatrix * animationMatrix;",
	"	vec4 pos;\n",
	"	if (forShadowmap) {"
	"		pos = shadowProjViewMat * newModel * vertexScaled;\n",
	"		pos.xy = DoubleResolution(pos.xy);"
	"	} else {"
	"		pos = UBOProjectionviewMatrix * newModel * vertexScaled;\n",
	// Scale the intensity from [0..255] to [0..1].
	"		fragmentTexCoord = textureScaled;\n",
	"		extIntensity = (intens2 & 0x0F)/15.0;\n",
	"		extAmbientLight = (intens2 >> 4)/15.0;\n",
	"		fragmentNormal = normalize((newModel*vec4(normalScaled, 0.0)).xyz);\n",
	"	}"
	"	position = vec3(newModel * vertexScaled);\n", // Copy position to the fragment shader
	"	gl_Position = pos;\n",
	"}\n",
};

static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	UNIFORMBUFFER
	"uniform sampler2D firstTexture;\n",
	"uniform bool forShadowmap = false;"
	"in vec3 fragmentNormal;\n",
	"in vec2 fragmentTexCoord;\n",
	"in float extIntensity;\n",
	"in float extAmbientLight;\n",
	"in vec3 position;\n",       // The model coordinate, as given by the vertex shader
	"out vec4 diffuseOutput;\n", // layout(location = 0)
	"out vec4 posOutput;\n",     // layout(location = 1)
	"out vec4 normOutput;\n",    // layout(location = 2)
	"void main(void)\n",
	"{\n",
	"   if (distance(UBOCamera.xyz, position) > UBOViewingDistance) { discard; return; }\n",
	"	vec4 clr = texture(firstTexture, fract(fragmentTexCoord));\n",
	"   float alpha = clr.a;\n",
	"   if (alpha == 0) { discard; return; }\n",
	"   if (forShadowmap) return;"		// Ignore the rest, only depth buffer is used anyway.
	"   posOutput.xyz = position;\n",   // This position shall not be transformed by the view matrix
	"   posOutput.a = extIntensity;\n", // Use the alpha channel for sun intensity
	"   normOutput = vec4(fragmentNormal, extAmbientLight);\n", // Use alpha channel of normal for ambient info.
	"   diffuseOutput = clr;\n",
	"}\n",
};

AnimationShader *AnimationShader::Make(void) {
	if (fgSingleton.fModelMatrixIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Init("AnimationShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void AnimationShader::PreLinkCallback(GLuint prg) {
	glBindFragDataLocation(prg, 0, "diffuseOutput");
	glBindFragDataLocation(prg, 1, "posOutput");
	glBindFragDataLocation(prg, 2, "normOutput");

	// Ensure that the same index for inputs are always used (to enable the use of the same VAO on other shaders).
	glBindAttribLocation(prg, StageOneShader::Normal, "normal");
	glBindAttribLocation(prg, StageOneShader::Vertex, "vertex");
	glBindAttribLocation(prg, StageOneShader::SkinWeights, "weights");
	glBindAttribLocation(prg, StageOneShader::Joints, "joints");
}

void AnimationShader::GetLocations(void) {
	fModelMatrixIndex = this->GetUniformLocation("modelMatrix");
	fFirstTextureIndex = this->GetUniformLocation("firstTexture");
	fBonesMatrix = this->GetUniformLocation("bonesMatrix");
	fForShadowmap = this->GetUniformLocation("forShadowmap");
	fShadowProjView = this->GetUniformLocation("shadowProjViewMat");
	checkError("AnimationShader::GetLocations");
}

void AnimationShader::Model(const glm::mat4 &mat) {
	glUniformMatrix4fv(fModelMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void AnimationShader::FirstTexture(int ind) {
	glUniform1i(fFirstTextureIndex, 0);
}

void AnimationShader::Bones(const glm::mat4 *p, int num) {
	glUniformMatrix4fv(fBonesMatrix, num, GL_FALSE, &((*p)[0][0]));
}

AnimationShader::AnimationShader() {
	fModelMatrixIndex = -1;
	fFirstTextureIndex = -1;
	fBonesMatrix = -1;
	fForShadowmap = -1;
	fShadowProjView = -1;
}

AnimationShader::~AnimationShader() {
}

void AnimationShader::Shadowmap(bool enable, const glm::mat4 &projectionview) {
	glUniform1i(fForShadowmap, enable);
	glUniformMatrix4fv(fShadowProjView, 1, GL_FALSE, &projectionview[0][0]);
}

AnimationShader AnimationShader::fgSingleton;
