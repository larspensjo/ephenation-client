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

uniform mat4 modelMatrix;
uniform mat4 bonesMatrix[64];
uniform bool forShadowmap = false;
uniform mat4 shadowProjViewMat;
in vec4 normal;
in vec4 vertex; // First 3 are vertex coordinates, the 4:th is texture data coded as two scaled bytes
in vec3 weights;
in vec3 joints;
out vec3 fragmentNormal;
out vec2 fragmentTexCoord;
out float extIntensity;
out float extAmbientLight;
out vec3 position;
void main(void)
{
	vec4 vertexScaled = vec4(vec3(vertex) / VERTEXSCALING, 1);
	int t1 = int(vertex[3]); // Extract the texture data
	vec2 tex = vec2(t1&0xFF, t1>>8);
	vec2 textureScaled = tex / TEXTURESCALING;
	vec3 normalScaled = normal.xyz / NORMALSCALING;
	int intens2 = int(normal[3]); // Bit 0 to 3 is sun intensity, 4 to 7 is ambient light
	if (intens2 < 0) intens2 += 256;
	mat4 animationMatrix = weights[0] * bonesMatrix[int(joints[0])] + weights[1] * bonesMatrix[int(joints[1])] + weights[2] * bonesMatrix[int(joints[2])];
	mat4 newModel = modelMatrix * animationMatrix;
	vec4 pos;
	if (forShadowmap) {
		pos = shadowProjViewMat * newModel * vertexScaled;
		pos.xy = DoubleResolution(pos.xy);
	} else {
		pos = UBOProjectionviewMatrix * newModel * vertexScaled;
		// Scale the intensity from [0..255] to [0..1].
		fragmentTexCoord = textureScaled;
		extIntensity = (intens2 & 0x0F)/15.0;
		extAmbientLight = (intens2 >> 4)/15.0;
		fragmentNormal = normalize((newModel*vec4(normalScaled, 0.0)).xyz);
	}
	position = vec3(newModel * vertexScaled); // Copy position to the fragment shader
	gl_Position = pos;
}

-- Fragment

uniform sampler2D firstTexture;
uniform bool forShadowmap = false;
in vec3 fragmentNormal;
in vec2 fragmentTexCoord;
in float extIntensity;
in float extAmbientLight;
in vec3 position;       // The model coordinate, as given by the vertex shader
out vec4 diffuseOutput; // layout(location = 0)
out vec4 posOutput;     // layout(location = 1)
out vec4 normOutput;    // layout(location = 2)
void main(void)
{
   if (distance(UBOCamera.xyz, position) > UBOViewingDistance) { discard; return; }
	vec4 clr = texture(firstTexture, fract(fragmentTexCoord));
   float alpha = clr.a;
   if (alpha == 0) { discard; return; }
   if (forShadowmap) return;		// Ignore the rest, only depth buffer is used anyway.
   posOutput.xyz = position;   // This position shall not be transformed by the view matrix
   posOutput.a = extIntensity; // Use the alpha channel for sun intensity
   normOutput = vec4(fragmentNormal, extAmbientLight); // Use alpha channel of normal for ambient info.
   diffuseOutput = clr;
}
