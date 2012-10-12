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

// A class for drawing a texture on a quad.

class SimpleTextureShader;

class DrawTexture {
public:
	static DrawTexture *Make(void); // Make one single DrawTexture, and always return the same.
	// Draw quad texture, transformed with the specified projection and model.
	void Draw(const glm::mat4 &projection, const glm::mat4 &model, float alpha = 1.0f) const;
	// Draw a square texture, multiplying texture coordinates with 'mult' and adding 'offs'
	void DrawOffset(const glm::mat4 &projection, const glm::mat4 &model, float offsX, float offsY, float mult) const;
	// Like DrawOffset, but with depth test enabled.
	void DrawDepth(const glm::mat4 &projection, const glm::mat4 &model, float offsX, float offsY, float mult) const;
	// Draw without changing any uniforms or program
	void DrawBasic(void) const;
private:
	DrawTexture(); // Constructor is private, to make it a singleton class.
	virtual ~DrawTexture();
	void Init(void);
	static DrawTexture fgSingleton;
	SimpleTextureShader *fShader;
	GLuint fBufferId, fVao;
};
