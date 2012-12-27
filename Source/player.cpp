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

#include <iostream>

#include <GL/glew.h>
#include <GL/glfw.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "render.h"
#include "chunk.h"
#include "player.h"
#include "client_prot.h"
#include "primitives.h"
#include "textures.h"
#include "BlenderModel.h"
#include "modes.h"
#include "shaders/StageOneShader.h"
#include "shaders/AnimationShader.h"
#include "Options.h"

player gPlayer;

#define FACT ((long long)BLOCK_COORD_RES * CHUNK_SIZE)

static int ScaleToChunk(long long x) {
	if (x >= 0)
		return (int)(x / FACT);
	else
		return (int)((x - FACT) / FACT);
}

void player::GetChunkCoord(ChunkCoord *cc) const {
	cc->x = ScaleToChunk(this->x);
	cc->y = ScaleToChunk(this->y);
	cc->z = ScaleToChunk(this->z);
}

glm::vec3 player::GetOffsetToChunk() const {
	ChunkCoord cc;
	GetChunkCoord(&cc);
	unsigned short dx = (unsigned short)(this->x - cc.x*FACT);
	unsigned short dy = (unsigned short)(this->y - cc.y*FACT);
	unsigned short dz = (unsigned short)(this->z - cc.z*FACT);

	// Convert this to OpenGL coordinates
	return glm::vec3(float(dx)/BLOCK_COORD_RES, float(dz)/BLOCK_COORD_RES, -float(dy)/BLOCK_COORD_RES);
}

void player::Draw(AnimationShader *animShader, StageOneShader *staticShader, bool torch, const AnimationModels *animationModels) {
	if (!this->KnownPosition())
		return;
	ChunkCoord cc;
	this->GetChunkCoord(&cc);
	float dx = (this->x - (signed long long)cc.x*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dy = (this->y - (signed long long)cc.y*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dz = (this->z - (signed long long)cc.z*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;

	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(dx, dz-PLAYER_HEIGHT*2.0f, -dy));
	model = glm::rotate(model, -this->fAngleHor, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(PLAYER_HEIGHT, PLAYER_HEIGHT, PLAYER_HEIGHT));

	glBindTexture(GL_TEXTURE_2D, GameTexture::RedColor);
	double tm = gCurrentFrameTime-0.22; // Offset in time where model is not in a stride.
	if (PlayerIsMoving())
		tm = 0.0;
	animShader->EnableProgram();
	gFrog.DrawAnimation(animShader, model, tm, false, 0);
	animShader->DisableProgram();
#if 0
	if (torch)
		gLightSources.Add(dx, dz, -dy, 12.0f);
	else
#endif
    if (gOptions.fDynamicShadows == 0 || this->BelowGround())
        gShadows.Add(dx, dz-PLAYER_HEIGHT*2.0f, -dy, 1.5f);
	if (this->fWeaponType > 0) {
		model = glm::translate(model, glm::vec3(0.6f, PLAYER_HEIGHT*0.4f, -1.2f));
		switch(this->fWeaponType) {
		case 1:
			// model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
			glBindTexture(GL_TEXTURE_2D, GameTexture::WEP1Text);
			break;
		case 2:
			model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
			glBindTexture(GL_TEXTURE_2D, GameTexture::WEP2Text);
			break;
		case 3:
			model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
			glBindTexture(GL_TEXTURE_2D, GameTexture::WEP3Text);
			break;
		case 4:
			model = glm::scale(model, glm::vec3(2.5f, 2.5f, 2.5f));
			glBindTexture(GL_TEXTURE_2D, GameTexture::WEP4Text);
			break;
		}
		staticShader->EnableProgram();
		gSwordModel1.Align(model);
		staticShader->Model(model);
		gSwordModel1.DrawStatic();
		animShader->EnableProgram();
	}
}

glm::vec3 player::GetPosition() const {
	ChunkCoord cc;
	this->GetChunkCoord(&cc);
	float dx = (this->x - (signed long long)cc.x*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dy = (this->y - (signed long long)cc.y*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dz = (this->z - (signed long long)cc.z*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	// Add some corrections to get the coordinate of the feet.
	return glm::vec3(dx, dz-4.0f, -dy);
}

glm::vec3 player::GetSelectionColor() const {
	return glm::vec3(-0.2f, 0.2f, -0.2f);
}

bool player::InGame(void) const { return gMode.Get() == GameMode::GAME; } // Return true if this object is still i the game.

bool player::BelowGround(void) const {
	return gPlayer.z < -10*BLOCK_COORD_RES;
}

void player::SetPosition(signed long long newx, signed long long newy, signed long long newz) {
	fPrevServerPosition = fServerPosition;
	fServerPosition = glm::dvec3(double(newx)/BLOCK_COORD_RES, double(newy)/BLOCK_COORD_RES, double(newz)/BLOCK_COORD_RES);
	fPrevUpdate = fLastUpdate;
	fLastUpdate = glfwGetTime();
	// std::cout << "player::SetPosition at " << fLastUpdate << " delta " << fLastUpdate-fPrevUpdate << std::endl;
	fKnownPosition = true;
}

void player::UpdatePositionSmooth(void) {
	double now = glfwGetTime();
	double projection = now - 0.1; // Set coordinates matching this time stamp, which is a little backward in time
	glm::dvec3 res;

	if (projection < fPrevUpdate) {
		// std::cout << "player::UpdatePositionSmooth using old by " << fPrevUpdate-projection << std::endl;
		res = fPrevServerPosition;
		fMoving = true;
	} else if (projection > fLastUpdate) {
		// std::cout << "player::UpdatePositionSmooth missing updated position" << std::endl;
		res = fServerPosition;
		fMoving = false;
	} else {
		double w = (projection - fPrevUpdate) / (fLastUpdate - fPrevUpdate);
		res = fPrevServerPosition*(1-w) + fServerPosition*w;
		fMoving = true;
		// std::cout << "player::UpdatePositionSmooth interpolating " << w << std::endl;
	}

	this->x = res.x * BLOCK_COORD_RES;
	this->y = res.y * BLOCK_COORD_RES;
	this->z = res.z * BLOCK_COORD_RES+PLAYER_HEIGHT*BLOCK_COORD_RES*2;
}

bool player::PlayerIsMoving(void) const {
	return fMoving;
}
