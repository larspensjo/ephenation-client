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

#include <GL/glew.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include <glm/glm.hpp>
#include "player.h"
#include "Map.h"
#include "render.h"
#include "chunk.h"
#include "imageloader.h"
#include "textures.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "DrawTexture.h"
#include "primitives.h"
#include "uniformbuffer.h"
#include "shaders/AnimationShader.h"
#include "monsters.h"
#include "otherplayers.h"

#define D 8 // How many chunks beside the current chunk to include

using namespace View;

Map::Map() {
}

Map::~Map() {
}

void Map::Create(AnimationShader *anim, StageOneShader *shader, float rotate, int width, const AnimationModels *animationModels) {
	gViewMatrix = glm::mat4(1);
	gViewMatrix = glm::rotate(gViewMatrix, -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)); // Looking downwards
	gViewMatrix = glm::rotate(gViewMatrix, rotate, glm::vec3(0.0f, 1.0f, 0.0f));
	gViewMatrix = glm::translate(gViewMatrix, -Model::gPlayer.GetOffsetToChunk());          // Adjust for player position

	glm::mat4 saveProj = gProjectionMatrix;
	// TODO: Why does the 'y' need to be mirrored? If not, triangles will be drawn in the wrong order, and culling of faces will get wrong.
	gProjectionMatrix = glm::scale(glm::mat4(1), glm::vec3(2.0f/width, -2.0f/width, 2/64.0f));
	auto saveMaxDistance = maxRenderDistance;
	maxRenderDistance = width/2*1.414; // Temporary override, all the way out to the corners.

	gUniformBuffer.Update();

	DrawLandscapeTopDown(shader, width, 64, false, DL_NoTransparent);
	DrawLandscapeTopDown(shader, width, 64, false, DL_OnlyTransparent);
	anim->EnableProgram();
	Model::gPlayer.Draw(anim, shader, false, animationModels);
	Model::gMonsters.RenderMonsters(false, false, animationModels);
	Model::gOtherPlayers->RenderPlayers(anim, false);

	gProjectionMatrix = saveProj;
	maxRenderDistance = saveMaxDistance;
	gUniformBuffer.Update();
}

void Map::Draw(float alpha) const {
	float screenRatio = gViewport[2] / gViewport[3];
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(2.0f/screenRatio, 2.0f, 1.0f));
	static const glm::mat4 projection(1.0f); // No need to create a new one every time.
	auto texture = DrawTexture::Make();
	texture->Draw(projection, model, alpha);
	glBindTexture(GL_TEXTURE_2D, 0);
}
