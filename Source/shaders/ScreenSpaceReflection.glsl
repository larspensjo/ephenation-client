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

// Create a float value 0 to 1 into a color from red, through green and then blue.
vec4 rainbow(float x) {
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
	return vec4(r, g, b, 1);
}

uniform sampler2D colTex;     // Color texture sampler
uniform sampler2D posTex;     // World position texture sampler
uniform sampler2D normalTex;  // Normal texture sampler
uniform usampler2D materialTex;  // Material texture sampler
in vec2 screen;               // The screen position (0 to 1)

layout(location = 0) out vec4 color;

// #define CALIBRATE // Define this to get a color coded representation of number of needed iterations
void main(void)
{
	vec4 origColor = texture(colTex, screen);
	uint effectType = texture(materialTex, screen).r & 0xfu; // High nibbles are used for modulating factor
	if (effectType != 1u) {
		color = origColor;
		return;
	}
	vec3 worldStartingPos = texture(posTex, screen).xyz;
	vec3 normal = texture(normalTex, screen).xyz;
	vec3 cameraToWorld = worldStartingPos.xyz - UBOCamera.xyz;
	float cameraToWorldDist = length(cameraToWorld);
	float scaleNormal = max(3.0, cameraToWorldDist*1.5);
#ifndef CALIBRATE
	normal.x += snoise(worldStartingPos*10)/scaleNormal;
	normal.y += snoise(worldStartingPos*10+100)/scaleNormal;
#endif
	vec3 cameraToWorldNorm = normalize(cameraToWorld);
	vec3 refl = normalize(reflect(cameraToWorldNorm, normal)); // This is the reflection vector
#ifdef CALIBRATE
	if (dot(refl, cameraToWorldNorm) < 0) {
		// Ignore reflections going backwards towards the camera, indicate with white
		color = vec4(1,1,1,1);
		return;
	}
#endif
	float cosAngle = abs(dot(normal, cameraToWorldNorm)); // Will be a value between 0 and 1
	float fact = 1 - cosAngle;
	fact = min(1, 1.38 - fact*fact);
#ifndef CALIBRATE
	if (fact > 0.95) {
		color = origColor;
		return;
	}
#endif // CALIBRATE
	vec3 newPos;
	vec4 newScreen;
	float i = 0;
	vec3 rayTrace = worldStartingPos;
	float currentWorldDist, rayDist;
	float incr = 0.4;
	do {
		i += 0.05;
		rayTrace += refl*incr;
		incr *= 1.3;
		newScreen = UBOProjectionviewMatrix * vec4(rayTrace, 1);
		newScreen /= newScreen.w;
		newPos = texture(posTex, newScreen.xy/2.0+0.5).xyz;
		currentWorldDist = length(newPos.xyz - UBOCamera.xyz);
		rayDist = length(rayTrace.xyz - UBOCamera.xyz);
		if (newScreen.x > 1 || newScreen.x < -1 || newScreen.y > 1 || newScreen.y < -1 || newScreen.z > 1 || newScreen.z < -1 || i >= 1.0 || cameraToWorldDist > currentWorldDist) {
			fact = 1.0; // Ignore any reflection
			break; // This is a failure mode.
		}
	} while(rayDist < currentWorldDist);
	// } while(0);
#ifdef CALIBRATE
	if (cameraToWorldDist > currentWorldDist)
		color = vec4(1,1,0,1); // Yellow indicates we found a pixel hidden behind another object
	else if (newScreen.x > 1 || newScreen.x < -1 || newScreen.y > 1 || newScreen.y < -1)
		color = vec4(0,0,0,1); // Black used for outside of screen
	else if (newScreen.z > 1 && newScreen.z < -1)
		color = vec4(1,1,1,1); // White outside of frustum
	else
		color = rainbow(i); // Encode number of iterations as a color. Red, then green, and last blue.
	return;
#endif
	vec4 newColor = texture(colTex, newScreen.xy/2.0 + 0.5);
	if (dot(refl, cameraToWorldNorm) < 0)
		fact = 1.0; // Ignore reflections going backwards towards the camera
	else if (newScreen.x > 1 || newScreen.x < -1 || newScreen.y > 1 || newScreen.y < -1)
		fact = 1.0; // Falling outside of screen
	else if (cameraToWorldDist > currentWorldDist)
		fact = 1.0;
	color = origColor*fact + newColor*(1-fact);
}
