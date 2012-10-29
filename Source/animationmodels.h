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
// Administrate animation models.
//

#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include "BlenderModel.h"

class AnimationShader;

class AnimationModels {
public:
	void Init(void);
	enum AnimationModelId {
		Frog, Morran, Alien,
		LAST // This one must always come last
	};

	void Draw(AnimationModelId id, const glm::mat4 &modelMatrix, double animationStart, bool dead);
private:

	struct Data { // TODO: Shouldn't it be enough with a forward declaration of this struct?
		std::unique_ptr<BlenderModel> model;
		std::vector<GLuint> textures; // The textures to use for each mesh
	};
	std::vector<std::unique_ptr<Data> > fModels;

	AnimationShader *fShader; // Singleton, do not destroy
};
