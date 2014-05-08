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

#include <glm/gtc/noise.hpp>

#include "Weather.h"
#include "primitives.h"

using namespace Model;

float Weather::GetRain() const {
	glm::vec2 seed(0.0f, float(gCurrentFrameTime/3.0));
	// Very simple model for now.
	return glm::simplex(seed);
}

Weather Weather::sgWeather;
