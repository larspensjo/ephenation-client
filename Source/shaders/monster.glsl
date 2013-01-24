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

uniform mat4 modelViewMatrix;
uniform mat3 normalMatrix; // TODO: This can be computed from the model matrix
uniform mat4 modelMatrix;
uniform int cycle;
in vec3 vertex;
in vec2 texCoord;
in vec3 normal;
out vec2 fragmentTexCoord;
out vec3 fragNormal;
out vec3 position;
void main(void)
{
	fragmentTexCoord = texCoord/180; // This will stretch the texture over several blocks.
	fragNormal = normalize(normalMatrix * normal);
	gl_Position = UBOProjectionMatrix * modelViewMatrix * vec4(vertex,1.0);
	position = vec3(modelMatrix * vec4(vertex, 1.0)); // Copy position to the fragment shader
}

-- Fragment

uniform sampler2D firstTexture;
uniform vec4 colorAddon; // Allow the color to be tweaked.
uniform float sunIntensity = 1;
uniform float ambient = 0.2;
in vec2 fragmentTexCoord;
in vec3 fragNormal;
in vec3 position;       // The model coordinate, as given by the vertex shader
out vec4 diffuseOutput; // layout(location = 0)
out vec4 posOutput;     // layout(location = 1)
out vec4 normOutput;    // layout(location = 2)
out vec4 blendOutput;   // layout(location = 3)
void main(void)
{
	diffuseOutput = texture(firstTexture, fragmentTexCoord) + colorAddon;
	posOutput = vec4(position,sunIntensity); // Set the sun flag in the alpha channel
	normOutput = vec4(fragNormal, ambient);  // Add some ambient in alpha channel. No normalize() needed as long as all surfaces are flat.
	//	gl_FragColor = vec4(fragNormal, 1);  // Used for debugging texture coordinates.
}
