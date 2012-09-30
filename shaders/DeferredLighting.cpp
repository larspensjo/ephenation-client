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
#include "DeferredLighting.h"
#include "../ui/Error.h"
#include "../Options.h"
#include "../uniformbuffer.h"
#include "../shadowconfig.h"
#include "../shapes/quad.h"

// This vertex shader will only draw two triangles, giving a full screen.
// The vertex input is 0,0 in one corner and 1,1 in the other.
static const GLchar *vertexShaderSource[] = {
	"#version 330\n", // This corresponds to OpenGL 3.3
	"layout (location=0) in vec2 vertex;\n",
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
	"uniform sampler2D diffuseTex;\n", // The color information
	"uniform sampler2D posTex;\n",     // World position
	"uniform sampler2D normalTex;\n",  // Normals
	"uniform sampler2D blendTex;\n",   // A bitmap with colors to blend with, afterwars.
	"uniform sampler2D lightTex;\n",   // A bitmap with colors to blend with, afterwars.
	"uniform sampler1D poissondisk;"
	"uniform bool Udead;\n",            // True if the player is dead
	"uniform bool Uwater;\n",           // True when head is in water
	"uniform bool Uteleport;\n",        // Special mode when inside a teleport
	"uniform float UwhitePoint = 3.0;"
	"in vec2 screen;\n",               // The screen position
	"layout(location = 0) out vec3 fragColor;\n",

	"bool skyPixel = false;\n",
	"vec4 worldPos;\n",
	"vec4 normal;\n",

	"float linearToSRGB(float linear) {"
	"	if (linear <= 0.0031308) return linear * 12.92;"
	"	else return 1.055 * pow(linear, 1/2.4) - 0.055;"
	"}"


	"vec2 seed;"
	"vec2 rand(vec2 a, vec2 b) {"
	"	seed = fract(a*10.23 + b*123.1232+screen*3.123 + seed*82.12354);" // A value from 0 to 1
	"	return seed;"
	"}"

	"const vec3 sundir = vec3(-0.577350269, 0.577350269, 0.577350269);\n",
	"void main(void)\n",
	"{\n",
	// Load data, stored in textures, from the first stage rendering.
	"   normal = texture(normalTex, screen);\n",
	"   vec4 diffuse = texture(diffuseTex, screen) * 0.95;\n", // Downscale a little, 1.0 can't be mapped to HDR.
	"   vec4 blend = texture(blendTex, screen);\n",
	"   worldPos = texture(posTex, screen);\n",
	"   if (normal.xyz == vec3(0,0,0)) skyPixel = true;\n",             // No normal, which means sky
	"   float ambient = normal.a;\n", // The ambient light is in the alpha channel.
	"   vec4 hdr = diffuse/(1-diffuse);\n",
	// Temporary helper data
	"   vec3 cameraToWorld = UBOCamera.xyz-worldPos.xyz;\n",
	"   vec3 eyeDir = normalize(cameraToWorld);\n",
	"   vec3 vHalfVector = normalize(sundir.xyz+eyeDir);\n",
	"   float inSun = worldPos.a;\n", // Is 1 if this position is reached by the sun
	"	float fact = texture(lightTex, screen).r;"
	"	float num = 1;"
	// Do some multi sampling to blur the lighting. It is kind of a box filter, but with enhanced weights
	// dynamically controlled by the geometry.
	"	for (int i=0; i < 4; i++) {"
	"		const float filterpixels = 4;" // The size of the area to include in the filter.
	"		vec2 ind = screen + rand2(screen)/UBOWindowHeight*filterpixels;"
	"		vec3 pos2 = texture(posTex, ind).xyz;"
	"		vec3 normal2 = texture(normalTex, ind).xyz;"
	"		const float maxdist = 1;" // Ignore contributions when distance difference exceeds this value
	"		float dist = min(abs(distance(UBOCamera.xyz, pos2) - length(cameraToWorld)), maxdist);" // A value between 0 and maxdist.
	//		Use a weight that depends on the distance difference. Also, if there is a sharp corner, we don't want
	//		shadows to bleed around the corner. This this is prevented by using a test for normals.
	"		float weight = (maxdist-dist)*dot(normal.xyz, normal2);" // A value from 0 to 1
	"		fact += weight * texture(lightTex, ind).r;"
	"		num += weight;"
	"	}"
	"	fact = fact/num + (ambient+UBOambientLight)*0.03;"
	"	if (UBODynamicshadows == 0) fact += inSun;"         // Add pre computed light instead of using shadow map
	"   if (skyPixel) { fact = 0.8; }\n",
	"   vec3 step1 = fact*hdr.xyz + inSun*pow(max(dot(normal.xyz,vHalfVector),0.0), 100) * 0.1;\n",
	"	step1 *= UBOexposure;"

	// Apply Reinhard tone mapping, based on the luminance
	"	float Lwhite2 = UwhitePoint*UwhitePoint;"
	"	float L = 0.2126 * step1.r + 0.7152 * step1.g + 0.0722 * step1.b;"
	"	vec3 step2 = step1 * (1+L/Lwhite2)/(1+L);"

	"   fragColor = (1-blend.a)*step2 + blend.xyz;\n",     // manual blending, using premultiplied alpha.
	//	Add some post processing effects
	"	fragColor.x = linearToSRGB(fragColor.x);"             // Transform to non-linear space
	"	fragColor.y = linearToSRGB(fragColor.y);"             // Transform to non-linear space
	"	fragColor.z = linearToSRGB(fragColor.z);"             // Transform to non-linear space
	"   if (Uwater) {\n",
	//		Player in water, gradually decrease visibility with distance
	"       float a = clamp(0.3+length(cameraToWorld)/50, 0, 1);\n",
	"       if (skyPixel) a = 1;\n",
	"       fragColor = mix(fragColor, vec3(0, 0.1, 0.5), a);\n",
	"   }\n",
	"   if (Udead) {\n",
	//		Player is dead, draw everything in black and white.
	"       float averageColor = fragColor.r*0.3 + fragColor.g*0.6 + fragColor.b*0.1;\n",
	"       fragColor.r = averageColor;\n",
	"       fragColor.g = averageColor;\n",
	"       fragColor.b = averageColor;\n",
	"   }\n",
	"   if (Uteleport) {\n",
	//		Inside teleport, dim the light.
	"       fragColor.r = (fragColor.r)/4;\n",
	"       fragColor.g = (fragColor.g)/4;\n",
	"       fragColor.b = (fragColor.b)/4;\n",
	"   }\n",
	//"	fragColor.rgb = vHalfVector;\n",
	//"	fragColor = (normal+1)/2;\n",
	//"	fragColor = shadowmapcoord.z;\n",
	//"	fragColor = texture(shadowmapTex, screen).x;\n",
	//"	fragColor = dist/32 * vec4(1,1,1,1);\n",
	//"	fragColor = blend;\n",
	//"	fragColor = debug * vec3(1,1,1);\n",
	//"	fragColor = depth;\n",
	//"	fragColor = inSun * vec3(1,1,1);\n",
	//"	fragColor = (sun*inSun+ambient)*vec4(1,1,1,1);\n",
	//"	fragColor = vec3(1,1,1)*texture(lightTex, screen).r;"
	//"	fragColor = vec3(rand2(screen), 0);" // Test the random
	"}\n",
};

