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

-- UniformBuffer

// This list must match the struct defined in Uniform.cpp.
layout(std140) uniform GlobalData {
	mat4 UBOProjectionMatrix;
	mat4 UBOProjectionviewMatrix;
	mat4 UBOViewMatrix;
	vec4 UBOCamera;
	float UBOViewingDistance;
	int UBOPerformance;
	int UBODynamicshadows;
	int UBOWindowHeight;
	int UBOToggleTesting;
	int UBOBelowGround;
	float UBOexposure;
	float UBOambientLight;
};

-- Poissondisk

float seedpoisson;
uniform sampler1D Upoissondisk;
vec2 rand2(vec2 n)
{
	seedpoisson = fract(seedpoisson + sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
	return 2.0*texture(Upoissondisk, seedpoisson).rg-1.0;
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

/// Find corner from quad that covers a light effect.
/// @param c The sphere center in model space where the light is located.
/// @param r Sphere radius.
/// @param v One corner from the quad (-1,-1) - (1,1).
/// @return The corner in screen space.
vec4 GetTileFromSphere(vec3 c, float r, vec2 v) {
	vec2 quadOffset =  v*r; // Compute offset to be used from sphere center, scaling with radius
	vec4 viewPos = UBOViewMatrix * vec4(c, 1); // Transform sphere model center to view coordinates

	// The vector "viewpos" shall now be moved, either forward or backward.
	// The objective is that the resulting quad shall always cover the sphere,
	// from the camera point of view. If it would not have been moved, ground in front
	// would conceal light effects.
	// Find the unit vector 'u' pointing to "viewPos" from the camera.
	vec3 u = normalize(viewPos.xyz);
	// The distance to the sphere center from the camera. Negative values means beyond the camera.
	float d = length(viewPos.xyz)*sign(-viewPos.z);
	// Find the adjustment to move the quad towards the camera. There are four cases
	// of the camera position that have to be handled:
	// 1. In front of the sphere and outside.
	// 2. Inside the sphere, but in front of the sphere center.
	// 3. Inside the sphere, but beyond the sphere center.
	// 4. Beyond the sphere, and completely outside.
	float delta;              // How far to move the vertices of the quad towards the camera
	bool inside = false;      // Force full screen
	if (d > r)                // Case 1
		delta = r;
	else if (d > 0 && d < r)  // Case 2
		inside = true;
	else if (d < 0 && -d < r) // Case 3
		inside = false;
	else                      // Case 4, the quad would be drawn behind the camera, and thus culled.
		delta = d;

	// The modelView is one of the vertices of the quad in view space.
	vec4 modelView = viewPos - vec4(u, 0)*delta + vec4(quadOffset, 0, 0);
	vec4 pos = UBOProjectionMatrix * modelView;
	if (inside)
		pos = vec4(v, 0, 1); // Override with the full screen normalized coordinate
	pos /= pos.w;
	return pos;
}
