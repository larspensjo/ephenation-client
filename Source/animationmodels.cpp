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
#include <GL/glew.h>

#include "animationmodels.h"
#include "textures.h"
#include "shaders/AnimationShader.h"

void AnimationModels::Init() {
	for (unsigned i=0; i < LAST; i++) {
		auto data = new Data;
		data->model.reset(new BlenderModel);
		fModels.push_back(std::unique_ptr<Data>(data));
	}
	// Use the bounds checking index operator for extra safety

	fModels.at(Frog)->model->Init("models/frog.dae", 0.0f, false);
	fModels.at(Frog)->textures.push_back(GameTexture::GreenColor);
	fModels.at(Frog)->textures.push_back(GameTexture::RedScalesId);
	fModels.at(Frog)->textures.push_back(GameTexture::BlueColor);

	fModels.at(Morran)->model->Init("models/morran.dae", 0.0f, false);
	fModels.at(Alien)->textures.push_back(GameTexture::Morran);

	fModels.at(Alien)->model->Init("models/alien.dae", 0.0f, false);
	fModels.at(Alien)->textures.push_back(GameTexture::GreenColor);
	fModels.at(Alien)->textures.push_back(GameTexture::RedScalesId);
	fModels.at(Alien)->textures.push_back(GameTexture::Fur1Id);
	fModels.at(Alien)->textures.push_back(GameTexture::BlueColor);

	fShader = AnimationShader::Make();
}

void AnimationModels::Draw(AnimationModelId id, const glm::mat4 &modelMatrix, double animationStart, bool dead) {
	fModels[id]->model->DrawAnimation(fShader, modelMatrix, animationStart, dead, &fModels[id]->textures[0]);
}
