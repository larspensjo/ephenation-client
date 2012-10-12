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

#include <glm/glm.hpp>

#include <list>
#include <vector>

class StageOneShader;

class Billboard {
public:
	enum Predefined { tree1, tree1Shadow, tree2, tree2Shadow, tree3, tree3Shadow, numElements };;
	Billboard(int pixelWidth, int numPictures); ~Billboard();
	// Initialize a big bitmap with 'numPictures' x 'numPictures'. Each picture will be a square
	// of the size 'pixelWidth' x 'pixelWidth'.
	void Init(void);

	// Permanently allocate a billboard picture. Return -1 for failure.
	int AllocatePermanentPicture(void);
	void FreePermanentPicture(int);

	// Enable the FBO for drawing into the temporary texture
	void EnableForDrawing(void);
	// Copy the resut into the billboard atlas and disable the FBO.
	void UppdateBillboard(int picture);

	// Bind the texture to GL_TEXTURE_2D
	void BindTexture(void);

	void InitializeTextures(StageOneShader *);

	void Draw(StageOneShader *shader, const glm::mat4 &modelMatrix, enum Predefined picture);
private:
	GLuint fboAtlas, fboTemp;
	GLuint fAtlasId; // The texture of the complete atlas
	GLuint fTempTextureId; // Temporary texture to draw into
	GLuint fDepthBuffer;
	int fPixelWidth, fNumPictures;

	std::list<int> fFreePictures;

	std::vector<int> fPredefined;
};

extern Billboard gBillboard;
