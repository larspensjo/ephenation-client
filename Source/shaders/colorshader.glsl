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

uniform mat4 UprojectionMatrix;
uniform mat4 UmodelViewMatrix;
layout (location=0) in vec3 Avertex; // Location must match VERTEX_INDEX
layout (location=1) in vec4 Acolor;  // Location must match COLOR_INDEX
out vec4 color;
void main(void)
{
	gl_Position = UprojectionMatrix * UmodelViewMatrix * vec4(Avertex,1.0);
	color = Acolor;
}

-- Fragment

uniform vec4 Ucolor;
in vec4 color;
void main(void)
{
	if (Ucolor.a != 0)
		gl_FragColor = Ucolor;
	else
		gl_FragColor = color;
}
