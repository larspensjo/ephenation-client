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


#include "shader.h"

/// A shader that will add a local fog.
///
/// It is done by blending color to the destination color buffer.
class AddLocalFog : public ShaderBase {
public:
	AddLocalFog();

	void Init();

	/// Draw the fog. The world position texture has to be bound on texture unit 1, and
	/// the luminance map on texture unit 0.
	/// @param point The world coordinate in point.xyz, point.w is the radius of the fog.
	void Draw(const glm::vec4 &point);
private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);

	GLint fPointIdx;
};
