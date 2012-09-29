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
//

#pragma once

//
// This class encapsulates a shader that will add lighting from a point light.
//

#include "shader.h"

class AddPointLight : public ShaderBase {
public:
	AddPointLight();

	void Init();
	// point.xyz is the world cordinate, adn point.w is the strength of the light.
	void Draw(const glm::vec3 &pos, float strength);
private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);

	GLint fLampIdx;
};
