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

class Atmosphere
{
public:
	Atmosphere();

	void Debug();
private:
	enum { NHEIGHT=32, NVIEW_ANGLE=64, NSUN_ANGLE=32};
	glm::vec3 fScattering[NHEIGHT][NVIEW_ANGLE][NSUN_ANGLE];

	float HeightParameterized(float h) const;
	float ViewAngleParameterized(float cv, float h) const;
	float SunAngleParameterization(float cs) const;

	glm::vec3 Transmittance(glm::vec3 pa, glm::vec3 pb) const;
};
