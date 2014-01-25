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
    screen = vertex;                       // Copy position to the fragment shader. Only x and y is needed.
}

-- Fragment

uniform sampler2D depthTex;   // The depth buffer
in vec2 screen;               // The screen position
layout(location = 0) out float light; // Used as a multiplicative effect

// #define CALIBRATE

void main(void)
{
	float refDist = WorldDistance(depthTex, screen);
	int num = 0;
	const int SIZE = 13;
	float py = 1.0/32.0;
	float px = 1.0/187.0; // Much tighter on x
	for (int i=0; i<SIZE; i++) {
		vec2 sampleInd = screen + (gPoissonDisk[i]*2-1)*vec2(px,py);
		float sampleDist = WorldDistance(depthTex, sampleInd);
		if (sampleDist-refDist > 0.42) { num-=10; }
		if (refDist-sampleDist > 1.5) { num-=10; }
		if (sampleDist < refDist) num++;
	}
	if (num > SIZE*0.78)
#ifdef CALIBRATE
		light = 1.0;
#else
		light = 0.64;
#endif
	else {
#ifdef CALIBRATE
		light = 0.2;
#else
		light = 1.0;
#endif
	}
}