DeferredLighting::DeferredLighting() {
	fDeadIndex = -1;
	fHeadInWaterIdx = -1; fInsideTeleportIdx = -1;
	fAverageLuminanceIdx = -1;
}

void DeferredLighting::Init(void) {
	const GLsizei vertexShaderLines = sizeof(vertexShaderSource) / sizeof(GLchar*);
	const GLsizei fragmentShaderLines = sizeof(fragmentShaderSource) / sizeof(GLchar*);
	ShaderBase::Init("DeferredLighting", vertexShaderLines, vertexShaderSource, fragmentShaderLines, fragmentShaderSource);
}

void DeferredLighting::GetLocations(void) {
	fHeadInWaterIdx = this->GetUniformLocation("Uwater");
	fInsideTeleportIdx = this->GetUniformLocation("Uteleport");
	fDeadIndex = this->GetUniformLocation("Udead");
	fAverageLuminanceIdx = this->GetUniformLocation("UwhitePoint");

	// The following uniforms only need to be initialized once
	glUniform1i(this->GetUniformLocation(RANDOMVEC2POISSON_SAMPLERNAME), 6);
	glUniform1i(this->GetUniformLocation("lightTex"), 5);
	glUniform1i(this->GetUniformLocation("blendTex"), 3);
	glUniform1i(this->GetUniformLocation("normalTex"), 2);
	glUniform1i(this->GetUniformLocation("posTex"), 1);
	glUniform1i(this->GetUniformLocation("diffuseTex"), 0);

	checkError("DeferredLighting::GetLocations");
}

void DeferredLighting::SetWhitePoint(float white) {
	glUniform1f(fAverageLuminanceIdx, white);
}

void DeferredLighting::EnableProgram(void) {
	glUseProgram(this->Program());
}

void DeferredLighting::DisableProgram(void) {
	glUseProgram(0);
}

void DeferredLighting::PlayerDead(bool fl) {
	glUniform1i(fDeadIndex, fl);
}

void DeferredLighting::Draw() {
	glUseProgram(this->Program());
	gQuad.Draw();
	glUseProgram(0);
}

void DeferredLighting::InWater(bool flag) {
	glUniform1i(fHeadInWaterIdx, flag);
}

void DeferredLighting::InsideTeleport(bool flag) {
	glUniform1i(fInsideTeleportIdx, flag);
}
