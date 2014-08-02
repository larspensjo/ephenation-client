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
#include "primitives.h"

using glm::vec3;
using glm::vec2;

static const float H_Atm = 80000.0f;
static const float R_Earth = 6371*1000;
// As taken RGB from http://www.vendian.org/mncharity/dir3/blackbody/UnstableURLs/bbr_color.html at 5800K
static const vec3 sunRGB(1, 243.0f/255.0f, 234/255.0f);
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

// The maximum horizontal vector, inside the atmosphere
static const float maxHorizontalDist = 2 * glm::sqrt((R_Earth+H_Atm)*(R_Earth+H_Atm) - R_Earth*R_Earth);
static const float minHorizontalDist = 10.0f; // Shorter than this has no effect
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
	return 0.5f * (std::atan(std::max(cs, -0.1975f) * tmp) / 1.1f + (1-0.26f));
}

static float SunAngleParameterizationInverse(float us) {
	// Using radians!
	return std::tan((2*us - 1.0f + 0.26f) * 0.75f) / std::tan(1.26f * 0.75f);
}

// Reference system (0,0,0) is the player at sea level
static float height(vec2 p) {
	p.y += R_Earth; // Now reference is earth center
	float dist = glm::length(p);
	return dist - R_Earth;
}

static const int INTEGRATION_STEPS = 10;

static const float MieScatterCoefficient = 2.0e-6;
static const float MieExtinctionCoefficient = MieScatterCoefficient / 0.9f;
static const vec3 RayleighScatterCoefficient(6.55e-6, 1.73e-5, 2.30e-5);

static float getDensityRayleigh(vec2 p) {
	float h = height(p);
	return std::exp(-h / 8000);
}

static float getDensityMie(vec2 p) {
	float h = height(p);
	return std::exp(-h / 1200);
}

