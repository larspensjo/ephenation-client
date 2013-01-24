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

/// This vertex shader will only draw two triangles, limited to the part of the screen
/// that can be affected.
/// The vertex input is 0,0 in one corner and 1,1 in the other.
uniform vec4 Upoint;            // A light. .xyz is the coordinate, and .w is the strength (radius)
layout (location = 0) in vec2 vertex;
out vec2 screen;                // Screen coordinate
void main(void)
{
	float radius = Upoint.w;    // Interpreted as the radius of the shadow
	// Relative bounding (2D) box around the point light
	vec3 box =  vec3(vertex*2-1, 0)*radius;
	vec4 viewPos = UBOViewMatrix * vec4(Upoint.xyz, 1);
	vec3 d = normalize(viewPos.xyz);
	// We want to move the quad towards the player. It shall be moved so as
	// precisely be outside the range of the lamp. This is needed as the depth
	// culling will remove parts of the quad that are hidden.
	float l = min(radius, -viewPos.z-1);
	// The modelView is one of the corners of the quad in view space.
	vec4 modelView = -vec4(d, 0)*l + vec4(box, 0) + viewPos;
	vec4 pos = UBOProjectionMatrix * modelView;
	pos /= pos.w;
	gl_Position = pos;
	// Copy position to the fragment shader. Only x and y is needed. Scale it
	// from interval -1 .. 1, to the interval 0 .. 1.
	screen = pos.xy/2+0.5;
}

-- Fragment

uniform sampler2D posTex;     // World position
uniform sampler2D normalTex;  // Normals
uniform vec4 Upoint;          // A point source. .xyz is the coordinate, and .w is the radius of the shadow
uniform int Umode = 0;        // Special case, use a selection color instead of a shadow
in vec2 screen;               // The screen position
layout (location = 0) out vec4 colorChange;

void main(void)
{
	vec4 normal = texture(normalTex, screen);
	if (normal.xyz == vec3(0,0,0)) { discard; return;}   // No normal, which means sky
	vec4 worldPos = texture(posTex, screen);
	float dist = distance(worldPos.xyz, Upoint.xyz);     // Distance to the player or monster
	if (worldPos.y - Upoint.y > 0.3 && Umode < 2) { discard; return; }   // Don't draw shadow too high on the object itself.
	float radius = Upoint.w;                             // The maximum distance (radius of the shadow) is coded in the w channel
	if (dist >= radius) {discard; return; }             // Is the pixel near, below the player/monster?
	float f, upper, lower;
	if (Umode >= 2) {
		upper = 1.0 + (1.0 - dist/radius) * 0.8;
		lower = 1.0 - (upper-1.0)/2;
	}
	switch(Umode) {
	case 4:
		colorChange = vec4(lower, lower, upper, 1);
		break;
	case 3:
		colorChange = vec4(lower, upper, lower, 1);
		break;
	case 2:
		colorChange = vec4(upper, lower, lower, 1);
		break;
	case 1:
		colorChange = vec4(1, 0, 0, 0.3);                   // Shall be used as a blending component, adding a red marker at the feet of a monster.
		break;
	case 0:
		f = 1 - (1 - dist/radius)/1.1;                      // 'f' goes from 0.09 to 1.0, depending on distance.
		colorChange = vec4(f, f, f, 1);                     // Will be used as a multiplicative component, making center almost black.
		break;
	}
}
