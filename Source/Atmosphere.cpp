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

static const float H_Atm = 80000.0f;
static const float R_Earth = 6371*1000;
static const glm::vec3 sunRGB(192.0f/255.0f,191.0f/255.0f,173.0f/255.0f); // As taken from http://forums.cgarchitect.com/8108-rgb-colour-sun.html#post242155

Atmosphere::Atmosphere()
{
	//ctor
}

static float HeightParameterized(float h) {
	return std::sqrt(h);
}

static float ViewAngleParameterized(float cv, float h) {
	float ch = - std::sqrt(h * (2 * R_Earth + h)) / (R_Earth + h); // The Angle between the horizon and zenith for the current height
	if (cv > ch)
		return 0.5f * std::pow((cv-ch) / (1-ch), 0.2f) + 0.5f;
	else
		return 0.5f * std::pow((ch-cv) / (ch+1), 0x2f);
}

static float SunAngleParameterization(float cs) {
	float tmp = std::tan(1.26f * 1.1f);
	return 0.5f * std::atan(std::max(cs, -0.1975f) * tmp) / 1.1f + (1-0.26f);
}

static const int INTEGRATION_STEPS = 10;

static const float MieScatterCoefficient = 2.0e-6;
static const float MieExtinctionCoefficient = MieScatterCoefficient / 0.9f;
static const glm::vec3 RayleighScatterCoefficient(6.55e-6, 1.73e-5, 2.30e-5);

static float getDensityRayleigh(float h) {
	return std::exp(-h / 8000);
}

static float getDensityMie(float h) {
	return std::exp(-h / 1200);
}

glm::vec3 Atmosphere::Transmittance(glm::vec3 pa, glm::vec3 pb) const {
	float stepSize = glm::distance(pa, pb) / INTEGRATION_STEPS;
	glm::vec3 dir = glm::normalize(pb-pa);
	float totalDensityMie = 0.0f, totalDensityRayleigh = 0.0f;
	float previousDensityMie = 0.0f, previousDensityReyleigh = 0.0f;
	for (int step=0; step < INTEGRATION_STEPS; ++step) {
		glm::vec3 s = pa + step * stepSize * dir;
		float currentDensityMie = getDensityMie(s.y);
		float currentDensityRayleigh = getDensityRayleigh(s.y);
		totalDensityMie += (currentDensityMie + previousDensityMie) / 2 * stepSize;
		totalDensityRayleigh += (currentDensityRayleigh + previousDensityReyleigh) / 2 * stepSize;
		previousDensityMie = currentDensityMie;
		previousDensityReyleigh = currentDensityRayleigh;
	}
	return glm::exp(-(totalDensityRayleigh * RayleighScatterCoefficient + totalDensityMie * MieExtinctionCoefficient));
}

void Atmosphere::SingleScattering(glm::vec3 pa, glm::vec3 v, glm::vec3 &mie, glm::vec3 &rayleigh) const {
	// Compute the intersection
	const glm::vec3 planeOrig(0, H_Atm, 0);
	const glm::vec3 planeNormal(0, -1.0f, 0);
	// TODO: Sun direction shall not be a constant
	const glm::vec3 sunDir(-0.577350269f, 0.577350269f, 0.577350269f); // Copied from sundir in common.glsl.
	float intersectionDistance;
	glm::intersectRayPlane(pa, -v, planeOrig, planeNormal, intersectionDistance);
	float stepSize = intersectionDistance / INTEGRATION_STEPS;
	glm::vec3 totalInscatteringMie(0,0,0), totalInscatteringRayleigh(0,0,0), previousInscatteringMie(0,0,0), previousInscatteringRayleigh(0,0,0);
	for (int step=0; step < INTEGRATION_STEPS; ++step) {
		const glm::vec3 p = pa + stepSize * (step+0.5f) * (-v);
		glm::vec3 transmittance = Transmittance(pa, p);
		glm::intersectRayPlane(p, sunDir, planeOrig, planeNormal, intersectionDistance);
		const glm::vec3 pc = p + sunDir * intersectionDistance;
		transmittance *= Transmittance(p, pc);
		glm::vec3 currentInscatteringMie = getDensityMie(p.y) * transmittance;
		glm::vec3 currentInscatteringRayleigh = getDensityRayleigh(p.y) * transmittance;
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

void Atmosphere::Debug() {
	glm::vec3 pa(0,0,0);
	glm::vec3 pb(1000, 0, 0);
	for (int i=0; i<5; i++) {
		glm::vec3 transm = Transmittance(pa, pb);
		LPLOG("Transmittance dist %.0fm: %f, %f, %f", pb.x, transm.r, transm.g, transm.b);
		pb.x *= 10.0f;
	}

	pb = glm::vec3(100000, H_Atm, 0);
	for (int i=0; i<6; i++) {
		glm::vec3 transm = Transmittance(pa, pb);
		LPLOG("Transmittance height %.0fm: %f, %f, %f", pb.y, transm.r, transm.g, transm.b);
		pb.y /= 10.0f;
	}

	glm::vec3 v(0,-1,0);
	for (int i=0; i<8; i++) {
		glm::vec3 mie, rayleigh;
		SingleScattering(pa, v, mie, rayleigh);
		LPLOG("Single scattering dir (%f, %f, %f)", v.x, v.y, v.z);
		LPLOG("Mie      (%f, %f, %f)", mie.r, mie.g, mie.b);
		LPLOG("Rayleigh (%f, %f, %f)", rayleigh.r, rayleigh.g, rayleigh.b);
		v = glm::rotateX(v, 90.0f/8.0f);
	}
}
