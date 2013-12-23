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

uniform mat4 modelMatrix;
in vec4 vertex; // First 3 are vertex coordinates, the 4:th is texture data coded as two scaled bytes
out vec2 fragmentTexCoord;
out vec3 position;
out vec2 screen;             // Screen coordinate
void main(void)
{
	vec4 vertexScaled = vec4(vec3(vertex) / VERTEXSCALING, 1);
	int t1 = int(vertex[3]); // Extract the texture data
	vec2 tex = vec2(t1&0xFF, t1>>8);
	vec2 textureScaled = tex / TEXTURESCALING;
	fragmentTexCoord = textureScaled;
	vec4 pos = UBOProjectionviewMatrix * modelMatrix * vertexScaled;
	gl_Position = pos;
	screen = pos.xy/2/pos.w+0.5;
	position = vec3(modelMatrix * vertexScaled); // Copy position to the fragment shader
}

-- Fragment

uniform sampler2D firstTexture;
uniform sampler2D posTexture;
uniform sampler2D USkyBox;
uniform bool depthDependingAlpha = false;
uniform vec2 screenSize = vec2(1920, 1003); // A default as a safety precaution
uniform float time = 0; // Number of seconds since the game was started

in vec2 screen;               // The screen position
in vec2 fragmentTexCoord;
in vec3 position;       // The model coordinate, as given by the vertex shader

layout(location = 0) out vec4 blendOutput;

float height(vec3 pos) {
	float wave1 = snoise(vec2(pos.x/3, pos.z+time/2.5))/10;
	float wave2 = snoise(vec2(pos.x/3+1000, pos.z+time/2.5))/10;
	return wave2*sin(time) + wave1*sin(time+3.14/2);
}

void main(void) {
	float distCamPos = distance(UBOCamera.xyz, position);
	if (distCamPos > UBOViewingDistance) { discard; return; }
	vec2 textCoord = fract(fragmentTexCoord);
	vec4 clr = texture(firstTexture, textCoord);
	vec3 backgroundPos = texture(posTexture, screen).xyz;
	if (depthDependingAlpha) {
		float dist = distance(position, backgroundPos); // TODO: Move this logic to the vertex shader?
		clr.a = clamp(0.4+dist/50, 0, 1);
		if (distCamPos<100 && UBOPerformance > 1) {
			float delta = 0.1;
			float centerHeight = height(position);
			float rightHeight = height(position + vec3(delta, 0, 0));
			float belowHeight = height(position + vec3(0, 0, delta));
			vec2 normal = vec2(centerHeight - rightHeight, centerHeight - belowHeight) / delta;
			// clr.rgb += height(position) * (1-distCamPos/100); // Let the water ripples fade out with distance
			clr.rgb += texture(USkyBox, 0.5+normal*vec2(3.5, 1.2)).rgb * (1-distCamPos/100);
			// clr.rgb = vec4(normal*vec2(3.5, 1.2), 0, 1);
		}
		clr.rgb *= clr.a; // This texture isn't premultiplied
	}
	blendOutput = clr; // The colors are already premultiplied by alpha
	blendOutput *= DistanceAlphaBlending(UBOViewingDistance, distCamPos);
}
