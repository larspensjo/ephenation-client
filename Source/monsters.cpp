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

#include <stdio.h>
#include <GL/glew.h>
#include <GL/glfw.h>
#include <string>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "object.h"
#include "client_prot.h"
#include "monsters.h"
#include "render.h"
#include "chunk.h"
#include "player.h"
#include "primitives.h"
#include "MonsterDef.h"
#include "RandomMonster.h"
#include "HealthBar.h"
#include "textures.h"
#include "SoundControl.h"
#include "DrawText.h"
#include "BlenderModel.h"
#include "shaders/AnimationShader.h"
#include "Options.h"
#include "animationmodels.h"

using std::string;
using std::stringstream;

Monsters gMonsters;

Monsters::Monsters() : fMaxIndex(0) {
	for (int i=0; i<MAXMONSTERS; i++) {
		fMonsters[i].slotFree = true;
	}
	fRandomMonster = RandomMonster::Make();
}

glm::vec3 Monsters::OneMonster::GetSelectionColor() const {
	return glm::vec3(0.2f, -0.2f, -0.2f);
}

glm::vec3 Monsters::OneMonster::GetPosition() const {
	ChunkCoord cc;
	gPlayer.GetChunkCoord(&cc);
	float dx = (this->x - (signed long long)cc.x*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dy = (this->y - (signed long long)cc.y*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dz = (this->z - (signed long long)cc.z*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	return glm::vec3(dx, dz, -dy);
}

Monsters::OneMonster *Monsters::GetSelection(unsigned char R, unsigned char G, unsigned char B) {
	return &fMonsters[G*256+B];
}

void Monsters::SetMonster(unsigned long id, unsigned char hp, unsigned int level, signed long long x, signed long long y, signed long long z, float dir) {
	// Find the monster, or an empty slot
	int i;
	int firstFree = -1;
	int max = fMaxIndex+1;
	if (max > MAXMONSTERS)
		max = MAXMONSTERS;
	for (i=0; i < max; i++) {
		if (fMonsters[i].slotFree) {
			if (firstFree == -1)
				firstFree = i;
		} else if (fMonsters[i].id == id) {
			break;
		}
	}

	// Either use the slot found, or the first free slot

	if (i == max) // Id was not already stored
		i = firstFree;

	if (i != MAXMONSTERS) {
		// printf("Store info for player %d in slot %d (max: %d, fMaxIndex %d)\n", id, i, max, fMaxIndex);
		if (hp != fMonsters[i].hp)
			fMonsters[i].prevHp = fMonsters[i].hp; // New hp value, remember the old
		if (fMonsters[i].prevHp < hp)
			fMonsters[i].prevHp = hp;			   // The hp was increasing, only remember old if it was bigger
		fMonsters[i].id = id;
		fMonsters[i].hp = hp;
		if (fMonsters[i].x != x || fMonsters[i].y != y || fMonsters[i].z != z) {
			if (!fMonsters[i].IsDead()) {
				// TODO: There is a server bug that will make dead monsters move.
				fMonsters[i].x = x;
				fMonsters[i].y = y;
				fMonsters[i].z = z;
				fMonsters[i].lastTimeMoved = gCurrentFrameTime;
			}
		}
		fMonsters[i].slotFree = false;
		fMonsters[i].ingame = true;
		fMonsters[i].level = level;
		fMonsters[i].fDir = dir;
		fMonsters[i].fUpdateTime = gCurrentFrameTime;

		if (i == fMaxIndex)
			fMaxIndex = i+1;
	}
}

// This is a linear search that can turn costly if there are any monsters.
Object *Monsters::Find(unsigned long id) {
	for (int i=0; i<MAXMONSTERS; i++) {
		if (fMonsters[i].id == id)
			return &fMonsters[i];
	}
	return 0;
}

void Monsters::RenderMonsters(AnimationShader *shader, bool forShadows, bool selectionMode, const AnimationModels *animationModels) const {
	// The monsters triangles are drawn CW, which is not the default for culling
	float sun = 1.0f;
	if (gPlayer.BelowGround()) {
		// A gross simplification. If underground, disable all sun.
		sun = 0.0f;
	}
	for (int i=0; i<fMaxIndex; i++) {
		if (fMonsters[i].ingame) {
			unsigned int level = fMonsters[i].level;
			glm::vec3 pos = fMonsters[i].GetPosition();
			float size = RandomMonster::Size(level);
			glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
			AnimationModels::AnimationModelId anim;
			// Choose a monster model, depending on the level.
			switch(level % 3) {
			case 2:
				anim = AnimationModels::AnimationModelId::Alien;
				break;
			case 1:
				anim = AnimationModels::AnimationModelId::Frog;
				break;
			case 0:
				anim = AnimationModels::AnimationModelId::Morran;
				break;
			}
			model = glm::rotate(model, -fMonsters[i].fDir, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::translate(model, glm::vec3(-size/3, 0.0f, size/3));

			if (fMonsters[i].IsDead())
				animationModels->Draw(anim, model, fMonsters[i].lastTimeMoved, true);
			else if (fMonsters[i].lastTimeMoved + 0.2 > gCurrentFrameTime)
				animationModels->Draw(anim, model, 0.0, false);
			else
				animationModels->Draw(anim, model, gCurrentFrameTime-0.22, false); // Offset in time where model is not in a stride.

			if (forShadows)
				continue;
			if (!gOptions.fDynamicShadows || sun == 0)
				gShadows.Add(pos.x, pos.y, pos.z, size);
		}
	}
	checkError("Monsters::RenderMonsters");
}

void Monsters::RenderMinimap(const glm::mat4 &miniMap, HealthBar *hb) const {
	for (int i=0; i<fMaxIndex; i++) {
		if (fMonsters[i].ingame && !fMonsters[i].IsDead()) {
			float dx = float(gPlayer.x - fMonsters[i].x)/CHUNK_SIZE/BLOCK_COORD_RES/2;
			float dy = float(gPlayer.y - fMonsters[i].y)/CHUNK_SIZE/BLOCK_COORD_RES/2;
			float dz = float(gPlayer.z - fMonsters[i].z)/CHUNK_SIZE/BLOCK_COORD_RES/2;
			// printf("Monsters::RenderMinimap (%f, %f)\n", dx, dy);
			if (dx > 1.0f || dx < -1.0f || dy > 1.0f || dy < -1.0f || dz > 1.0f || dz < -1.0f)
				continue; // To far away on the radar
			glm::mat4 model = glm::translate(miniMap, glm::vec3(0.5f-dx, 0.5f-dy, 0.0f));
			model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
			// model = glm::rotate(model, 180.0f-fMonsters[i].fDir, glm::vec3(0.0f, 1.0f, 0.0f));
			hb->DrawSquare(model, 1.0f, 0.4f, 0.4f, 1.0f);
		}
	}
}

void Monsters::OneMonster::RenderHealthBar(HealthBar *hb, float angle) const {
	ChunkCoord cc;
	gPlayer.GetChunkCoord(&cc);
	float dx = (this->x - (signed long long)cc.x*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dy = (this->y - (signed long long)cc.y*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dz = (this->z - (signed long long)cc.z*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(dx, dz+3.0f, -dy));
	model = glm::rotate(model, -angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Need to counter the rotation from the view matrix
	model = glm::scale(model, glm::vec3(2.0f, 0.2f, 1.0f));
	float dmg = (this->prevHp - this->hp)/255.0f;
	hb->DrawHealth(gProjectionMatrix, gViewMatrix * model, this->hp/255.0f, dmg, true);

	glm::vec4 v = glm::vec4(dx, dz+4, -dy, 1.0f);
	glm::vec4 screen = gProjectionMatrix * gViewMatrix * v;
	screen = screen / screen.w;
	// printf("Selected: (%f,%f,%f,%f)\n", screen.x, screen.y, screen.z, screen.w);
	if (screen.z > 1)
		return; // Ignore display of Level information for monsters behind the camera
	gDrawFont.Enable();
	gDrawFont.UpdateProjection();
	gDrawFont.SetOffset(gViewport[2] * (screen.x+1)/2, gViewport[3] * (1-screen.y)/2);
	stringstream ss;
	ss << "Level " << this->level;
	glm::vec3 offs(0.0f, -0.8f, -0.8f);
	if (this->level+3 < gPlayer.fLevel)
		offs = glm::vec3(-0.2f, -0.2f, -0.2f); // Gray
	else if (this->level < gPlayer.fLevel+3)
		offs = glm::vec3(-0.8f, -0.0f, -0.8f); // Green
	gDrawFont.vsfl.renderAndDiscard(ss.str(), offs);
}

Object *Monsters::GetNext(Object *current) {
	float curr_z = 0.0f;
	if (current)
		curr_z = glm::project(current->GetPosition(), gViewMatrix, gProjectionMatrix, gViewport).z;
	Object *best = 0, *first = 0;
	float best_z = 1.0f, first_z = 1.0f;
	for (int i=0; i < this->fMaxIndex; i++) {
		OneMonster *mp = &fMonsters[i];
		if (mp->slotFree || current == mp || mp->IsDead())
			continue; // No monster at this slot, or it was the one already selected
		// Transform monster coordinates into projection coordinates
		glm::vec3 pos = mp->GetPosition();
		glm::vec3 screen = glm::project(pos, gViewMatrix, gProjectionMatrix, gViewport);
		if (screen.z < 0.0f || screen.z > 1.0f)
			continue;
		// TODO: This test will actually cut off some monsters that are partly visible.
		if (screen.x < 0.0f || screen.x > gViewport[2] || screen.y < 0.0f || screen.y > gViewport[3])
			continue;
		if (screen.z < first_z) {
			first_z = screen.z;
			first = mp;
		}
		// Found a monster visible in the projection. Check the distance
		if (screen.z < curr_z)
			continue; // Before the current monster
		if (screen.z > best_z)
			continue; // Not as good as best found yet
		best = mp;
		best_z = screen.z;
	}
	// If there was no monster after the current selection, choose the one nearest to the player.
	if (first == 0)
		first = current;
	if (best == 0)
		best = first;
	return best;
}

// The server doesn't explicitely tell the client when a monster shall be removed. The reason for this is
// that the client only gets information about changed monsters that are near enough. The way to go is to
// measure the time that the monster was last updated, and discard the monster after 5s. The server will normally
// only update monster information every time there is a change, but there will also be a complete update at
// least every 4s. So monsters that have not had any update in 5s are stale, and should be removed.
void Monsters::Cleanup(void) {
	double now = gCurrentFrameTime;
	for (int i=0; i<fMaxIndex; i++) {
		if (!fMonsters[i].ingame)
			continue;
		if (now - fMonsters[i].fUpdateTime < 5.0)
			continue;
		// printf("Found stale monster: index %d, id %ld\n", i, fMonsters[i].id);
		fMonsters[i].ingame = false;
		fMonsters[i].slotFree = true;

		// Remove this monster as a creature in SoundControl
		gSoundControl.RemoveCreatureSound(SoundControl::SMonster,fMonsters[i].id);
	}
}
