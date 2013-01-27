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
//

-- Vertex

uniform mat4 modelMatrix;
in vec3 normal;
in vec3 vertex;
out vec3 fragmentNormal;
void main(void)
{
	vec4 vertexScaled = vec4(vec3(vertex) / VERTEXSCALING, 1);
	// Special scaling applies for the normal in picking mode.
	fragmentNormal = normal/255; // The normal will be coded into the color, scaled from range 0..255 to 0..1.
	gl_Position = UBOProjectionviewMatrix * modelMatrix * vertexScaled;
}

-- Fragment

in vec3 fragmentNormal;
void main(void)
{
	gl_FragColor = vec4(fragmentNormal,1.0);
}
