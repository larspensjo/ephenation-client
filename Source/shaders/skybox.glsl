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

/// Vertex shader for the skybox.
/// This vertex shader will only draw two triangles, limited to the part of the screen
/// that can be affected.
/// The vertex input is 0,0 in one corner and 1,1 in the other. Draw the quad at z -1, with x and y
/// going from -1 to 1 (and then transformed with the model matrix).
/// * @todo [position = pos*10000]

uniform mat3 UmodelMatrix;         // Only rotation needed
layout (location = 0) in vec2 vertex;
out vec2 TexCoord;
out vec3 position;
void main(void)
{
	mat3 view = mat3(UBOViewMatrix);   // Only rotation needed
	vec3 pos = UmodelMatrix * vec3(vertex*2-1, -1);
	gl_Position = UBOProjectionMatrix * vec4(view * pos, 1);
	position = pos*10000;              // This is the important thing; that the sky has a coordinate that is very far away.
	TexCoord = vertex;
}

-- Fragment

uniform sampler2D UTextureSampler;
in vec3 position;       // The model coordinate, as given by the vertex shader
in vec2 TexCoord;

layout(location = 0) out vec4 diffuseOutput;
layout(location = 1) out vec4 posOutput;
layout(location = 2) out vec4 normOutput;

void main(void)
{
	vec4 color = texture(UTextureSampler, TexCoord);
	diffuseOutput = color;
	normOutput = vec4(0,0,0,1);    // Last byte is ambient light
	posOutput = vec4(position, 1); // Last byte is sun intensity
}
