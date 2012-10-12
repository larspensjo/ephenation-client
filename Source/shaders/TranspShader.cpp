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
#include "TranspShader.h"
#include "../primitives.h"
#include "../uniformbuffer.h"

static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	UNIFORMBUFFER
	"const float VERTEXSCALING="  STR(VERTEXSCALING) ";" // The is the scaling factor used for vertices
	"const float TEXTURESCALING="  STR(TEXTURESCALING) ";" // The is the scaling factor used for vertices
	"uniform mat4 modelMatrix;\n",
	"in vec4 vertex;\n", // First 3 are vertex coordinates, the 4:th is texture data coded as two scaled bytes
	"out vec2 fragmentTexCoord;\n",
	"out vec3 position;\n",
	"void main(void)\n",
	"{\n",
	"	vec4 vertexScaled = vec4(vec3(vertex) / VERTEXSCALING, 1);"
	"	int t1 = int(vertex[3]);" // Extract the texture data
	"	vec2 tex = vec2(t1&0xFF, t1>>8);"
	"	vec2 textureScaled = tex / TEXTURESCALING;"
	"	fragmentTexCoord = textureScaled;\n",
	"	gl_Position = UBOProjectionviewMatrix * modelMatrix * vertexScaled;\n",
	"   position = vec3(modelMatrix * vertexScaled);\n", // Copy position to the fragment shader
	"}\n",
};

static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	UNIFORMBUFFER
	"uniform sampler2D firstTexture;\n",
	"uniform sampler2D posTexture;\n",
	"uniform bool depthDependingAlpha = false;\n",
	"uniform vec2 screenSize = vec2(1920, 1003);\n", // A default as a safety precaution
	"uniform float time = 0;\n" // Number of seconds since the game was started
	"in vec2 fragmentTexCoord;\n",
	"in vec3 position;\n",       // The model coordinate, as given by the vertex shader
	"out vec4 blendOutput;\n",   // layout(location = 0)
	"vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }\n"
	"vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }\n"
	"vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }\n"
	"float snoise(vec2 v) {\n"
	"  const vec4 C = vec4(0.211324865405187, 0.366025403784439, -0.577350269189626, 0.024390243902439);\n"
	"  vec2 i  = floor(v + dot(v, C.yy) );\n"
	"  vec2 x0 = v -   i + dot(i, C.xx);\n"
	"  vec2 i1; i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);\n"
	"  vec4 x12 = x0.xyxy + C.xxzz;\n"
	"  x12.xy -= i1;\n"
	"  i = mod289(i);\n"
	"  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 )) + i.x + vec3(0.0, i1.x, 1.0 ));\n"
	"  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);\n"
	"  m = m*m ; m = m*m ;\n"
	"  vec3 x = 2.0 * fract(p * C.www) - 1.0;\n"
	"  vec3 h = abs(x) - 0.5;\n"
	"  vec3 ox = floor(x + 0.5);\n"
	"  vec3 a0 = x - ox;\n"
	"  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );\n"
	"  vec3 g; g.x  = a0.x  * x0.x  + h.x  * x0.y; g.yz = a0.yz * x12.xz + h.yz * x12.yw;\n"
	"  return 130.0 * dot(m, g);\n"
	"}\n"
	"void main(void)\n",
	"{\n",
	"   vec2 screen = gl_FragCoord.xy / screenSize;\n",
	"   float distCamPos = distance(UBOCamera.xyz, position);\n"
	"   if (distCamPos > UBOViewingDistance) { discard; return; }\n",
	"   vec2 textCoord = fract(fragmentTexCoord);\n",
	"	vec4 clr = texture(firstTexture, textCoord);\n",
	"   vec3 backgroundPos = texture(posTexture, screen).xyz;\n",
	"   float alpha = clr.a;\n",
	"   if (depthDependingAlpha) {\n",
	"		float dist = distance(position, backgroundPos);\n", // TODO: Move this logic to the vertex shader?
	"		alpha = clamp(0.4+dist/50, 0, 1);\n",
	"		clr.rgb *= alpha;" // This texture isn't premultiplied
	"       if (distCamPos<100 && UBOPerformance > 1) {\n"
	"           float wave1 = snoise(vec2(position.x/3, position.z+time/2.5))/10;\n"
	"           float wave2 = snoise(vec2(position.x/3+1000, position.z+time/2.5))/10;\n"
	"           clr.rgb += (wave2*sin(time) + wave1*sin(time+3.14/2)) * (1-distCamPos/100);\n" // Let the water ripples fade out with distance
	"       }\n"
	"   }\n",
	"   blendOutput.rgb = clr.rgb;\n", // The colors are already premultiplied by alpha
	"   blendOutput.a = alpha;\n",
	"}\n",
};

void TranspShader::Init(void) {
	if (fModelMatrixIndex == -1) {
		const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
		const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
		ShaderBase::Init("TranspShader", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	}
}

void TranspShader::PreLinkCallback(GLuint prg) {
	glBindFragDataLocation(prg, 0, "blendOutput");

	// Ensure that the same index for inputs are always used (to enable the use of the same VAO on other shaders).
	glBindAttribLocation(prg, StageOneShader::Vertex, "vertex");
}

void TranspShader::GetLocations(void) {
	fModelMatrixIndex = this->GetUniformLocation("modelMatrix");
	fDepthDependingAlpha = this->GetUniformLocation("depthDependingAlpha");
	fScreenSizeIdx = this->GetUniformLocation("screenSize");
	fTimeIdx = this->GetUniformLocation("time");

	GLint firstTextureIndex = this->GetUniformLocation("firstTexture");
	GLint deptPositionIdx = this->GetUniformLocation("posTexture");
	glUniform1i(firstTextureIndex, 0);
	glUniform1i(deptPositionIdx, 1);
	checkError("TranspShader::GetLocations");
}

void TranspShader::Model(const glm::mat4 &mat) {
	glUniformMatrix4fv(fModelMatrixIndex, 1, GL_FALSE, &mat[0][0]); // Send our modelView matrix to the shader
}

void TranspShader::View(float time) {
	glUniform1f(fTimeIdx, time);
}

void TranspShader::Projection(float w, float h) {
	glUniform2f(fScreenSizeIdx, w, h);
}

void TranspShader::DrawingWater(bool flag) {
	glUniform1i(fDepthDependingAlpha, flag);
}

TranspShader::TranspShader() {
	fModelMatrixIndex = -1;
	fDepthDependingAlpha = -1;
	fScreenSizeIdx = -1;
	fTimeIdx = -1;
}

TranspShader::~TranspShader() {
	// TODO Auto-generated destructor stub
}

TranspShader gTranspShader;
