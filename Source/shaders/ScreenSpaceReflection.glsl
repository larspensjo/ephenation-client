// Copyright 2013 The Ephenation Authors
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

-- Vertex

// This vertex shader will only draw two triangles, giving a full screen.
// The vertex input is 0,0 in one corner and 1,1 in the other.

layout(location = 0) in vec2 vertex;
out vec2 screen;                          // Screen coordinate
void main(void)
{
	gl_Position = vec4(vertex*2-1, 0, 1); // Transform from interval 0 to 1, to interval -1 to 1.
	screen = vertex;                      // Copy position to the fragment shader. Only x and y is needed.
}

-- Fragment
vec3 rainbow(float x) {
	float level = x * 2.0;
	float r, g, b;
	if (level <= 0) {
		r = g = b = 0;
	} else if (level <= 1) {
		r = mix(1, 0, level);
		g = mix(0, 1, level);
		b = 0;
	} else if (level > 1) {
		r = 0;
		g = mix(1, 0, level-1);
		b = mix(0, 1, level-1);
	}
	return vec3(r, g, b);
}

float noise(vec3 v) {
	return snoise((v.xy+v.z)*10);
}

uniform sampler2D colTex;     // Input colors
uniform sampler2D posTex;     // World position
uniform sampler2D normalTex;  // Normals
in vec2 screen;               // The screen position
layout(location = 0) out vec4 color;

void main(void)
{
	vec3 worldStartingPos = texture(posTex, screen).xyz;
	vec3 normal = texture(normalTex, screen).xyz;
	normal.x += noise(worldStartingPos)/20;
	normal.y += noise(worldStartingPos+100)/20;
	vec3 cameraToWorld = worldStartingPos.xyz - UBOCamera.xyz;
	float cameraToWorldDist = length(cameraToWorld);
	vec3 cameraToWorldNorm = normalize(cameraToWorld);
	vec3 refl = normalize(reflect(cameraToWorldNorm, normal));
#ifdef CALIBRATE
	if (dot(refl, cameraToWorldNorm) < 0) {
		// Ignore reflections going backwards towards the camera
		color = vec4(1,1,1,1);
		return;
	}
#endif
	vec4 origColor = texture(colTex, screen);
	float cosAngle = abs(dot(normal, cameraToWorldNorm)); // Will be a value between 0 and 1
	float fact = 1 - cosAngle;
	fact = min(1, 1.3 - fact*fact);
#ifndef CALIBRATE
	if (fact > 0.95) {
		// This test reduce the cost by approximately 50%
		color = origColor;
		return;
	}
#endif // CALIBRATE
	vec3 newPos;
	vec4 newScreen = vec4(screen, 0, 1);
	float i = 0;
	vec3 worldCurrentPos = worldStartingPos;
	float currentWorldDist, currentWorldTestDist;
	float incr = 0.4;
	vec2 lastGoodPos;
	do {
		i += 0.04;
		worldCurrentPos += refl*incr;
		incr *= 1.3;
		lastGoodPos = screen.xy;
		newScreen = UBOProjectionviewMatrix * vec4(worldCurrentPos, 1);
		newScreen /= newScreen.w;
		newPos = texture(posTex, newScreen.xy/2.0+0.5).xyz;
		currentWorldDist = length(newPos.xyz - UBOCamera.xyz);
		currentWorldTestDist = length(worldCurrentPos.xyz - UBOCamera.xyz);
	} while(newScreen.x < 1 && newScreen.x > -1 && newScreen.y < 1 && newScreen.y > -1 && newScreen.z < 1 && newScreen.z > -1 && currentWorldTestDist < currentWorldDist && i < 1.0);
	// } while(0);
#ifdef CALIBRATE
	if (newScreen.x > 1 || newScreen.x < -1 || newScreen.y > 1 || newScreen.y < -1)
		color = vec4(0,0,0,1);
	else
		color = vec4(rainbow(i), 1);
	return;
#endif
	vec4 newColor = texture(colTex, newScreen.xy/2.0 + 0.5);
	if (dot(refl, cameraToWorldNorm) < 0)
		fact = 1.0; // Ignore reflections going backwards towards the camera
	if (newScreen.x > 1 || newScreen.x < -1 || newScreen.y > 1 || newScreen.y < -1)
		fact = 1.0; // Falling outside of screen
	color = origColor*fact + newColor*(1-fact);
	// color = noise(worldStartingPos); // Debug the noise function
}
