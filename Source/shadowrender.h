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

#include <memory>
#include <GL/glew.h>

#include <glm/glm.hpp>

class ShadowMapShader;
class FBOFlat;

namespace View {
	class AnimationModels;

/// Create a shadowmap (depth texture).
class ShadowRender {
public:
	/// Initialize a shadow map of specified bitmap size
	ShadowRender(int width, int height);
	virtual ~ShadowRender();

	void Init();

	/// Render the shadow map, givne the specified world volume (specified in blocks).
	/// Blocks horisontal from -width/2 to +width/2 will be included, and -height/2 to +height/2.
	void Render(int width, int height, const AnimationModels *animationModels);

	/// Bind the shadowmap texture to the currently active texture unit.
	void BindTexture(void) const;

	const glm::mat4 &GetProViewMatrix(void) const;

private:
	GLuint fTexture; // Texture id used for depth buffer
	int fMapWidth, fMapHeight; // Size of the shadow map
	std::unique_ptr<ShadowMapShader> fShader;
	std::unique_ptr<FBOFlat> fbo;
	glm::mat4 fProjViewMatrix; // The matrix is needed elsewhere, save it for later
};

}
