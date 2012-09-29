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

#include <string>

#include "vsfl/vsfl.h"

class SimpleTextureShader;

class DrawFont {
public:
	void UpdateProjection(void);	// Use the view port to update the projection.
	void SetOffset(float row, float col);	// Control the screen offset where to draw next string
	void Enable(void);					// Prepare for drawing text. This will change the shader program.
	void Disable(void);					// Must be called after text drawing done.
	VSFLFont vsfl;
	DrawFont();
	~DrawFont();
	void Init(const std::string &fontName);				// Call once, sometimes after OpenGL is enabled.
private:
	bool fProjectionMatrixNeedUpdate;
	SimpleTextureShader *fShader;
	bool fShaderEnabled;
};

extern DrawFont gDrawFont;
