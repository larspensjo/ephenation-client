// Copyright 2012-2013 The Ephenation Authors
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

// This file defines various common functions and definitions used by shaders.
// Defined names have global scope, so use unique identifiers to minimize collisions.

-- UniformBuffer

// This list must match the struct defined in Uniform.cpp.
layout(std140) uniform GlobalData {
	mat4 UBOProjectionMatrix;
	mat4 UBOProjectionviewMatrix;
	mat4 UBOViewMatrix;
	vec4 UBOCamera;
	vec4 UBOOVRDistortion;
	vec2 UBOLensCenter;
	float UBOViewingDistance;
	float UBOTime;
	int UBOPerformance;
	int UBODynamicshadows;
	int UBOWindowHeight;
	int UBOWindowWidth;
	int UBOToggleTesting;
	int UBOBelowGround;
	float UBOexposure;
	float UBOambientLight;
	float UBOcalibrationFactor;
	float UBOProjK1, UBOProjK2;
	int UBOEnableDistortion;
};

// Given a depth texture, compute the distance to a pixel
float WorldDistance(sampler2D depthSampler, vec2 screen) {
	float depth = texture(depthSampler, screen).r * 2.0 - 1.0;
	return UBOProjK1 / (UBOProjK2 - depth);
}

-- OvrDistortion

// Apply distortion effect to a coordinate. It has to be centered at 0,0, and range from -1 to +1.
vec2 HmdWarp(vec2 in01)
{
	vec2 theta = in01 - UBOLensCenter;
	float rSq = theta.x * theta.x + theta.y * theta.y;
	vec2 rvector = theta * (UBOOVRDistortion.x +
							UBOOVRDistortion.y * rSq +
							UBOOVRDistortion.z * rSq * rSq +
							UBOOVRDistortion.w * rSq * rSq * rSq
			);
	return rvector + UBOLensCenter;
}

-- DoubleResolutionFunction

// A shader transformation function.
// k: Will define the change rate of the resolution. k/2 will be the rate near the player.
// A higher value will give better near shadows, but a quicker fall towards 0.
// The Logistics function is calibrated so as to give f(0)=0 and f(1) = 1.

float LogisticFunction(float x) {
	const float k = 12.0;
	return 2.0/(1.0+exp(-k*x))-1;
}
vec2 DoubleResolution(vec2 coord) {
	vec2 ret;
	if (coord.x >= 0.0) ret.x = LogisticFunction(coord.x);
	else ret.x = -LogisticFunction(-coord.x);
	if (coord.y >= 0.0) ret.y = LogisticFunction(coord.y);
	else ret.y = -LogisticFunction(-coord.y);
	return ret;
}

-- SunDirection

const vec3 sundir = vec3(-0.577350269, 0.577350269, 0.577350269);

-- DistanceAlphaBlending

/// Distant object shall be blended into the sky/fog distance.
/// @param maxViewDistance Distance where objects are culled.
/// @param currentViewDistance Distance from camera to pixel.
/// @return The alpha used for blending. 1 at near distance, 0 at far.
float DistanceAlphaBlending(float maxViewDistance, float currentViewDistance) {
	// The fog will go from transparent to opaque in "fogDepth" blocks.
	float fogDepth = maxViewDistance/5;

	// Compute the horizontal fog gradient.
	float alpha = clamp(maxViewDistance-currentViewDistance, 0, fogDepth)/fogDepth;
	return alpha;
}

-- GetTileFromSphere

/// Find corner from octagon that covers a light effect.
/// @param c The sphere center in model space where the light is located.
/// @param r Sphere radius.
/// @param v One corner from the octagon (-1,-1) - (1,1).
/// @return The corner in screen space.
vec4 GetTileFromSphere(vec3 c, float r, vec2 v) {
	vec4 viewPos = UBOViewMatrix * vec4(c, 1); // Transform sphere model center to view coordinates

	// The vector "viewpos" shall now be moved, either forward or backward.
	// The objective is that the resulting octagon shall always cover the sphere,
	// from the camera point of view. If it would not have been moved, ground in front
	// would conceal light effects.
	// Find the unit vector 'u' pointing to "viewPos" from the camera.
	vec3 u = normalize(viewPos.xyz);
	// The distance to the sphere center from the camera. Negative values means beyond the camera.
	float d = length(viewPos.xyz)*sign(-viewPos.z);
	// Find the adjustment to move the octagon towards the camera. There are four cases
	// of the camera position that have to be handled:
	// 1. In front of the sphere and outside.
	// 2. Inside the sphere, but in front of the sphere center.
	// 3. Inside the sphere, but beyond the sphere center.
	// 4. Beyond the sphere, and completely outside.
	float delta;              // How far to move the vertices of the octagon towards the camera
	bool inside = false;      // Force full screen
	if (d > r)                // Case 1
		delta = r;
	else if (d > 0 && d < r)  // Case 2
		inside = true;
	else if (d < 0 && -d < r) // Case 3
		inside = true;
	else                      // Case 4, the octagon would be drawn behind the camera, and thus culled.
		delta = d;

	// Compute offset to be used from sphere center, scaling with radius.
	// Also scale with perspective, the octagon in front of the light can be a little
	// smaller than the light radius.
	vec2 vertexOffset =  v*r*(d-r)/sqrt(d*d-r*r);
	// The modelView is one of the vertices of the octagon in view space.
	vec4 modelView = viewPos - vec4(u, 0)*delta + vec4(vertexOffset, 0, 0);
	vec4 pos = UBOProjectionMatrix * modelView;
	if (inside)
		pos = vec4(v, 0, 1); // Override with the full screen normalized coordinate
	pos /= pos.w;
	return pos;
}

