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
	vec4 pos = GetTileFromSphere(Upoint.xyz, Upoint.w, vertex);
	gl_Position = pos;
	// Copy position to the fragment shader. Only x and y is needed. Scale it
	// from interval -1 .. 1, to the interval 0 .. 1.
	screen = pos.xy/2+0.5;
}

-- Fragment

uniform sampler2D posTex;     // World position
uniform vec4 Upoint;          // A point source. .xyz is the coordinate, and .w is the radius of the shadow
uniform int Umode = 0;        // Special case, use a selection color instead of a shadow
in vec2 screen;               // The screen position
layout (location = 0) out vec4 colorChange;

void main(void)
{
	vec4 worldPos = texture(posTex, screen);
	float dist = distance(worldPos.xyz, Upoint.xyz); // Distance to the player or monster
	if (worldPos.y - Upoint.y > 0.3 && Umode < 2) { discard; return; }   // Don't draw shadow too high on the object itself.
	float radius = Upoint.w;                         // The maximum distance (radius of the shadow) is coded in the w channel
	if (dist >= radius) {discard; return; }          // Is the pixel too far away?
	float cameraToWorldDistance = length(UBOCamera.xyz-worldPos.xyz);
	float alpha = DistanceAlphaBlending(UBOViewingDistance, cameraToWorldDistance);
	float f;
	if (Umode >= 2) {
		f = min(dist/radius/2+0.5, 1);               // 'f' goes from 0.5 to 1, depending on distance.
		f = 1 - (1-f)*alpha;                         // Blend between colored light radius fading, and skybox distance fading.
	}
	switch(Umode) {
	case 4: // Add blue tint by multiplying color channels
		colorChange = vec4(f, f, 1, 1);
		break;
	case 3: // Add green tint by multiplying color channels
		colorChange = vec4(f, 1, f, 1);
		break;
	case 2: // Add red tint by multiplying color channels
		colorChange = vec4(1, f, f, 1);
		break;
	case 1: // Red marker below monster feet
		colorChange = vec4(1, 0, 0, 0.3*alpha);      // Shall be used as a blending component, adding a red marker at the feet of a monster.
		break;
	case 0: // Static shadow below trees
		f = (1 - dist/radius)/1.1;                   // 'f' goes from 0.91 to 0, depending on distance.
		colorChange = vec4(0, 0, 0, alpha*f);
		break;
	}
}
