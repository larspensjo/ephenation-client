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
#include <stdio.h>
#include <stdlib.h>

#include <glm/glm.hpp>
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
	"uniform vec4 Upoint;\n",            // A light. .xyz is the coordinate, and .w is the strength (radius)
	"layout (location = 0) in vec2 vertex;\n",
	"out vec2 screen;\n",             // Screen coordinate
	"void main(void)\n",
	"{\n",
	"	float radius = Upoint.w;" // Interpreted as the radius of the shadow
	// Relative bounding (2D) box around the point light
	"	vec3 box =  vec3(vertex*2-1, 0)*radius;"
	"	vec4 viewPos = UBOViewMatrix * vec4(Upoint.xyz, 1);"
	"	vec3 d = normalize(viewPos.xyz);"
	// We want to move the quad towards the player. It shall be moved so as
	// precisely be outside the range of the lamp. This is needed as the depth
	// culling will remove parts of the quad that are hidden.
	"	float l = min(radius, -viewPos.z-1);"
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
	"uniform vec4 Upoint;\n",          // A point source. .xyz is the coordinate, and .w is the radius of the shadow
	"uniform int Umode = 0;"           // Special case, use a selection color instead of a shadow
	"in vec2 screen;\n",               // The screen position
	"layout (location = 0) out vec4 colorChange;\n",

	"void main(void)\n",
	"{\n",
	"	vec4 normal = texture(normalTex, screen);\n",
	"	if (normal.xyz == vec3(0,0,0)) { discard; return;}\n",   // No normal, which means sky
	"	vec4 worldPos = texture(posTex, screen);\n",
	"	float dist = distance(worldPos.xyz, Upoint.xyz);\n",     // Distance to the player or monster
	"	if (worldPos.y - Upoint.y > 0.3 && Umode < 2) { discard; return; }"   // Don't draw shadow too high on the object itself.
	"	float radius = Upoint.w;\n",                             // The maximum distance (radius of the shadow) is coded in the w channel
	"	if (dist >= radius) {discard; return; }",                // Is the pixel near, below the player/monster?
	"	float f;"
	"	switch(Umode) {"
	"	case 4:"
	"		f = (1 - dist/radius)/2;\n",                         // 'f' goes from 0.09 to 1.0, depending on distance.
	"		colorChange = vec4(0, 0, 1, f);"                     // Will be used as a multiplicative component, making center almost black.
	"		break;"
	"	case 3:"
	"		f = (1 - dist/radius)/2;\n",                         // 'f' goes from 0.09 to 1.0, depending on distance.
	"		colorChange = vec4(0, 1, 0, f);"                     // Will be used as a multiplicative component, making center almost black.
	"		break;"
	"	case 2:"
	"		f = (1 - dist/radius)/2;\n",                         // 'f' goes from 0.09 to 1.0, depending on distance.
	"		colorChange = vec4(1, 0, 0, f);"                     // Will be used as a multiplicative component, making center almost black.
	"		break;"
	"	case 1:"
	"		colorChange = vec4(1, 0, 0, 0.3);"                   // Shall be used as a blending component, adding a red marker at the feet of a monster.
	"		break;"
	"	case 0:"
	"		f = 1 - (1 - dist/radius)/1.1;\n",                   // 'f' goes from 0.09 to 1.0, depending on distance.
	"		colorChange = vec4(f, f, f, 1);"                     // Will be used as a multiplicative component, making center almost black.
	"		break;"
	"	}"
	"}\n",
};

AddPointShadow::AddPointShadow() {
	fPointIdx = -1; fSelectionIdx = -1;
	fPreviousMode = 0;
}

void AddPointShadow::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Init("DeferredLigting", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
}

void AddPointShadow::GetLocations(void) {
	fPointIdx = this->GetUniformLocation("Upoint");
	fSelectionIdx = this->GetUniformLocation("Umode");

	glUniform1i(this->GetUniformLocation("normalTex"), 2);
	glUniform1i(this->GetUniformLocation("posTex"), 1);
}

void AddPointShadow::DrawBlueLight(const glm::vec4 &pos) {
	glUseProgram(this->Program());
	if (fPreviousMode != 4) {
		// Remember which was the previous state, to minimize updates
		fPreviousMode = 4;
		glUniform1i(fSelectionIdx, fPreviousMode);
	}
	glUniform4fv(fPointIdx, 1, &pos.x);
	gQuad.Draw();
	glUseProgram(0);
}

void AddPointShadow::DrawGreenLight(const glm::vec4 &pos) {
	glUseProgram(this->Program());
	if (fPreviousMode != 3) {
		// Remember which was the previous state, to minimize updates
		fPreviousMode = 3;
		glUniform1i(fSelectionIdx, fPreviousMode);
	}
	glUniform4fv(fPointIdx, 1, &pos.x);
	gQuad.Draw();
	glUseProgram(0);
}

void AddPointShadow::DrawRedLight(const glm::vec4 &pos) {
	glUseProgram(this->Program());
	if (fPreviousMode != 2) {
		// Remember which was the previous state, to minimize updates
		fPreviousMode = 2;
		glUniform1i(fSelectionIdx, fPreviousMode);
	}
	glUniform4fv(fPointIdx, 1, &pos.x);
	gQuad.Draw();
	glUseProgram(0);
}

void AddPointShadow::DrawMonsterSelection(const glm::vec4 &pos) {
	glUseProgram(this->Program());
	if (fPreviousMode != 1) {
		// Remember which was the previous state, to minimize updates
		fPreviousMode = 1;
		glUniform1i(fSelectionIdx, fPreviousMode);
	}
	glUniform4fv(fPointIdx, 1, &pos.x);
	gQuad.Draw();
	glUseProgram(0);
}

void AddPointShadow::DrawPointShadow(const glm::vec4 &pos) {
	glUseProgram(this->Program());
	if (fPreviousMode != 0) {
		// Remember which was the previous state, to minimize updates
		fPreviousMode = 0;
		glUniform1i(fSelectionIdx, fPreviousMode);
	}
	glUniform4fv(fPointIdx, 1, &pos.x);
	gQuad.Draw();
	glUseProgram(0);
}
