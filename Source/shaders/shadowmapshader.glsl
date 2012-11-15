// Copyright 2012 The Ephenation Authors
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

uniform mat4 projectionViewMatrix;
uniform mat4 modelMatrix;
// This contains both a offset and a multiplier to be used for the bitmap. It enables the use
// of atlas bitmaps.
uniform vec3 textOffsMulti = vec3(0,0,1);
in vec4 vertex; // First 3 are vertex coordinates, the 4:th is texture data coded as two scaled bytes
out vec2 fragmentTexCoord;
void main(void)
{
	vec4 vertexScaled = vec4(vec3(vertex) / VERTEXSCALING, 1);
	int t1 = int(vertex[3]); // Extract the texture data
	vec2 tex = vec2(t1&0xFF, t1>>8);
	vec2 textureScaled = tex / TEXTURESCALING;
	vec4 pos = projectionViewMatrix * modelMatrix * vertexScaled;
	pos.xy = DoubleResolution(pos.xy);
	gl_Position = pos;
	float textMult = textOffsMulti.z;
	fragmentTexCoord = textureScaled*textMult + textOffsMulti.xy;
}

-- Fragment

uniform sampler2D firstTexture;
in vec2 fragmentTexCoord;
void main(void)
{
	vec4 clr = texture(firstTexture, fract(fragmentTexCoord));
	if (clr.a == 0) { discard; }
}
