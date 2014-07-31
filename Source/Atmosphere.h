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

#pragma once

#include <glm/glm.hpp>
#include <GL/glew.h>

class Atmosphere
{
public:
	Atmosphere();

	void Debug();

	GLuint LoadTexture();

private:
	void Init();

	enum { NHEIGHT=32, NVIEW_ANGLE=64, NSUN_ANGLE=32, NTRANS_HOR_RES = 128};
	glm::vec3 fScattering[NHEIGHT][NVIEW_ANGLE][NSUN_ANGLE];
	glm::vec3 fTransmittance[NHEIGHT][NTRANS_HOR_RES]; // Precomputed transmittance for RGB

	void PreComputeTransmittance();
	void PreComputeSingleScattering();

	/// Compute transmittance
	/// @param pa Starting point
	/// @param pb End point
	/// @return Transmittance for R, G and B
	glm::vec3 Transmittance(glm::vec3 pa, glm::vec3 pb) const;

	/// Compute the single inscattering
	/// @param pa The player position
	/// @param l Direction from the sun
	/// @param v The direction into 'pa'
	void SingleScattering(glm::vec3 pa, glm::vec3 l, glm::vec3 v, glm::vec3 &mie, glm::vec3 &rayleigh) const;
	glm::vec3 GatheredLight(glm::vec3 p, glm::vec3 v, glm::vec3 l) const;
	glm::vec3 fetchScattered(float h, float cv, float cs) const;

	bool fInitialized = false;
};
