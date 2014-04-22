// Copyright 2012-2014 The Ephenation Authors
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

// A shader program used for drawing the transparent parts of the chunks.
// Texture 0 has to be the diffuse texture, and 1 the position texture.
class TranspShader : public StageOneShader {
public:
	virtual ~TranspShader();
	void Init(void);

	void Model(const glm::mat4 &);// Define the Model matrix

	virtual void DrawingWater(bool);
protected:
	virtual void PreLinkCallback(GLuint prg);

private:
	// Callback that defines all uniform and attribute indices.
	virtual void GetLocations(void);

	GLint fModelMatrixIndex = -1;
	GLint fDepthDependingAlpha = -1;
};

extern TranspShader gTranspShader;
