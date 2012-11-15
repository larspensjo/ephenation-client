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

// This file defines various common functions and definitions used by shaders.

-- UniformBuffer

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
	float UBOexposure;
	float UBOambientLight;
};

-- Poissondisk

float seedpoisson;
uniform sampler1D Upoissondisk; // Name must match RANDOMVEC2POISSON_SAMPLERNAME
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
