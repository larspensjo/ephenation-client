// Copyright 2014 The Ephenation Authors
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

#include <cmath>
#include <algorithm>
#include <glm/gtx/intersect.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Atmosphere.h"
#include "Debug.h"

using glm::vec3;
using glm::vec2;

static const float H_Atm = 80000.0f;
static const float R_Earth = 6371*1000;
// As taken from http://www.vendian.org/mncharity/dir3/blackbody/UnstableURLs/bbr_color.html at 5800K and converted from SRGB
static const vec3 sunRGB(1, 0.73694, 0.63461);
// TODO: Sun direction shall not be a constant
static const vec3 sunDir(-0.577350269f, 0.577350269f, 0.577350269f); // Copied from sundir in common.glsl.

Atmosphere::Atmosphere()
{
	//ctor
}

static float HeightParameterized(float h) {
	return std::sqrt(h / H_Atm);
}

static float HeightParameterizedInverse(float uh) {
	return uh*uh*H_Atm;
}


static const float maxHorizontalDist = 1000000.0f;
static const float minHorizontalDist = 100.0f; // Shorter than this has no effect
static const float coeffHor = glm::log(maxHorizontalDist / minHorizontalDist);

static float HorizontalDistParameterized(float x) {
	return glm::log(x/minHorizontalDist) / coeffHor;
}

// Convert from interval [0,1] to [minHorizontalDist, maxHorizontalDist]
static float HorizontalDistParameterizedInverse(float x) {
	return minHorizontalDist * glm::exp(coeffHor * x);
}

static float ViewAngleParameterized(float cv, float h) {
	float ch = - std::sqrt(h * (2 * R_Earth + h)) / (R_Earth + h); // The Angle between the horizon and zenith for the current height
	if (cv > ch)
		return 0.5f * std::pow((cv-ch) / (1-ch), 0.2f) + 0.5f;
	else
		return 0.5f * std::pow((ch-cv) / (ch+1), 0.2f);
}

static float ViewAngleParameterizedInverse(float uv, float h) {
	float ch = - std::sqrt(h * (2 * R_Earth + h)) / (R_Earth + h); // The Angle between the horizon and zenith for the current height
	if (uv > 0.5f)
		return ch + std::pow((uv-0.5f)*2, 5.0f) * (1.0f - ch);
	else
		return ch - std::pow(uv*2, 5.0f) * (1.0f + ch);
}

static float SunAngleParameterization(float cs) {
	// Using radians!
	float tmp = std::tan(1.26f * 1.1f);
	return 0.5f * std::atan(std::max(cs, -0.1975f) * tmp) / 1.1f + (1-0.26f);
}

static float SunAngleParameterizationInverse(float us) {
	// Using radians!
	return std::tan((2*us - 1.0f + 0.26f) * 0.75f) / std::tan(1.26f * 0.75f);
}

static const int INTEGRATION_STEPS = 10;

static const float MieScatterCoefficient = 2.0e-6;
static const float MieExtinctionCoefficient = MieScatterCoefficient / 0.9f;
static const vec3 RayleighScatterCoefficient(6.55e-6, 1.73e-5, 2.30e-5);

static float getDensityRayleigh(vec2 p) {
	float h = glm::length(p);
	return std::exp(-h / 8000);
}

static float getDensityMie(vec2 p) {
	float h = glm::length(p);
	return std::exp(-h / 1200);
}

vec3 Atmosphere::Transmittance(vec3 pa, vec3 pb) const {
	float stepSize = glm::distance(pa, pb) / INTEGRATION_STEPS;
	vec3 dir = glm::normalize(pb-pa);
	float totalDensityMie = 0.0f, totalDensityRayleigh = 0.0f;
	float previousDensityMie = 0.0f, previousDensityReyleigh = 0.0f;
	for (int step=0; step < INTEGRATION_STEPS; ++step) {
		vec3 s = pa + step * stepSize * dir;
		float currentDensityMie = getDensityMie(vec2(s));
		float currentDensityRayleigh = getDensityRayleigh(vec2(s));
		totalDensityMie += (currentDensityMie + previousDensityMie) / 2 * stepSize;
		totalDensityRayleigh += (currentDensityRayleigh + previousDensityReyleigh) / 2 * stepSize;
		previousDensityMie = currentDensityMie;
		previousDensityReyleigh = currentDensityRayleigh;
	}
	return glm::exp(-(totalDensityRayleigh * RayleighScatterCoefficient + totalDensityMie * MieExtinctionCoefficient));
}

