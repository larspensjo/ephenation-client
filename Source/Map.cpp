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

#include "HudTransformation.h"
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

void Map::Create(AnimationShader *anim, StageOneShader *shader, float rotate, int width, const AnimationModels *animationModels, bool stereoView) {
	glm::mat4 saveView = gViewMatrix;
	gViewMatrix = glm::mat4(1);
	gViewMatrix = glm::rotate(gViewMatrix, -90.0f, glm::vec3(1.0f, 0.0f, 0.0f)); // Looking downwards
	gViewMatrix = glm::rotate(gViewMatrix, rotate, glm::vec3(0.0f, 1.0f, 0.0f));
	gViewMatrix = glm::translate(gViewMatrix, -Model::gPlayer.GetOffsetToChunk());          // Adjust for player position

	glm::mat4 saveProj = gProjectionMatrix;
	// TODO: Why does the 'y' need to be mirrored? If not, triangles will be drawn in the wrong order, and culling of faces will get wrong.
	gProjectionMatrix = glm::scale(glm::mat4(1), glm::vec3(2.0f/width, -2.0f/width, 2/64.0f));
	auto saveMaxDistance = maxRenderDistance;
	maxRenderDistance = width/2*1.414; // Temporary override, all the way out to the corners.

	gUniformBuffer.Update(stereoView);

	DrawLandscapeTopDown(shader, width, 64, false, DL_NoTransparent);
	DrawLandscapeTopDown(shader, width, 64, false, DL_OnlyTransparent);
	anim->EnableProgram();
	Model::gPlayer.Draw(anim, shader, false, animationModels);
	Model::gMonsters.RenderMonsters(false, false, animationModels);
	Model::gOtherPlayers.RenderPlayers(anim, false);

	gProjectionMatrix = saveProj;
	gViewMatrix = saveView;
	gUniformBuffer.Update(stereoView);
	maxRenderDistance = saveMaxDistance;
}

void Map::Draw(float alpha, bool stereoView) const {
	glm::mat4 proj(1);
	glm::mat4 model(1);
	if (stereoView) {
		proj = gProjectionMatrix;
		glm::mat4 center = glm::translate(glm::mat4(1), glm::vec3(-0.5, -0.5f, 0.0f));
		glm::mat4 scaleToMeter = glm::scale(glm::mat4(1), glm::vec3(3.0f, 3.0f, 3.0f));
		glm::mat4 makeHorizontal = glm::rotate(glm::mat4(1), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 transl = glm::translate(glm::mat4(1), glm::vec3(0.0f, -2.0f, -3.0f));
		model = View::gHudTransformation.GetViewTransform() * (transl * makeHorizontal * scaleToMeter * center);
	} else {
		float screenRatio = gViewport[2] / gViewport[3];
		model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(2.0f/screenRatio, 2.0f, 1.0f));
	}
	DrawTexture::Make()->Draw(proj, model, alpha);
	glBindTexture(GL_TEXTURE_2D, 0);
}
