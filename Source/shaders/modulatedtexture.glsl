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

uniform mat4 UprojectionMatrix;
uniform mat4 UmodelViewMatrix;
layout (location=0) in vec3 Avertex;      // Location must match VERTEX_INDEX
layout (location=1) in vec2 AtexCoord;    // Location must match TEXTURE_INDEX
layout (location=2) in vec3 AcolorFactor; // Location must match COLOR_INDEX
out vec2 fragmentTexCoord;
out vec3 colorFactor;
void main(void)
{
	fragmentTexCoord = AtexCoord;
	colorFactor = AcolorFactor;
	gl_Position = UprojectionMatrix * UmodelViewMatrix * vec4(Avertex,1.0);
}

-- Fragment

uniform sampler2D UfirstTexture;
in vec2 fragmentTexCoord;
in vec3 colorFactor;
void main(void)
{
	gl_FragColor = texture(UfirstTexture, fragmentTexCoord);
	gl_FragColor.rgb *= colorFactor;
	if (gl_FragColor.a < 0.1) discard;
}
