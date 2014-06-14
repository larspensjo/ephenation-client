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

#include "Atmosphere.h"
#include "Debug.h"

static const float H_Atm = 80000.0f;
static const float R_Earth = 6371*1000;

Atmosphere::Atmosphere()
{
	//ctor
}

float Atmosphere::HeightParameterized(float h) const {
	return std::sqrt(h);
}

float Atmosphere::ViewAngleParameterized(float cv, float h) const {
	float r = R_Earth / H_Atm; // Normalize
	float ch = - std::sqrt(h * (2 * r + h)) / (r + h); // The Angle between the horizon and zenith for the current height
	if (cv > ch)
		return 0.5f * std::pow((cv-ch) / (1-ch), 0.2f) + 0.5f;
	else
		return 0.5f * std::pow((ch-cv) / (ch+1), 0x2f);
}

float Atmosphere::SunAngleParameterization(float cs) const {
	float tmp = std::tan(1.26f * 1.1f);
	return 0.5f * std::atan(std::max(cs, -0.1975f) * tmp) / 1.1f + (1-0.26f);
}

static const int INTEGRATION_STEPS = 10;

static const float MieScatterCoefficient = 2.0e-6;
static const float MieExtinctionCoefficient = MieScatterCoefficient / 0.9f;
static const glm::vec3 RayleighScatterCoefficient(6.55e-6, 1.73e-5, 2.30e-5);

static float getDensityRayleigh(float h) {
	return std::exp(-h / (8000 / H_Atm));
}

static float getDensityMie(float h) {
	return std::exp(-h / (1200 / H_Atm));
}

glm::vec3 Atmosphere::Transmittance(glm::vec3 pa, glm::vec3 pb) const {
	float stepSize = glm::distance(pa, pb) / INTEGRATION_STEPS;
	glm::vec3 dir = glm::normalize(pb-pa);
	float totalDensityMie = 0.0f, totalDensityRayleigh = 0.0f;
	float previousDensityMie = 0.0f, previousDensityReyleigh = 0.0f;
	for (int step=0; step < INTEGRATION_STEPS; ++step) {
		glm::vec3 s = pa + step * stepSize * dir;
		float h = s.y / H_Atm; // Normalized
		float currentDensityMie = getDensityMie(h);
		float currentDensityRayleigh = getDensityRayleigh(h);
		totalDensityMie += (currentDensityMie + previousDensityMie) / 2 * stepSize;
		totalDensityRayleigh += (currentDensityRayleigh + previousDensityReyleigh) / 2 * stepSize;
		previousDensityMie = currentDensityMie;
		previousDensityReyleigh = currentDensityRayleigh;
	}
	return glm::exp(-(totalDensityRayleigh * RayleighScatterCoefficient + totalDensityMie * MieExtinctionCoefficient));
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
}
