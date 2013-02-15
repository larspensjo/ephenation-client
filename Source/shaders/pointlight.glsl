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
// The vertex input is -1,-1 in one corner and 1,1 in the other.
uniform vec4 Upoint;            // A light. .xyz is the coordinate, and .w is the strength
layout (location = 0) in vec2 vertex;
out vec2 screen;             // Screen coordinate
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
uniform sampler2D normalTex;  // Normals
uniform vec4 Upoint;           // A light. .xyz is the coordinate, and .w is the strength of the lamp
in vec2 screen;                // The screen position
layout (location = 0) out float addLighting;

void main(void)
{
	// Load data, stored in textures, from the first stage rendering.
	// 	addLighting = 0.5;return;
	vec4 normal = texture(normalTex, screen);
	vec4 worldPos = texture(posTex, screen);
	float dist = distance(worldPos.xyz, Upoint.xyz); // Distance to the lamp
	float strength = Upoint.w;
	if (dist >= strength) { discard; return; }
	vec3 lampVector = normalize(worldPos.xyz - Upoint.xyz);
	float mult = clamp(-dot(lampVector, normal.xyz), 0, 1);
	addLighting = 1.5*(1 - dist/strength)*mult;
}
