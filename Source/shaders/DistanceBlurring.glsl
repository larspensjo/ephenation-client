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

uniform sampler2D colTex;     // Color texture sampler
in vec2 screen;               // The screen position (0 to 1)

layout(location = 0) out vec4 color;

void main(void)
{
	float xPixel = 1.0/float(UBOWindowWidth); // Width of one pixel
	float yPixel = 1.0/float(UBOWindowHeight); // Height of one pixel
	float f = 0.5;
	color = (
			texture(colTex, screen+vec2( xPixel, yPixel)*f) +
			texture(colTex, screen+vec2( xPixel,-yPixel)*f) +
			texture(colTex, screen+vec2(-xPixel, yPixel)*f) +
			texture(colTex, screen+vec2(-xPixel,-yPixel)*f)
			) / 4.0;
}
