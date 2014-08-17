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

	/// Precomputed scattering for RGB
	/// The first index is the parameterized height.
	/// The second index is the parameterized outgoing cosine of the view angle. 1 is upwards, -1 is downwards.
	/// The third index is the parameterized outgoing cosine of the sun angle. 1 is upwards, -1 is downwards.
	glm::vec3 fScattering[NHEIGHT][NVIEW_ANGLE][NSUN_ANGLE]; // Precomputed scattering
	/// Precomputed transmittance for RGB
	/// It will give transmittance for a vector starting at height given by the first index,
	/// pointing in the outgoing direction given by the second index. The vector reach to the end of atmosphere.
	glm::vec3 fTransmittance[NHEIGHT][NVIEW_ANGLE];

	void PreComputeTransmittance();
	glm::vec3 FetchTransmittanceToHorizon(glm::vec2 pa, glm::vec2 v) const;
	glm::vec3 FetchTransmittance(glm::vec2 pa, glm::vec2 pb) const;
	void PreComputeSingleScattering();
	glm::vec3 fetchScattered(float h, float cv, float cs) const;

	/// Compute transmittance
	/// @param pa Starting point
	/// @param pb End point
	/// @return Transmittance for R, G and B
	glm::vec3 Transmittance(glm::vec2 pa, glm::vec2	 pb) const;

	/// Compute the single inscattering
	/// @param pa The player position
	/// @param l Direction from the sun
	/// @param v The direction into 'pa'
	void SingleScattering(glm::vec2 pa, glm::vec2 l, glm::vec2 v, glm::vec3 &mie, glm::vec3 &rayleigh) const;
	glm::vec3 GatheredLight(glm::vec2 p, glm::vec2 v, glm::vec2 l) const;

	bool fInitialized = false;
};
