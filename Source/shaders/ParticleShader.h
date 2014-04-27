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

#include "StageOneShader.h"

/// A shader program used for drawing chunks.
/// No lighting effects etc.
class ParticleShader : public StageOneShader {
public:
	/// Make an instance
	static ParticleShader *Make(void);

	/// Define the Model matrix
	void Model(const glm::mat4 &);

	/// Configure the behaviour
	/// @param timeScaling Thefactor time is scaled down.
	/// @param distribute How big area to distribute the effects.
	/// @param fallingSpeed Let particles fall, and reappear
	void Configure(float timeScaling, float distribute, float fallingSpeed = 0.0f);

	virtual void TextureOffsetMulti(float offsX, float offsY, float mult); // Override
protected:
	virtual void PreLinkCallback(GLuint prg);

private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);
	static ParticleShader fgSingleton; // This is the singleton instance

	GLint fModelMatrixIndex = -1;
	GLint fTextOffsMultiInd = -1;
	GLint fTimeScaling = -1;
	GLint fDistribute = -1;
	GLint fFallingSpeed = -1;
};
