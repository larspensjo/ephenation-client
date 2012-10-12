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
#include "ChunkShader.h"
#include "../primitives.h"
#include "../uniformbuffer.h"
#include "../ui/Error.h"

static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	UNIFORMBUFFER
	"const float VERTEXSCALING="  STR(VERTEXSCALING) ";" // The is the scaling factor used for vertices
	"const float NORMALSCALING="  STR(NORMALSCALING) ";" // The is the scaling factor used for vertices
	"const float TEXTURESCALING="  STR(TEXTURESCALING) ";" // The is the scaling factor used for vertices
	"uniform mat4 modelMatrix;\n",
	// This contains both a offset and a multiplier to be used for the bitmap. It enables the use
	// of atlas bitmaps.
	"uniform vec3 textOffsMulti = vec3(0,0,1);\n",
	"in vec4 normal;\n",
	"in vec4 vertex;\n", // First 3 are vertex coordinates, the 4:th is texture data coded as two scaled bytes
	"out vec3 fragmentNormal;\n",
	"out vec2 fragmentTexCoord;\n",
	"out float extIntensity;\n",
	"out float extAmbientLight;\n",
	"out vec3 position;\n",
	"void main(void)\n",
	"{\n",
	"	int t1 = int(vertex[3]);" // Extract the texture data
	"	vec2 tex = vec2(t1&0xFF, t1>>8);"
	"	vec4 vertexScaled = vec4(vec3(vertex) / VERTEXSCALING, 1);"
	"	vec2 textureScaled = tex / TEXTURESCALING;"
	"	int intens2 = int(normal[3]);" // Bit 0 to 3 is sun intensity, 4 to 7 is ambient light
	"	if (intens2 < 0) intens2 += 256;"
	"   float textMult = textOffsMulti.z;\n",
	"	fragmentTexCoord = textureScaled*textMult + textOffsMulti.xy;\n",
	"	fragmentNormal = normalize((modelMatrix*vec4(normal.xyz/NORMALSCALING, 0.0)).xyz);\n",
	"	gl_Position = UBOProjectionviewMatrix * modelMatrix * vertexScaled;\n",
	"   position = vec3(modelMatrix * vertexScaled);\n", // Copy position to the fragment shader
	// Scale the intensity from [0..255] to [0..1].
	"	extIntensity = (intens2 & 0x0F)/15.0;\n",
	// "	extIntensity = 0.1;\n",
	"   extAmbientLight = (intens2 >> 4)/15.0;\n",
	// "   extAmbientLight = 1;\n",
	"}\n",
};

static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	UNIFORMBUFFER
	"uniform sampler2D firstTexture;\n",
	"in vec3 fragmentNormal;\n",
	"in vec2 fragmentTexCoord;\n",
	"in float extIntensity;\n",
	"in float extAmbientLight;\n",
	"in vec3 position;\n",       // The model coordinate, as given by the vertex shader
	"layout(location = 0) out vec4 diffuseOutput;\n",
	"layout(location = 1) out vec4 posOutput;\n",
	"layout(location = 2) out vec4 normOutput;\n",
	"void main(void)\n",
	"{\n",
	"   if (distance(UBOCamera.xyz, position) > UBOViewingDistance) { discard; return; }\n",
	"	vec4 clr = texture(firstTexture, fract(fragmentTexCoord));\n",
	"   float alpha = clr.a;\n",
	"   if (alpha == 0) { discard; return; }\n", // For some reason, some alpha that should be discarded is still not exactly 0.
	"   posOutput.xyz = position;\n",   // This position shall not be transformed by the view matrix
	"   posOutput.a = extIntensity;\n", // Use the alpha channel for sun intensity. TODO: This is a 32-bit float, very inefficient
	"   normOutput = vec4(fragmentNormal, extAmbientLight);\n", // Use alpha channel of normal for ambient info.
	"   diffuseOutput = clr;\n",
	"}\n",
};

ChunkShader *ChunkShader::Make(void) {
	if (fgSingleton.fModelMatrixIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		fgSingleton.Init("ChunkShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
	return &fgSingleton;
}

void ChunkShader::PreLinkCallback(GLuint prg) {
	// Ensure that the same index for inputs are always used (to enable the use of the same VAO on other shaders).
	glBindAttribLocation(prg, StageOneShader::Normal, "normal");
	glBindAttribLocation(prg, StageOneShader::Vertex, "vertex");
}

void ChunkShader::GetLocations(void) {
	fModelMatrixIndex = this->GetUniformLocation("modelMatrix");
	fFirstTextureIndex = this->GetUniformLocation("firstTexture");
	fTextOffsMultiInd = this->GetUniformLocation("textOffsMulti");

	checkError("ChunkShader::GetLocations");
}

void ChunkShader::Model(const glm::mat4 &mat) {
	if (fModelMatrixIndex != -1)
		glUniformMatrix4fv(fModelMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void ChunkShader::FirstTexture(int ind) {
	if (fFirstTextureIndex != -1)
		glUniform1i(fFirstTextureIndex, 0);
}

void ChunkShader::TextureOffsetMulti(float offsX, float offsY, float mult) {
	glUniform3f(fTextOffsMultiInd, offsX, offsY, mult);
}

ChunkShader::ChunkShader() {
	fModelMatrixIndex = -1;
	fFirstTextureIndex = -1;
	fTextOffsMultiInd = -1;
}

ChunkShader::~ChunkShader() {
	// TODO Auto-generated destructor stub
}

ChunkShader ChunkShader::fgSingleton;
