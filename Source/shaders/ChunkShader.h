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
// A shader program used for drawing chunks.
// No lighting effects etc.
// This is a singleton class.
//

#include "StageOneShader.h"

class ChunkShader : public StageOneShader {
public:
	static ChunkShader *Make(void);

	void Model(const glm::mat4 &);// Define the Model matrix
	void FirstTexture(int ind);
	virtual void TextureOffsetMulti(float offsX, float offsY, float mult); // Override
protected:
	virtual void PreLinkCallback(GLuint prg);

private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);
	ChunkShader(); // Only allow access through the maker.
	virtual ~ChunkShader(); // Don't allow destruction as this is a singleton.
	static ChunkShader fgSingleton; // This is the singleton instance

	GLint fModelMatrixIndex;
	GLint fFirstTextureIndex;
	GLint fTextOffsMultiInd;
};
