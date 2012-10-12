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
// A shader program used for drawing the transparent parts of the chunks.
// Texture 0 has to be the main texture.
//

#include "StageOneShader.h"

class TranspShader : public StageOneShader {
public:
	TranspShader();
	virtual ~TranspShader();
	void Init(void);

	void Model(const glm::mat4 &);// Define the Model matrix
	void View(float time);// The view depends on the time.
	void Projection(float w, float h); // Define the projection matrix

	virtual void DrawingWater(bool);
protected:
	virtual void PreLinkCallback(GLuint prg);

private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);

	GLint fModelMatrixIndex;
	GLint fDepthDependingAlpha;
	GLint fScreenSizeIdx;
	GLint fTimeIdx;
};

extern TranspShader gTranspShader;
