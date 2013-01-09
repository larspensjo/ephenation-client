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

// ==========================================================================================================
/**
 * @class SkyBox
 *
 * @brief Implementation of the Skybox used in Ephenation.
 *
 * Vertex Shader:
 *      This vertex shader will only draw two triangles, limited to the part of the screen that
 * can be affected.The vertex input is 0,0 in one corner and 1,1 in the other.
 * Draw the quad at z -1, with x and y going from -1 to 1(and then transformed with the model matrix).
 *
 * "pos" is the eye space coordinate of the vertices.
 * "position" is the eye space value, multiplied by a large number. Done so that a large value  is
 *  stored in the G-Buffer.
 * @todo: explain above better.
 *
 * Fragment Shader:
 *      Output to the G-Buffer
 */
 // ==========================================================================================================
class SkyBox : public ShaderBase {
public:
	SkyBox();

	void Init();
	// point.xyz is the world cordinate, adn point.w is the strength of the light.
	void Draw();
private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);

	GLint fModelMatrixIdx;
};
