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
#include <stdlib.h>

#include <glm/glm.hpp>
#include "../primitives.h"
#include "adddynamicshadow.h"
#include "../ui/Error.h"
#include "../Options.h"
#include "../uniformbuffer.h"
#include "../shadowconfig.h"
#include "../shapes/quad.h"

// This vertex shader will only draw two triangles, giving a full screen.
// The vertex input is 0,0 in one corner and 1,1 in the other.
static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"layout(location = 0) in vec2 vertex;\n",
	"out vec2 screen;\n",             // Screen coordinate
	"void main(void)\n",
	"{\n",
	"	gl_Position = vec4(vertex*2-1, 0, 1);\n", // Transform from interval 0 to 1, to interval -1 to 1.
	"   screen = vertex;\n",  // Copy position to the fragment shader. Only x and y is needed.
	"}\n",
};

static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	UNIFORMBUFFER
	DOUBLERESOLUTIONFUNCTION
	RANDOMVEC2POISSON
	"uniform sampler2D posTex;\n",     // World position
	"uniform sampler2D normalTex;\n",  // Normals
	"in vec2 screen;\n",               // The screen position
	"layout(location = 0) out float light;\n",

	"vec4 worldPos;\n",
	"vec4 normal;\n",

// Declarations used for sun and shadow
	"uniform mat4 shadowmat;\n"       // combination of projection * view matrices of the light source
	"uniform sampler2D shadowmapTex;\n" // TODO: Use shadow sampler instead
	"const float shadowMapSize1 = " STR(DYNAMIC_SHADOW_MAP_SIZE) ";\n" // The size of the dynamic shadowmap bitmap
	"const float shadowMapSize2 = " STR(STATIC_SHADOW_MAP_SIZE) ";\n" // The size of the dynamic shadowmap bitmap
	"const vec3 sundir = vec3(-0.577350269, 0.577350269, 0.577350269);\n",
	"const int shadowMultiSample = 5;"

	// This function computes lighting from a low resolution shadow map. The purpose is to use it for
	// low performance systems
	"float ShadowMapLinear(vec3 pos, vec3 normal) {\n"
	"   vec4 shadowmapcoord = shadowmat * vec4(pos.xyz, 1);\n"
	"	shadowmapcoord.xy = DoubleResolution(shadowmapcoord.xy);"
	// Scale x and y from -1..1 to 0..1 so that they can be used to lookup a bitmap.
	// z is also scaled, as OpenGL returns the interval 0..1 from the depth buffer.
	"   shadowmapcoord.xyz = shadowmapcoord.xyz/2 + 0.5;\n"
	"   float sun = 1.0;\n"
	"   if (shadowmapcoord.x > 0 && shadowmapcoord.x < 1 && shadowmapcoord.y > 0 && shadowmapcoord.y < 1 && shadowmapcoord.z < 1) {\n"
	"       const float p = 1.0/shadowMapSize2;\n" // Size of one pixel
	// Add a small delta or there is a chance objects will shadow themselves.
	"		float cosTheta = clamp(dot(normal, sundir), 0.1, 1);"
	"       float d = clamp(0.002/sqrt(cosTheta), 0.0, 0.01);\n"
	// "		int start = int(fract(dot(pos, vec3(23.534, 65.91281, 31.231)))*32)%32;"
	"		for (int i=0;i<shadowMultiSample;i++) {"
	"			vec2 ind = shadowmapcoord.xy + rand2(screen.xy)*p;"
	"			float depth = texture(shadowmapTex, ind).x;\n"
	"			if (shadowmapcoord.z > depth + d) sun -= 1.0/shadowMultiSample;\n"
	"		}"
	"   }\n"
	"   return sun;\n"
	"}\n"

	// This function computes lighting from a high resolution shadow map. The purpose is to use it for
	// high performance systems
	"float ShadowMap(vec3 pos, vec3 normal) {\n"
	"   vec4 shadowmapcoord = shadowmat * vec4(pos.xyz, 1);\n"
	"	shadowmapcoord.xy = DoubleResolution(shadowmapcoord.xy);"
	// Scale x and y from -1..1 to 0..1 so that they can be used to lookup a bitmap.
	// z is also scaled, as OpenGL returns the interval 0..1 from the depth buffer.
	"   shadowmapcoord.xyz = shadowmapcoord.xyz/2 + 0.5;\n"
	"   float sun = 1.0;\n"
	"   if (shadowmapcoord.x > 0 && shadowmapcoord.x < 1 && shadowmapcoord.y > 0 && shadowmapcoord.y < 1 && shadowmapcoord.z < 1) {\n"
	"       const float p = 1.0/shadowMapSize1;\n" // Size of one pixel
	// Add a small delta or there is a chance objects will shadow themselves.
	"		float cosTheta = clamp(dot(normal, sundir), 0.1, 1);"
	"       float d = clamp(0.002/sqrt(cosTheta), 0.0, 0.01);\n"
	"		for (int i=0;i<shadowMultiSample;i++) {"
	"			vec2 ind = shadowmapcoord.xy + rand2(screen.xy)*p*4;"
	"			float depth = texture(shadowmapTex, ind).x;\n"
	"			if (shadowmapcoord.z > depth + d) sun -= 1.0/shadowMultiSample;\n"
	"		}"
	"   }\n"
	"   return sun;\n"
	"}\n"

	"void main(void)\n",
	"{\n",
	// Load data, stored in textures, from the first stage rendering.
	"   normal = texture(normalTex, screen);\n",
	"   worldPos = texture(posTex, screen);\n",
	// "   if (normal.xyz == vec3(0,0,0)) skyPixel = true;\n", // No normal, which means sky. Not needed because of stencil mask
	// Temporary helper data
	"   float sun = max(dot(normal.xyz,sundir),0);\n",
	"   float inSun = worldPos.a;\n", // Is greater than 0 if this position is reached by the sun
	"	if (inSun > 0 && UBODynamicshadows == 1) inSun = ShadowMap(worldPos.xyz, normal.xyz);\n" // Override with dynamic shadows
	"	if (inSun > 0 && UBODynamicshadows == 2) inSun = ShadowMapLinear(worldPos.xyz, normal.xyz);\n" // Override with dynamic shadows
	// As the last step, combine all the diffuse color with the lighting and blending effects
	"   light = inSun*sun*1.5;\n",
	// "	light = rand2(screen.xy).r;" // Test the random number generator
	// "	light = worldPos.a;\n",
	"}\n",
};

AddDynamicShadow::AddDynamicShadow() {
	fShadowMapMatrixIdx = -1;
}

void AddDynamicShadow::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Init("AddDynamicShadow", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	checkError("AddDynamicShadow::Init");
}

void AddDynamicShadow::GetLocations(void) {
	fShadowMapMatrixIdx = this->GetUniformLocation("shadowmat");

	// The followig uniforms only need to be initialized once
	glUniform1i(this->GetUniformLocation("shadowmapTex"), 4); // The shadow map has to use GL_TEXTURE4
	glUniform1i(this->GetUniformLocation("normalTex"), 2);
	glUniform1i(this->GetUniformLocation("posTex"), 1);
	glUniform1i(this->GetUniformLocation(RANDOMVEC2POISSON_SAMPLERNAME), 0);

	checkError("AddDynamicShadow::GetLocations");
}

void AddDynamicShadow::Draw(const glm::mat4 &mat) {
	glUseProgram(this->Program());
	glUniformMatrix4fv(fShadowMapMatrixIdx, 1, GL_FALSE, &mat[0][0]);
	gQuad.Draw();
	glUseProgram(0);
}
