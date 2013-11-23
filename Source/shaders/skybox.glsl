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

uniform mat3 UmodelMatrix;         // Only rotation needed
layout (location = 0) in vec2 vertex;
out vec2 TexCoord;
out vec3 worldPos;
void main(void)
{
	mat3 view = mat3(UBOViewMatrix);   // Only rotation needed
	vec3 pos = UmodelMatrix * vec3(vertex*2-1, -1);
	gl_Position = UBOProjectionMatrix * vec4(view * pos, 1);
	worldPos = pos*10000;              // This is the important thing; that the sky has a coordinate that is very far away.
	TexCoord = vertex;
}

-- Fragment

uniform sampler2D UTextureSampler;
uniform bool UDisableDistortion = true;
in vec3 worldPos;       // The model coordinate, as given by the vertex shader
in vec2 TexCoord;

layout(location = 0) out vec3 diffuseOutput;
layout(location = 1) out vec4 posOutput;

void main(void)
{
	vec2 coord = TexCoord;
	if (UBOEnableDistortion == 1 && !UDisableDistortion) {
		coord = HmdWarp(TexCoord-vec2(0.5, 0.5)) + vec2(0.5, 0.5);
	}
	vec3 color = texture(UTextureSampler, coord).rgb;
	// Use a vertical fog gradient to add fog to all pixels below a certain height.
	vec3 cameraToWorld = UBOCamera.xyz-worldPos.xyz;
	vec3 eyeDir = normalize(cameraToWorld);
	float vertFogGrad = 1.0 - clamp(dot(-eyeDir, vec3(0,1,0))-0.1, 0.0, 0.25) / 0.25;
	vec3 fogColor = vec3(0.6, 0.6, 0.6);
	diffuseOutput = mix(color, fogColor, vertFogGrad);
	posOutput = vec4(worldPos, 1); // Last byte is sun intensity
}
