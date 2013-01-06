// Copyright 2012-2013 The Ephenation Authors
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
// It is done by multiplying the float output with a factor approaching almost zero.
// The same factor is used for all 3 channels (RGB).
// The shader can also be used to create a red marker at the feet of a monster. This is
// a similar mechanism, but blending with red is used instead.
//

#include "shader.h"

class AddPointShadow : public ShaderBase {
public:
	AddPointShadow();

	void Init();
	// point.xyz is the world cordinate, adn point.w is the strength of the shadow.
	void DrawPointShadow(const glm::vec4 &point);
	void DrawMonsterSelection(const glm::vec4 &point);
	void DrawRedLight(const glm::vec4 &point);
	void DrawGreenLight(const glm::vec4 &point);
	void DrawBlueLight(const glm::vec4 &point);
private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);

	GLint fPointIdx, fSelectionIdx;

	// Save the mode used for the shader.
	// 0: Point shadow
	// 1: Monster selection marker
	// 2: General red light blended
	// 3: General green light blended
	// 4: General blue light blended
	int fPreviousMode;
};
