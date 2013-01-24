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
void main(void)
{
	vec4 vertexScaled = vec4(vec3(vertex) / VERTEXSCALING, 1);
	int t1 = int(vertex[3]); // Extract the texture data
	vec2 tex = vec2(t1&0xFF, t1>>8);
	vec2 textureScaled = tex / TEXTURESCALING;
	fragmentTexCoord = textureScaled;
	gl_Position = UBOProjectionviewMatrix * modelMatrix * vertexScaled;
	position = vec3(modelMatrix * vertexScaled); // Copy position to the fragment shader
}

-- Fragment

uniform sampler2D firstTexture;
uniform sampler2D posTexture;
uniform bool depthDependingAlpha = false;
uniform vec2 screenSize = vec2(1920, 1003); // A default as a safety precaution
uniform float time = 0; // Number of seconds since the game was started
in vec2 fragmentTexCoord;
in vec3 position;       // The model coordinate, as given by the vertex shader
out vec4 blendOutput;   // layout(location = 0)

vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }
/// This is an implementation of a 2D simplex noise
float snoise(vec2 v) {
	const vec4 C = vec4(0.211324865405187, 0.366025403784439, -0.577350269189626, 0.024390243902439);
	vec2 i  = floor(v + dot(v, C.yy) );
	vec2 x0 = v -   i + dot(i, C.xx);
	vec2 i1; i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
	vec4 x12 = x0.xyxy + C.xxzz;
	x12.xy -= i1;
	i = mod289(i);
	vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 )) + i.x + vec3(0.0, i1.x, 1.0 ));
	vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
	m = m*m ; m = m*m ;
	vec3 x = 2.0 * fract(p * C.www) - 1.0;
	vec3 h = abs(x) - 0.5;
	vec3 ox = floor(x + 0.5);
	vec3 a0 = x - ox;
	m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
	vec3 g; g.x  = a0.x  * x0.x  + h.x  * x0.y; g.yz = a0.yz * x12.xz + h.yz * x12.yw;
	return 130.0 * dot(m, g);
}

void main(void) {
	vec2 screen = gl_FragCoord.xy / screenSize;
	float distCamPos = distance(UBOCamera.xyz, position);
	if (distCamPos > UBOViewingDistance) { discard; return; }
	vec2 textCoord = fract(fragmentTexCoord);
	vec4 clr = texture(firstTexture, textCoord);
	vec3 backgroundPos = texture(posTexture, screen).xyz;
	float alpha = clr.a;
	if (depthDependingAlpha) {
		float dist = distance(position, backgroundPos); // TODO: Move this logic to the vertex shader?
		alpha = clamp(0.4+dist/50, 0, 1);
		clr.rgb *= alpha; // This texture isn't premultiplied
		if (distCamPos<100 && UBOPerformance > 1) {
			float wave1 = snoise(vec2(position.x/3, position.z+time/2.5))/10;
			float wave2 = snoise(vec2(position.x/3+1000, position.z+time/2.5))/10;
			clr.rgb += (wave2*sin(time) + wave1*sin(time+3.14/2)) * (1-distCamPos/100); // Let the water ripples fade out with distance
		}
	}
	blendOutput.rgb = clr.rgb; // The colors are already premultiplied by alpha
	blendOutput.a = alpha;
}
