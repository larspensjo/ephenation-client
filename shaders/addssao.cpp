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
#include "addssao.h"
#include "../ui/Error.h"
#include "../shadowconfig.h"
#include "../shapes/quad.h"
#include "../uniformbuffer.h"

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
	"uniform sampler2D posTex;\n",     // World position
	"uniform sampler2D normalTex;\n",  // Normals
	"in vec2 screen;\n",               // The screen position
	"layout(location = 0) out float light;\n",

	"vec4 worldPos;\n",
	"vec4 normal;\n",

	"vec2 seed;"

	"vec2 rand(vec2 a, vec2 b) {"
	"	seed = fract(a*10.23 + b*123.1232+screen*3.123 + seed*82.12354);" // A value from 0 to 1
	"	return seed;"
	"}"

	"void main(void)\n",
	"{\n",
	"	normal = texture(normalTex, screen);\n",
	"	worldPos = texture(posTex, screen);\n",
	"	float ref = distance(UBOCamera.xyz, worldPos.xyz);"
	"	int num = 0;"
	"	const int SIZE=20;"
	"	float p = 1.0/UBOWindowHeight;\n" // Size of one pixel
	"	for (int i=0; i<SIZE; i++) {"
	"		int ind = i + abs(int(worldPos.x*12.32));"
	"		vec2 sampleInd = screen + (rand(worldPos.xy, normal.xy)*2-1)*p*20;"
	"		vec3 sample = texture(posTex, sampleInd).xyz;\n"
	"		float dist = distance(UBOCamera.xyz, sample);"
	"		if (abs(dist-ref) > 0.2) { num-=10; }"
	"		if (dist < ref) num++;\n"
	"	}"
	"	if (num > SIZE*0.8)"
	// As the last step, combine all the diffuse color with the lighting and blending effects
	"		light = -0.1;\n"
	"	else {"
	"		discard; return;"
	"	}"
	// "   light = worldPos.a;\n",
	"}\n",
};

AddSSAO::AddSSAO() {
}

void AddSSAO::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Init("AddSSAO", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
	checkError("AddSSAO::Init");
}

void AddSSAO::GetLocations(void) {
	// The followig uniforms only need to be initialized once
	glUniform1i(this->GetUniformLocation("normalTex"), 2);
	glUniform1i(this->GetUniformLocation("posTex"), 1);

	checkError("AddSSAO::GetLocations");
}

void AddSSAO::Draw(void) {
	glUseProgram(this->Program());
	gQuad.Draw();
	glUseProgram(0);
}
