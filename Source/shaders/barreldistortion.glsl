// Copyright 2014 The Ephenation Authors
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

uniform mat4 modelViewMatrix;
in vec3 vertex;
in vec2 texCoord;
out vec2 fragmentTexCoord;
void main(void)
{
	fragmentTexCoord = texCoord;
	gl_Position = modelViewMatrix * vec4(vertex, 1.0);
}

-- Fragment

uniform sampler2D firstTexture;
uniform vec4 UOVRDistortion;
uniform vec2 ULensCenter;
in vec2 fragmentTexCoord;

// Apply distortion effect to a coordinate. It has to be centered at 0,0, and range from -1 to +1.
vec2 HmdWarp(vec2 in01)
{
	vec2 theta = in01 - UBOLensCenter;
	float rSq = theta.x * theta.x + theta.y * theta.y;
	vec2 rvector = theta * (UBOOVRDistortion.x +
							UBOOVRDistortion.y * rSq +
							UBOOVRDistortion.z * rSq * rSq +
							UBOOVRDistortion.w * rSq * rSq * rSq
			);
	return rvector + UBOLensCenter;
}

void main(void)
{
	vec2 coord = HmdWarp((fragmentTexCoord-vec2(0.5, 0.5))*2)/2 + vec2(0.5, 0.5);
	gl_FragColor = texture(firstTexture, coord);
}
