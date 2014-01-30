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

#define M_PI 3.1415926535897932384626433832795

// #define CALIBRATE

// These values are a subset from common.glsl.
// The subset was tested to give a good result
const vec2 gPoissonDisk[] = vec2[] (
	vec2( 0.0651875, 0.708609 ),
	vec2( 0.641987, 0.0233772 ),
	vec2( 0.376415, 0.944243 ),
	vec2( 0.827723, 0.723258 )
);

float refDist;
float py;
float px;

bool AcuteAngle(int ind) {
	vec2 delta = (gPoissonDisk[ind]*2-1)*vec2(px,py);
	float sampleDist1 = WorldDistance(depthTex, screen+delta);
	float sampleDist2 = WorldDistance(depthTex, screen-delta);
	float v1 = atan((refDist - sampleDist1) / length(delta));
	float v2 = atan((refDist - sampleDist2) / length(delta));
	float v = M_PI - v1 - v2;
	const float cutoff = 0.58;
	if (sampleDist1 < refDist - cutoff)
		v = M_PI;
	if (sampleDist2 < refDist - cutoff)
		v = M_PI;
	if (v < M_PI*0.20)
		return true;
	return false;
}

void main(void)
{
	refDist = WorldDistance(depthTex, screen);
	float found = 0;
	float total = 0;
	py = 0.115/refDist;
	px = 0.115/refDist;
	for (int i=0; i<4; i++) {
		if (AcuteAngle(i))
			found++;
		total += 1.0;
	}

	float thresh = 0.07;
	if (found > total*thresh) {
#ifdef CALIBRATE
		const float bs = 0.1;
		light = bs + (1-bs) * (found - total*thresh) / (1.0 - thresh) / total;
#else
		const float bs = 0.55;
		light = 1.0 - (1-bs) * (found - total*thresh) / (1.0 - thresh) / total;
#endif
	} else {
#ifdef CALIBRATE
		light = 0.1;
#else
		light = 1.0;
#endif
	}
}
