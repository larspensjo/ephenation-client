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

#include "../glm/glm.hpp"
#include "../primitives.h"
#include "addpointlight.h"
#include "../ui/Error.h"
#include "../uniformbuffer.h"
#include "../shapes/quad.h"

// This vertex shader will only draw two triangles, limited to the part of the screen
// that can be affected.
// The vertex input is 0,0 in one corner and 1,1 in the other.
static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	UNIFORMBUFFER
	"uniform vec4 Upoint;\n",            // A light. .xyz is the coordinate, and .w is the strength
	"layout (location = 0) in vec2 vertex;\n",
	"out vec2 screen;\n",             // Screen coordinate
	"void main(void)\n",
	"{\n",
	"	float strength = Upoint.w;"
	// Relative bounding (2D) box around the point light
	"	vec3 box =  vec3(vertex*2-1, 0)*strength;"
	"	vec4 viewPos = UBOViewMatrix * vec4(Upoint.xyz, 1);"
	"	vec3 d = normalize(viewPos.xyz);"
	// We want to move the quad towards the player. It shall be moved so as
	// precisely be outside the range of the lamp. This is needed as the depth
	// culling will remove parts of the quad that are hidden.
	"	float l = min(strength, -viewPos.z-1);"
	// The modelView is one of the corners of the quad in view space.
	"	vec4 modelView = -vec4(d, 0)*l + vec4(box, 0) + viewPos;"
	"	vec4 pos = UBOProjectionMatrix * modelView;"
	"	pos /= pos.w;"
	"	gl_Position = pos;\n",
	// Copy position to the fragment shader. Only x and y is needed. Scale it
	// from interval -1 .. 1, to the interval 0 .. 1.
	"	screen = pos.xy/2+0.5;\n",
	"}\n",
};

static const GLchar *fragmentShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"uniform sampler2D posTex;\n",     // World position
	"uniform sampler2D normalTex;\n",  // Normals
	"uniform vec4 Upoint;\n",            // A light. .xyz is the coordinate, and .w is the strength of the lamp
	"in vec2 screen;\n",               // The screen position
	"layout (location = 0) out float addLighting;\n",

	"void main(void)\n",
	"{\n",
	// Load data, stored in textures, from the first stage rendering.
	// "   addLighting = 0.5;return;\n",
	"   vec4 normal = texture(normalTex, screen);\n",
	// "   if (normal.xyz == vec3(0,0,0)) { discard; return;}\n", // No normal, which means sky. Test not needed because of stencil mask
	"   vec4 worldPos = texture(posTex, screen);\n",
	"	float dist = distance(worldPos.xyz, Upoint.xyz);\n", // Distance to the lamp
	"	float strength = Upoint.w;\n",
	"	if (dist >= strength) { discard; return; } ",
	"	vec3 lampVector = normalize(worldPos.xyz - Upoint.xyz);\n",
	"	float mult = clamp(-dot(lampVector, normal.xyz), 0, 1);\n",
	"	addLighting = 1.5*(1 - dist/strength)*mult;\n",
	"}\n",
};
AddPointLight::AddPointLight() {
	fLampIdx = -1;
}

void AddPointLight::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Init("DeferredLigting", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	checkError("AddPointLight::Init");
}

void AddPointLight::GetLocations(void) {
	fLampIdx = this->GetUniformLocation("Upoint");

	glUniform1i(this->GetUniformLocation("normalTex"), 2);
	glUniform1i(this->GetUniformLocation("posTex"), 1);

	checkError("AddPointLight::GetLocations");
}

void AddPointLight::Draw(const glm::vec3 &pos, float strength) {
	glUseProgram(this->Program());
	const glm::vec4 point(pos.x, pos.y, pos.z, strength);
	glUniform4fv(fLampIdx, 1, &point.x);
	gQuad.Draw();
	glUseProgram(0);
}
