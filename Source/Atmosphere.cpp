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

static const float H_Atm = 80000.0f;
static const float R_Earth = 6371*1000;
static const vec3 sunRGB(192.0f/255.0f,191.0f/255.0f,173.0f/255.0f); // As taken from http://forums.cgarchitect.com/8108-rgb-colour-sun.html#post242155

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
		return 0.5f * std::pow((ch-cv) / (ch+1), 0x2f);
}

static float ViewAngleParameterizedInverse(float uv, float h) {
	float ch = - std::sqrt(h * (2 * R_Earth + h)) / (R_Earth + h); // The Angle between the horizon and zenith for the current height
	if (uv > 0.5f)
		return ch + std::pow(uv-0.5f, 5.0f) * (1.0f - ch);
	else
		return ch - std::pow(uv, 5.0f) * (1.0f + ch);
}

static float SunAngleParameterization(float cs) {
	float tmp = std::tan(1.26f * 1.1f);
	return 0.5f * std::atan(std::max(cs, -0.1975f) * tmp) / 1.1f + (1-0.26f);
}

static float SunAngleParameterizationInverse(float us) {
	return std::tan((2*us - 1.0f + 0.26f) * 0.75f) / std::tan(1.26f * 0.75f);
}

static const int INTEGRATION_STEPS = 10;

static const float MieScatterCoefficient = 2.0e-6;
static const float MieExtinctionCoefficient = MieScatterCoefficient / 0.9f;
static const vec3 RayleighScatterCoefficient(6.55e-6, 1.73e-5, 2.30e-5);

static float getDensityRayleigh(float h) {
	return std::exp(-h / 8000);
}

static float getDensityMie(float h) {
	return std::exp(-h / 1200);
}

vec3 Atmosphere::Transmittance(vec3 pa, vec3 pb) const {
	float stepSize = glm::distance(pa, pb) / INTEGRATION_STEPS;
	vec3 dir = glm::normalize(pb-pa);
	float totalDensityMie = 0.0f, totalDensityRayleigh = 0.0f;
	float previousDensityMie = 0.0f, previousDensityReyleigh = 0.0f;
	for (int step=0; step < INTEGRATION_STEPS; ++step) {
		vec3 s = pa + step * stepSize * dir;
		float currentDensityMie = getDensityMie(s.y);
		float currentDensityRayleigh = getDensityRayleigh(s.y);
		totalDensityMie += (currentDensityMie + previousDensityMie) / 2 * stepSize;
		totalDensityRayleigh += (currentDensityRayleigh + previousDensityReyleigh) / 2 * stepSize;
		previousDensityMie = currentDensityMie;
		previousDensityReyleigh = currentDensityRayleigh;
	}
	return glm::exp(-(totalDensityRayleigh * RayleighScatterCoefficient + totalDensityMie * MieExtinctionCoefficient));
}

void Atmosphere::SingleScattering(vec3 pa, vec3 v, vec3 &mie, vec3 &rayleigh) const {
	// Compute the intersection
	const vec3 planeOrig(0, H_Atm, 0);
	const vec3 planeNormal(0, -1.0f, 0);
	// TODO: Sun direction shall not be a constant
	const vec3 sunDir(-0.577350269f, 0.577350269f, 0.577350269f); // Copied from sundir in common.glsl.
	float intersectionDistance;
	glm::intersectRayPlane(pa, -v, planeOrig, planeNormal, intersectionDistance);
	float stepSize = intersectionDistance / INTEGRATION_STEPS;
	vec3 totalInscatteringMie(0,0,0), totalInscatteringRayleigh(0,0,0), previousInscatteringMie(0,0,0), previousInscatteringRayleigh(0,0,0);
	for (int step=0; step < INTEGRATION_STEPS; ++step) {
		const vec3 p = pa + stepSize * (step+0.5f) * (-v);
		vec3 transmittance = Transmittance(pa, p);
		glm::intersectRayPlane(p, sunDir, planeOrig, planeNormal, intersectionDistance);
		const vec3 pc = p + sunDir * intersectionDistance;
		transmittance *= Transmittance(p, pc);
		vec3 currentInscatteringMie = getDensityMie(p.y) * transmittance;
		vec3 currentInscatteringRayleigh = getDensityRayleigh(p.y) * transmittance;
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
	const glm::vec3 pb(0,0,0);
	for (int xi = 0; xi < NTRANS_HOR_RES; xi++) {
		float ux = float(xi) / NTRANS_HOR_RES;
		for (int hi = 0; hi < NHEIGHT; hi++) {
			float uh = float(hi) / NHEIGHT;
			glm::vec3 pa(HorizontalDistParameterizedInverse(ux), HeightParameterizedInverse(uh), 0);
			fTransmittance[hi][xi] = this->Transmittance(pa, pb);
		}
	}
}

void Atmosphere::Debug() {
	this->PreComputeTransmittance();
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
	for (int i=0; i<8; i++) {
		vec3 mie, rayleigh;
		SingleScattering(pa, v, mie, rayleigh);
		LPLOG("Single scattering dir (%f, %f, %f)", v.x, v.y, v.z);
		LPLOG("Mie      (%f, %f, %f)", mie.r, mie.g, mie.b);
		LPLOG("Rayleigh (%f, %f, %f)", rayleigh.r, rayleigh.g, rayleigh.b);
		v = glm::rotateX(v, 90.0f/8.0f);
	}
}
