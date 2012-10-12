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
// This class encapsulates a shader that will add a local shadow beneath an object.
// It is done by multiplying the float output with the light map.
//

#include "shader.h"

class AddPointShadow : public ShaderBase {
public:
	AddPointShadow();

	void Init();
	// point.xyz is the world cordinate, adn point.w is the strength of the shadow.
	void Draw(const glm::vec4 &point, bool forSelection=false);
private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);

	GLint fPointIdx, fSelectionIdx;
	bool fPreviousForSelection;
};
