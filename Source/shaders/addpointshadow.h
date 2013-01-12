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

/**
 * @brief A shader that will add a local shadow or other lighting effects
 *
 * Proper blending mode has to be configured.
 * It is done by multiplying the float output with a factor approaching almost zero.
 * The same factor is used for all 3 channels (RGB).
 * The shader can also be used to create a red marker at the feet of a monster or lighting effects.
 * This is a similar mechanism, but blending with red is used instead.
 */
class AddPointShadow : public ShaderBase {
public:
	AddPointShadow();

	void Init();

	/** @brief Draw a dark circular point shadow at the feet of a monster.
	 *  @param point.xyz The the world cordinate.
	 *  @param point.w The radius of the shadow.
	 *
	 * The shadow will decrease with the distance.
	 */
	void DrawPointShadow(const glm::vec4 &point);

	/** @brief Draw a red circular point shadow at the feet of a monster.
	 *  @param point.xyz The the world cordinate.
	 *  @param point.w The radius of the shadow.
	 */
	void DrawMonsterSelection(const glm::vec4 &point);

	/** @brief Add a sphere of red light effect.
	 *  @param point.xyz The the world cordinate.
	 *  @param point.w The radius of the sphere.
	 */
	void DrawRedLight(const glm::vec4 &point);

	/** @brief Add a sphere of green light effect.
	 *  @param point.xyz The the world cordinate.
	 *  @param point.w The radius of the sphere.
	 */
	void DrawGreenLight(const glm::vec4 &point);

	/** @brief Add a sphere of blue light effect.
	 *  @param point.xyz The the world cordinate.
	 *  @param point.w The radius of the sphere.
	 */
	void DrawBlueLight(const glm::vec4 &point);
private:

	/** @brief Callback that defines all uniform and attribute indices.
	 */
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
