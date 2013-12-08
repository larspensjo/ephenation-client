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

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform vec3 textOffsMulti; // This contains both a offset and a multiplier
in vec3 vertex;
in vec2 texCoord;
out vec2 fragmentTexCoord;
void main(void)
{
	float textMult = textOffsMulti.z;
	fragmentTexCoord = texCoord*textMult + textOffsMulti.xy;
	gl_Position = projectionMatrix * modelViewMatrix * vec4(vertex,1.0);
}

-- Fragment

uniform sampler2D firstTexture;
uniform float forceTransparent;
uniform vec3 colorOffset;
in vec2 fragmentTexCoord;
uniform bool UCompensateDistortion = false;
void main(void)
{
	vec2 coord = fragmentTexCoord;
	if (UCompensateDistortion) {
		coord = HmdWarp((fragmentTexCoord-vec2(0.5, 0.5))*2)/2 + vec2(0.5, 0.5);
	}
	gl_FragColor = texture(firstTexture, coord) + vec4(colorOffset, 0);
	if (gl_FragColor.a < 0.1) discard;
	if (forceTransparent < 1) gl_FragColor.a = forceTransparent; // The transparency isn't used unless enabled.
}