void Atmosphere::SingleScattering(vec3 pa, vec3 l, vec3 v, vec3 &mie, vec3 &rayleigh) const {
	// Compute the intersection distance to the point 'pb' where the ray leaves the atmosphere.
	// See figure 4.
	float intersectionDistance;
	vec3 earthCenter(0, -R_Earth, 0); // Height 0 is ground level
	const float atmSquared = (R_Earth+H_Atm) * (R_Earth+H_Atm);
	bool found = glm::intersectRaySphere(pa, -v, earthCenter, atmSquared, intersectionDistance);
	if (!found)
		return;
	float stepSize = intersectionDistance / INTEGRATION_STEPS;
	vec3 totalInscatteringMie, totalInscatteringRayleigh, previousInscatteringMie, previousInscatteringRayleigh;
	for (int step=0; step < INTEGRATION_STEPS; ++step) {
		// 'p' will iterate over the line from 'pa' to 'pb'.
		const vec3 p = pa - stepSize * (step+0.5f) * v; // Step backwards from pa
		vec3 transmittance = Transmittance(pa, p);
		found = glm::intersectRaySphere(p, -l, earthCenter, atmSquared, intersectionDistance);
		const vec3 pc = p - l * intersectionDistance; // Step backwards from p
		// TODO: Use precomputed fTransmittance instead
		transmittance *= Transmittance(p, pc);
		vec3 currentInscatteringMie = getDensityMie(vec2(p)) * transmittance;
		vec3 currentInscatteringRayleigh = getDensityRayleigh(vec2(p)) * transmittance;
		totalInscatteringMie += (currentInscatteringMie + previousInscatteringMie)/2.0f * stepSize;
		totalInscatteringRayleigh += (currentInscatteringRayleigh + previousInscatteringRayleigh)/2.0f * stepSize;
		previousInscatteringMie = currentInscatteringMie;
		previousInscatteringRayleigh = currentInscatteringRayleigh;
	}
	totalInscatteringMie *= MieScatterCoefficient / (4.0f * glm::pi<float>()) * sunRGB;
	totalInscatteringRayleigh *= RayleighScatterCoefficient / (4.0f * glm::pi<float>()) * sunRGB;
	mie = totalInscatteringMie;
	rayleigh = totalInscatteringRayleigh;
}

void Atmosphere::PreComputeTransmittance() {
	const vec3 pb(0,0,0);
	for (int xi = 0; xi < NTRANS_HOR_RES; xi++) {
		float ux = float(xi) / NTRANS_HOR_RES;
		for (int hi = 0; hi < NHEIGHT; hi++) {
			float uh = float(hi) / NHEIGHT;
			vec3 pa(HorizontalDistParameterizedInverse(ux), HeightParameterizedInverse(uh), 0);
			fTransmittance[hi][xi] = this->Transmittance(pa, pb);
		}
	}
}

void Atmosphere::PreComputeSingleScattering() {
	for (int heightIndex = 0; heightIndex < NHEIGHT; heightIndex++) {
		float uHeight = float(heightIndex) / NHEIGHT;
		float h = HeightParameterizedInverse(uHeight);
		vec3 pa(0,h,0);
		for (int viewAngleIndex = 0; viewAngleIndex < NVIEW_ANGLE; viewAngleIndex++) {
			float uViewAngle = float(viewAngleIndex) / NVIEW_ANGLE;
			float cosViewAngle = ViewAngleParameterizedInverse(uViewAngle, h);
			float sinViewAngle = glm::sqrt(1 - cosViewAngle*cosViewAngle); // Pythagoras
			// The view angle is the angle from the azimuth
			vec3 v(sinViewAngle, cosViewAngle, 0); // Pointing to 'pa'
			for (int sunAngleIndex = 0; sunAngleIndex < NSUN_ANGLE; sunAngleIndex++) {
				float uSunAngle = float(sunAngleIndex) / NSUN_ANGLE;
				float cosSunAngle = SunAngleParameterizationInverse(uSunAngle);
				float sinSunAgle = glm::sqrt(1 - cosSunAngle*cosSunAngle);
				// The sun angle is the angle between the azimuth and the sun
				vec3 l(sinSunAgle, cosSunAngle, 0); // Pointing toward 'pa'
				vec3 mie, rayleigh;
				SingleScattering(pa, l, v, mie, rayleigh);
				fScattering[heightIndex][viewAngleIndex][sunAngleIndex] = mie + rayleigh;
			}
		}
	}
}

void Atmosphere::Debug() {
	this->PreComputeTransmittance();
	this->PreComputeSingleScattering();
	vec3 pa(0,0,0);
	for (float i=1; i>=0; i -= 0.15f) {
		vec3 pb(HorizontalDistParameterizedInverse(i), 0, 0);
		vec3 transm = Transmittance(pa, pb);
		LPLOG("Transmittance dist %.0fm: %f, %f, %f", pb.x, transm.r, transm.g, transm.b);
	}

	for (float i=1; i>=0; i -= 0.15f) {
		vec3 pb(100000, HeightParameterizedInverse(i), 0);
		vec3 transm = Transmittance(pa, pb);
		LPLOG("Transmittance height %.0fm: %f, %f, %f", pb.y, transm.r, transm.g, transm.b);
	}

	vec3 v(0,-1,0);
	for (int i=0; i<9; i++) {
		vec3 mie, rayleigh;
		SingleScattering(pa, -sunDir, v, mie, rayleigh);
		LPLOG("Single scattering dir (%f, %f, %f)", v.x, v.y, v.z);
		LPLOG("Mie      (%f, %f, %f)", mie.r, mie.g, mie.b);
		LPLOG("Rayleigh (%f, %f, %f)", rayleigh.r, rayleigh.g, rayleigh.b);
		v = glm::rotateZ(v, 90.0f/8.0f);
	}
}
