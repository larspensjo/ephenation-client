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
#include "addpointshadow.h"
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
	"uniform vec4 Upoint;\n",          // A point source. .xyz is the coordinate, and .w is the strength of the shadow
	"uniform bool Uselection = false;" // Special case, use a selection color instead of a shadow
	"in vec2 screen;\n",               // The screen position
	"layout (location = 0) out vec4 colorChange;\n",

	"void main(void)\n",
	"{\n",
	"	vec4 normal = texture(normalTex, screen);\n",
	"	if (normal.xyz == vec3(0,0,0)) { discard; return;}\n",   // No normal, which means sky
	"	vec4 worldPos = texture(posTex, screen);\n",
	"	float dist = distance(worldPos.xyz, Upoint.xyz);\n",     // Distance to the player or monster
	"	if (worldPos.y - Upoint.y > 0.3) { discard; return; }"   // Don't draw shadow too high on the object itself.
	"	float lim = Upoint.w;\n",                                // The maximum distance is coded in the w channel
	"	if (dist >= lim) {discard; return; }",                   // Is the pixel near, below the player/monster?
	"	if (Uselection) {"
	"		colorChange = vec4(1, 0, 0, 0.3);"                   // Shall be used as a blending component
	"	} else {"
	"		float f = 1 - (1 - dist/lim)/1.1;\n",
	"		colorChange = vec4(f, f, f, 1);"                     // Shall be used as a multiplicative component
	"	}"
	"}\n",
};

AddPointShadow::AddPointShadow() {
	fPointIdx = -1; fSelectionIdx = -1;
	fPreviousForSelection = false;
}

void AddPointShadow::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Init("DeferredLigting", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	checkError("AddPointShadow::Init");
}

void AddPointShadow::GetLocations(void) {
	fPointIdx = this->GetUniformLocation("Upoint");
	fSelectionIdx = this->GetUniformLocation("Uselection");

	glUniform1i(this->GetUniformLocation("normalTex"), 2);
	glUniform1i(this->GetUniformLocation("posTex"), 1);

	checkError("AddPointShadow::GetLocations");
}

void AddPointShadow::Draw(const glm::vec4 &pos, bool forSelection) {
	glUseProgram(this->Program());
	if (forSelection != fPreviousForSelection) {
		// Remember which was the previous state, to minimize updates
		fPreviousForSelection = forSelection;
		glUniform1i(fSelectionIdx, forSelection);
	}
	glUniform4fv(fPointIdx, 1, &pos.x);
	gQuad.Draw();
	glUseProgram(0);
}
