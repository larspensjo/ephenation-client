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

//
// A shader program used for creating shadow maps.
//

#pragma once

#include <memory>

#include "StageOneShader.h"

class ShadowMapShader : public StageOneShader {
public:
	static std::unique_ptr<ShadowMapShader> Make(void);

	void Model(const glm::mat4 &);// Define the Model matrix
	void ProjectionView(const glm::mat4 &);// Define a projectionview matrix
	virtual void TextureOffsetMulti(float offsX, float offsY, float mult); // Override
protected:
	virtual void PreLinkCallback(GLuint prg);

private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);
	ShadowMapShader(); // Only allow access through the maker.

	GLint fProjectionViewMatrixIdx;
	GLint fModelMatrixIndex;
	GLint fTextOffsMultiInd;
};
