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

// This vertex shader will only draw two triangles, limited to the part of the screen
// that can be affected.
// The vertex input is 0,0 in one corner and 1,1 in the other.

uniform vec4 Upoint;            // A light. .xyz is the coordinate, and .w is the strength
layout (location = 0) in vec2 vertex;
out vec2 screen;             // Screen coordinate
void main(void)
{
	float strength = Upoint.w;
	// Relative bounding (2D) box in front of the fog
	vec3 box =  vec3(vertex*2-1, 0)*strength;
	vec4 viewPos = UBOViewMatrix * vec4(Upoint.xyz, 1);
	vec3 d = normalize(viewPos.xyz);
	// We want to move the quad towards the player. It shall be moved so as
	// precisely be outside the range of the lamp. This is needed as the depth
	// culling will remove parts of the quad that are hidden.
	float l = min(strength, -viewPos.z-1);
	// The modelView is one of the corners of the quad in view space.
	vec4 modelView = -vec4(d, 0)*l + vec4(box, 0) + viewPos;
	vec4 pos = UBOProjectionMatrix * modelView;
	gl_Position = pos;
	// Copy position to the fragment shader. Only x and y is needed. Scale it
	// from interval -1 .. 1, to the interval 0 .. 1.
	screen = pos.xy/2/pos.w+0.5;
}

-- Fragment

uniform sampler2D posTex;     // World position
uniform sampler2D lumTex;     // The blurred luminance map.
uniform vec4 Upoint;          // A point source. .xyz is the coordinate, and .w is the strength of the shadow
in vec2 screen;               // The screen position
layout (location = 0) out vec4 blendColor;

void main(void)
{
	// "	blendColor = vec4(1, 0, 0, 0.5);
	// "	return;
	vec4 worldPos = texture(posTex, screen);
	vec3 cameraToWorld = UBOCamera.xyz-worldPos.xyz;
	float vertexDistance = length(cameraToWorld);
	// Add some small random delta to the radius
	float delta = (fract(screen.x*812219.65 + screen.y*242328.123)+fract(vertexDistance*3122.323))*0.3;
	float dist = 0;             // This is the length of the ray that is inside the fog.
	float radius = Upoint.w + delta;                               // The maximum distance is coded in the w channel
	float cameraToFogDist = distance(UBOCamera.xyz, Upoint.xyz);
	if (vertexDistance + radius < cameraToFogDist) { discard; return; } // Quick test if fog is too far away.
	float pixelToFogDist = distance(worldPos.xyz, Upoint.xyz);
	if (cameraToFogDist < radius && pixelToFogDist < radius) {
		dist = vertexDistance; // Simple case, camera and pixel completely inside fog
	} else {
		vec3 l = normalize(worldPos.xyz-UBOCamera.xyz);
		float ldotc = dot(l,Upoint.xyz-UBOCamera.xyz);
		float tmp = ldotc*ldotc - cameraToFogDist*cameraToFogDist + radius*radius;
		if (cameraToFogDist > radius && pixelToFogDist > radius && ldotc > 0 && tmp > 0) {
			float sqrttmp = sqrt(tmp);
			vec3 entrance = UBOCamera.xyz + l*(ldotc-sqrttmp);
			if (vertexDistance > distance(UBOCamera.xyz, entrance)) dist = sqrttmp*2;
		} else if (cameraToFogDist > radius && pixelToFogDist < radius) {
			vec3 entrance = UBOCamera.xyz + l*(ldotc-sqrt(tmp)); // Outside of fog, looking at pixel inside
			dist = distance(entrance, worldPos.xyz);
		} else if (cameraToFogDist < radius && pixelToFogDist > radius) {
			vec3 exit = UBOCamera.xyz + l*(ldotc+sqrt(tmp)); // Inside of fog, looking at pixel outside
			dist = distance(exit, UBOCamera.xyz);
		}
	}
	float sqr = dist*dist/radius/radius;
	float alpha = 0.25 * sqr;
	float luminance = texture(lumTex, screen).r * 1.05; // Add a small offset to get a little whiter fogs.
	// For blending this fog into the far distance, use distance to fog center as criteria, not the pixel behind the fog.
	float distanceBlending = DistanceAlphaBlending(UBOViewingDistance, cameraToFogDist);
	blendColor = vec4(luminance, luminance, luminance, alpha*distanceBlending);
}