vec3 Atmosphere::Transmittance(vec2 pa, vec2 pb) const {
	float ha = height(pa), hb = height(pb);
	if (ha > hb)
		std::swap(pa, pb); // We want pa at the place with highest density
	float totalDistance = glm::distance(pa, pb);
	float stepSize = totalDistance;
	if (stepSize > 10)
		stepSize = 10;
	vec2 dir = glm::normalize(pb-pa);
	float totalDensityMie = 0.0f, totalDensityRayleigh = 0.0f;
	float previousDensityMie = 0.0f, previousDensityReyleigh = 0.0f;
	float prevDistance = 0;
	for (float distance=stepSize; distance < totalDistance; distance *= 1.05f) {
		vec2 s = pa + distance * dir;
		float currentDensityMie = getDensityMie(vec2(s));
		float currentDensityRayleigh = getDensityRayleigh(vec2(s));
		totalDensityMie += (currentDensityMie + previousDensityMie) / 2 * (distance-prevDistance);
		totalDensityRayleigh += (currentDensityRayleigh + previousDensityReyleigh) / 2 * (distance-prevDistance);
		previousDensityMie = currentDensityMie;
		previousDensityReyleigh = currentDensityRayleigh;
		prevDistance = distance;
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
		if (transmittance.r < 0.00001f && transmittance.g < 0.00001f && transmittance.b < 0.00001f)
			break; // Give it up
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

vec3 Atmosphere::fetchScattered(float h, float cv, float cs) const {
	float uh = HeightParameterized(h);
	float uv = ViewAngleParameterized(cv, h);
	float us = SunAngleParameterization(cs);

	// Round
	int ih = int(uh*(NHEIGHT-1)+0.5f);
	int iv = int(uv*(NVIEW_ANGLE-1)+0.5f);
	int is = int(us*(NSUN_ANGLE-1)+0.5f);
	assert(ih >= 0 && ih < NHEIGHT && iv >= 0 && iv < NVIEW_ANGLE && is >= 0 && is < NSUN_ANGLE);
	// TODO: Interpolation could help?
	return fScattering[ih][iv][is];
}

vec3 Atmosphere::GatheredLight(vec3 p, vec3 v, vec3 l) const {
	vec3 gathered;
	for (float thetaV = 0.0f; thetaV < 2.0f*glm::pi<float>(); thetaV += 2.0f * glm::pi<float>() / INTEGRATION_STEPS) {
		gathered += fetchScattered(height(vec2(p)), glm::cos(thetaV), l.y);
	}
	gathered *= 4.0f * glm::pi<float>() / INTEGRATION_STEPS;
	return gathered;
}

void Atmosphere::PreComputeTransmittance() {
	for (int horIndex = 0; horIndex < NTRANS_HOR_RES; horIndex++) {
		float uHor = float(horIndex) / NTRANS_HOR_RES;
		for (int heightIndex1 = 0; heightIndex1 < NHEIGHT; heightIndex1++) {
			float uh1 = float(heightIndex1) / NHEIGHT;
			float h1 = HeightParameterizedInverse(uh1);
			vec2 pa(0, h1);
			float dist = HorizontalDistParameterizedInverse(uHor);
			vec2 pb(dist, h1);
			fTransmittance[heightIndex1][horIndex] = this->Transmittance(pb, pa);
		}
	}
}

// Find point on line a to b, which is nearest p.
static vec2 GetNearestPoint(vec2 p, vec2 a, vec2 b) {
	vec2 a_to_p = p-a;
	vec2 a_to_b = b-a;

	float l2 = a_to_b.x*a_to_b.x + a_to_b.y*a_to_b.y;
	float atp_dot_atb = glm::dot(a_to_p, a_to_b);

	float dist = atp_dot_atb / l2;

	vec2 ret(a.x + a_to_b.x*dist, a.y + a_to_b.y*dist);

	return ret;
}

vec3 Atmosphere::FetchTransmittance(vec2 pa, vec2 pb) const {
	vec2 earthCenter(0, -R_Earth); // Height 0 is ground level
	vec2 p = GetNearestPoint(earthCenter, pa, pb);
	if (height(p) < 0)
		return Transmittance(pa, pb); // The pre computed table isn't good for this
	auto f = [this](float h, float dist) {
		if (dist < 1.0f)
			return vec3(1,1,1);
		float uh = HeightParameterized(h);
		float ud = HorizontalDistParameterized(dist);
		int ih = int(uh*(NHEIGHT-1)+0.5f);
		int id = int(ud*(NTRANS_HOR_RES-1)+0.5f);
		assert(ih >= 0 && ih < NHEIGHT && id >= 0 && id < NTRANS_HOR_RES);
		return fTransmittance[ih][id];
	};
	float h = height(p);
	vec3 transm1 = f(h, glm::length(pa - p));
	vec3 transm2 = f(h, glm::length(pb - p));
	vec2 PaToP = p - pa;
	vec2 PbToP = p - pb;
	vec2 PaToPb = pb - pa;
	if (glm::dot(PaToP, PaToPb) < 0)
		transm1 = 1.0f / transm1; // Nearest point is outside of pa
	if (glm::dot(PbToP, PaToPb) > 0)
		transm2 = 1.0f / transm2; // Nearest point is outside of pb
	return transm1 * transm2;
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
	this->Init();

	vec3 pa(0,0,0);
	for (float i=1; i>=0; i -= 0.15f) {
		vec2 pb(HorizontalDistParameterizedInverse(i)/2, 0);
		vec3 transm = Transmittance(vec2(pa), pb);
		vec3 transm2 = FetchTransmittance(vec2(pa), vec2(pb));
		LPLOG("Transmittance dist %.0fm: %f, %f, %f (%f %f %f)", pb.x, transm.r, transm.g, transm.b, transm2.r, transm2.g, transm2.b);
	}

	for (float i=1; i>=0; i -= 0.15f) {
		vec2 pb(100000, HeightParameterizedInverse(i));
		vec3 transm = Transmittance(vec2(pa), pb);
		LPLOG("Transmittance height %.0fm: %f, %f, %f", pb.y, transm.r, transm.g, transm.b);
	}

	{
		vec2 pb(1, HeightParameterizedInverse(1));
		vec3 transm = Transmittance(vec2(pa), pb);
		vec3 transm2 = FetchTransmittance(vec2(pa), vec2(pb));
		LPLOG("Transmittance height %f, upwards: %f, %f, %f (%f %f %f)", pb.y, transm.r, transm.g, transm.b, transm2.r, transm2.g, transm2.b);
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

	for (int heightIndex = 0; heightIndex < NHEIGHT; heightIndex += 8) {
		float uHeight = float(heightIndex) / NHEIGHT;
		float h = HeightParameterizedInverse(uHeight);
		LPLOG("Height %f", h);
		for (int viewAngleIndex = 0; viewAngleIndex < NVIEW_ANGLE; viewAngleIndex += 8) {
			float uViewAngle = float(viewAngleIndex) / NVIEW_ANGLE;
			float cosViewAngle = ViewAngleParameterizedInverse(uViewAngle, h);
			float viewAngle = glm::acos(cosViewAngle);
			LPLOG("View angle %f", viewAngle/2/glm::pi<float>()*360);
			for (int sunAngleIndex = 0; sunAngleIndex < NSUN_ANGLE; sunAngleIndex += 4) {
				float uSunAngle = float(sunAngleIndex) / NSUN_ANGLE;
				float cosSunAngle = SunAngleParameterizationInverse(uSunAngle);
				float sunAngle = glm::acos(cosSunAngle);
				LPLOG("[%d][%d][%d] Sun %f: %f %f %f", heightIndex, viewAngleIndex, sunAngleIndex, sunAngle/2/glm::pi<float>()*360, fScattering[heightIndex][viewAngleIndex][sunAngleIndex].r, fScattering[heightIndex][viewAngleIndex][sunAngleIndex].g, fScattering[heightIndex][viewAngleIndex][sunAngleIndex].b);
			}
		}
	}
}
GLuint Atmosphere::LoadTexture() {
	this->Init();

	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_3D, textureId);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F, NHEIGHT, NVIEW_ANGLE, NSUN_ANGLE, 0, GL_RGB, GL_FLOAT, 0); // TODO: No data sent yet
	checkError("Atmosphere::LoadTexture", false);
	return textureId;
}

void Atmosphere::Init() {
	if (fInitialized)
		return;
	this->PreComputeTransmittance();
	this->PreComputeSingleScattering();
	fInitialized = true;
}