-- SimplexNoise

vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }
/// An implementation of a 2D simplex noise
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

-- PoissonDisk

// This table is from http://asawicki.info/Download/Productions/Applications/PoissonDiscGenerator/2D.txt
const vec2 gPoissonDisk[64] = vec2[] (
	vec2( 0.282571, 0.023957 ),
	vec2( 0.792657, 0.945738 ),
	vec2( 0.922361, 0.411756 ),
	vec2( 0.165838, 0.552995 ),
	vec2( 0.566027, 0.216651 ),
	vec2( 0.335398, 0.783654 ),
	vec2( 0.0190741, 0.318522 ),
	vec2( 0.647572, 0.581896 ),
	vec2( 0.916288, 0.0120243 ),
	vec2( 0.0278329, 0.866634 ),
	vec2( 0.398053, 0.4214 ),
	vec2( 0.00289926, 0.051149 ),
	vec2( 0.517624, 0.989044 ),
	vec2( 0.963744, 0.719901 ),
	vec2( 0.76867, 0.018128 ),
	vec2( 0.684194, 0.167302 ),
	vec2( 0.727103, 0.410871 ),
	vec2( 0.557482, 0.724143 ),
	vec2( 0.483352, 0.0527055 ),
	vec2( 0.162877, 0.351482 ),
	vec2( 0.959716, 0.180578 ),
	vec2( 0.140355, 0.112003 ),
	vec2( 0.796228, 0.223365 ),
	vec2( 0.187048, 0.787225 ),
	vec2( 0.55446, 0.35612 ),
	vec2( 0.449965, 0.640522 ),
	vec2( 0.438917, 0.194769 ),
	vec2( 0.791253, 0.565325 ),
	vec2( 0.719718, 0.794794 ),
	vec2( 0.0651875, 0.708609 ),
	vec2( 0.641987, 0.0233772 ),
	vec2( 0.376415, 0.944243 ),
	vec2( 0.827723, 0.723258 ),
	vec2( 0.968627, 0.884518 ),
	vec2( 0.263405, 0.458968 ),
	vec2( 0.985717, 0.559587 ),
	vec2( 0.0616169, 0.468612 ),
	vec2( 0.159154, 0.934782 ),
	vec2( 0.287301, 0.284768 ),
	vec2( 0.550066, 0.849391 ),
	vec2( 0.353587, 0.003296 ),
	vec2( 0.000671407, 0.582507 ),
	vec2( 0.850459, 0.461989 ),
	vec2( 0.526139, 0.640126 ),
	vec2( 0.786889, 0.487686 ),
	vec2( 0.164129, 0.02472 ),
	vec2( 0.517075, 0.90933 ),
	vec2( 0.316111, 0.663564 ),
	vec2( 0.09476, 0.895749 ),
	vec2( 0.298288, 0.195318 ),
	vec2( 0.427229, 0.7828 ),
	vec2( 0.734764, 0.266152 ),
	vec2( 0.0816065, 0.965972 ),
	vec2( 0.698935, 0.646352 ),
	vec2( 0.281899, 0.355144 ),
	vec2( 0.871334, 0.303171 ),
	vec2( 0.138249, 0.661214 ),
	vec2( 0.202399, 0.252449 ),
	vec2( 0.0734275, 0.399853 ),
	vec2( 0.786767, 0.660268 ),
	vec2( 0.933744, 0.508621 ),
	vec2( 0.398236, 0.0509049 ),
	vec2( 0.500473, 0.130253 ),
	vec2( 0.0332957, 0.526292)
);
