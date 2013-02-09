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

-- Vertex

layout (location=0) in vec2 vertex;
out vec2 screen;                           // Screen coordinate
void main(void)
{
	gl_Position = vec4(vertex*2-1, 0, 1); // Transform from interval 0 to 1, to interval -1 to 1.
	screen = vertex;                         // Copy position to the fragment shader. Only x and y is needed.
}

-- Fragment

uniform sampler2D diffuseTex; // The color information
uniform sampler2D posTex;     // World position
uniform sampler2D normalTex;  // Normals
uniform sampler2D blendTex;   // A bitmap with colors to blend with, afterwars.
uniform sampler2D lightTex;   // A bitmap with colors to blend with, afterwars.
uniform sampler1D poissondisk;
uniform bool Udead;            // True if the player is dead
uniform bool Uwater;           // True when head is in water
uniform bool Uteleport;        // Special mode when inside a teleport
uniform float UwhitePoint = 3.0;
in vec2 screen;               // The screen position
layout(location = 0) out vec4 fragColorFinal;

bool skyPixel = false;
vec4 worldPos;
vec4 normal;

float linearToSRGB(float linear) {
	if (linear <= 0.0031308) return linear * 12.92;
	else return 1.055 * pow(linear, 1/2.4) - 0.055;
}

vec2 seed;
vec2 rand(vec2 a, vec2 b) {
	seed = fract(a*10.23 + b*123.1232+screen*3.123 + seed*82.12354); // A value from 0 to 1
	return seed;
}

void main(void)
{
	// Load data, stored in textures, from the first stage rendering.
	normal = texture(normalTex, screen);
	vec4 diffuse = texture(diffuseTex, screen) * 0.95; // Downscale a little, 1.0 can't be mapped to HDR.
	vec4 blend = texture(blendTex, screen);
	worldPos = texture(posTex, screen);
	float ambient = normal.a; // The ambient light is in the alpha channel.
	vec4 hdr = diffuse/(1-diffuse);
	// Temporary helper data
	vec3 cameraToWorld = UBOCamera.xyz-worldPos.xyz;
	float cameraToWorldDistance = length(cameraToWorld);
	// The sky is more than 1000 blocks away
	if (cameraToWorldDistance > 1000) skyPixel = true;
	vec3 eyeDir = normalize(cameraToWorld);
	vec3 vHalfVector = normalize(sundir.xyz+eyeDir);
	float inSun = worldPos.a; // Is greater than 0 if this position is reached by the sun
	float fact = texture(lightTex, screen).r;
	float num = 1;
	// Do some multi sampling to blur the lighting. It is kind of a box filter, but with enhanced weights
	// dynamically controlled by the geometry.
	for (int i=0; i < 4; i++) {
		const float filterpixels = 4; // The size of the area to include in the filter.
		vec2 ind = screen + rand2(screen)/UBOWindowHeight*filterpixels;
		vec3 pos2 = texture(posTex, ind).xyz;
		vec3 normal2 = texture(normalTex, ind).xyz;
		const float maxdist = 1; // Ignore contributions when distance difference exceeds this value
		float dist = min(abs(distance(UBOCamera.xyz, pos2) - cameraToWorldDistance), maxdist); // A value between 0 and maxdist.
		//		Use a weight that depends on the distance difference. Also, if there is a sharp corner, we don't want
		//		shadows to bleed around the corner. This this is prevented by using a test for normals.
		float weight = (maxdist-dist)*dot(normal.xyz, normal2); // A value from 0 to 1
		fact += weight * texture(lightTex, ind).r;
		num += weight;
	}
	fact = fact/num + (ambient+UBOambientLight)*0.03;
	if (UBODynamicshadows == 0) fact += inSun;         // Add pre computed light instead of using shadow map
	if (skyPixel) { fact = 0.8; }
	vec3 step1 = fact*hdr.xyz; // + inSun*pow(max(dot(normal.xyz,vHalfVector),0.0), 100) * 0.1; // TODO: Specular glare isn't correct
	step1 *= UBOexposure;

	// Apply Reinhard tone mapping, based on the luminance
	float Lwhite2 = UwhitePoint*UwhitePoint;
	float L = 0.2126 * step1.r + 0.7152 * step1.g + 0.0722 * step1.b;
	vec3 step2 = step1 * (1+L/Lwhite2)/(1+L);

	vec3 fragColor = (1-blend.a)*step2 + blend.xyz;     // manual blending, using premultiplied alpha.
	//	Add some post processing effects
	fragColor.x = linearToSRGB(fragColor.x);       // Transform to non-linear space
	fragColor.y = linearToSRGB(fragColor.y);       // Transform to non-linear space
	fragColor.z = linearToSRGB(fragColor.z);       // Transform to non-linear space
	if (Uwater) {
		//		Player in water, gradually decrease visibility with distance
	    float a = clamp(0.3+length(cameraToWorld)/50, 0, 1);
	    if (skyPixel) a = 1;
	    fragColor = mix(fragColor, vec3(0, 0.1, 0.5), a);
	}
	if (Udead) {
		//		Player is dead, draw everything in black and white.
	    float averageColor = fragColor.r*0.3 + fragColor.g*0.6 + fragColor.b*0.1;
	    fragColor.r = averageColor;
	    fragColor.g = averageColor;
	    fragColor.b = averageColor;
	}
	if (Uteleport) {
		//		Inside teleport, dim the light.
	    fragColor.r = (fragColor.r)/4;
	    fragColor.g = (fragColor.g)/4;
	    fragColor.b = (fragColor.b)/4;
	}

	fragColorFinal = vec4(fragColor, DistanceAlphaBlending(UBOViewingDistance, cameraToWorldDistance));
}
