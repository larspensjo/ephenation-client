// Copyright 2013 The Ephenation Authors
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

/// A shader that will blur a texture
class GaussianBlur : public ShaderBase {
public:
	GaussianBlur();

	void Init();

	/// Apply blurring on horisontal pixels.
	/// This will leave the program active for the next phase.
	/// @param tap Number of taps. Should be 5, 7, 9...
	/// @param sigma The sigma value used in the Gaussian blur
	void BlurHorizontal(int tap, float sigma) const;

	/// Apply blurring on vetical pixels. Use the same 'tap' and 'sigma' as for the horizontal blur.
	/// The program is expected to be open from the previous phase.
	void BlurVertical() const;
private:

	/** @brief Callback that defines all uniform and attribute indices.
	 */
	virtual void GetLocations(void);

	GLint fSigmaIdx, fNumBlurPixelsPerSideIdx, fBlurMultiplyVecIdx;
};
