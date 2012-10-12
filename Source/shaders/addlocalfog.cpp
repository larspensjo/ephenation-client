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
#include "addlocalfog.h"
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
	// Relative bounding (2D) box in front of the fog
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
	UNIFORMBUFFER
	"uniform sampler2D posTex;\n",     // World position
	"uniform vec4 Upoint;\n",          // A point source. .xyz is the coordinate, and .w is the strength of the shadow
	"uniform float Uambient;"          // Approximate amount of ambient light at around the fog.
	"in vec2 screen;\n",               // The screen position
	"layout (location = 0) out vec4 blendColor;\n",

	"void main(void)\n",
	"{\n",
	// "	blendColor = vec4(1, 0, 0, 0.5);\n",
	// "	return;"
	"	vec4 worldPos = texture(posTex, screen);\n",
	"	vec3 cameraToWorld = UBOCamera.xyz-worldPos.xyz;\n",
	"	float vertexDistance = length(cameraToWorld);\n",
	// Add some small random delta to the radius
	"	float delta = (fract(screen.x*812219.65 + screen.y*242328.123)+fract(vertexDistance*3122.323))*0.3;"
	"	float dist = 0;\n",             // This is the length of the ray that is inside the fog.
	"	float radius = Upoint.w + delta;\n",                               // The maximum distance is coded in the w channel
	"	float cameraToFogDist = distance(UBOCamera.xyz, Upoint.xyz);\n",
	"	if (vertexDistance + radius < cameraToFogDist) { discard; return; }\n", // Quick test if fog is too far away.
	"	float pixelToFogDist = distance(worldPos.xyz, Upoint.xyz);\n",
	"	if (cameraToFogDist < radius && pixelToFogDist < radius) {\n",
	"		dist = vertexDistance;\n", // Simple case, camera and pixel completely inside fog
	"	} else {\n",
	"		vec3 l = normalize(worldPos.xyz-UBOCamera.xyz);\n",
	"		float ldotc = dot(l,Upoint.xyz-UBOCamera.xyz);\n",
	"		float tmp = ldotc*ldotc - cameraToFogDist*cameraToFogDist + radius*radius;\n",
	"		if (cameraToFogDist > radius && pixelToFogDist > radius && ldotc > 0 && tmp > 0) {\n",
	"			float sqrttmp = sqrt(tmp);\n",
	"			vec3 entrance = UBOCamera.xyz + l*(ldotc-sqrttmp);\n",
	"			if (vertexDistance > distance(UBOCamera.xyz, entrance)) dist = sqrttmp*2;\n",
	"		} else if (cameraToFogDist > radius && pixelToFogDist < radius) {\n",
	"			vec3 entrance = UBOCamera.xyz + l*(ldotc-sqrt(tmp));\n", // Outside of fog, looking at pixel inside
	"			dist = distance(entrance, worldPos.xyz);\n",
	"		} else if (cameraToFogDist < radius && pixelToFogDist > radius) {\n",
	"			vec3 exit = UBOCamera.xyz + l*(ldotc+sqrt(tmp));\n", // Inside of fog, looking at pixel outside
	"			dist = distance(exit, UBOCamera.xyz);\n",
	"		}\n",
	"	}\n",
	"	float sqr = dist*dist/radius/radius;"
	"	float alpha = 0.25 * sqr;\n",
	// Uambient is a function 0-1 of how much sky is visible (indirect light).
	// UBOambientLight is a global offset, so as to never have a place that is completely dark.
	"	float intensity = Uambient*0.6f + UBOambientLight*0.7;"
	"	blendColor = vec4(intensity, intensity, intensity, alpha);\n",
	"}\n",
};

AddLocalFog::AddLocalFog() {
	fPointIdx = -1; fAmbientIdx = -1;
}

void AddLocalFog::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Init("DeferredLigting", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	checkError("AddLocalFog::Init");
}

void AddLocalFog::GetLocations(void) {
	fPointIdx = this->GetUniformLocation("Upoint");
	fAmbientIdx = this->GetUniformLocation("Uambient");

	glUniform1i(this->GetUniformLocation("posTex"), 1);

	checkError("AddLocalFog::GetLocations");
}

void AddLocalFog::Draw(const glm::vec4 &pos, float ambient) {
	glUseProgram(this->Program());
	glUniform4fv(fPointIdx, 1, &pos.x);
	glUniform1f(fAmbientIdx, ambient);
	gQuad.Draw();
	glUseProgram(0);
}
