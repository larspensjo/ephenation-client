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
#include <GL/glfw.h>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "object.h"
#include "monsters.h"
#include "chunk.h"
#include "player.h"
#include "primitives.h"
#include "HealthBar.h"
#include "SoundControl.h"
#include "DrawText.h"
#include "manageanimation.h"
#include "Options.h"
#include "animationmodels.h"

using std::stringstream;
using std::shared_ptr;

using namespace Model;

Monsters Model::gMonsters;

struct Monsters::OneMonster : public Object {
	unsigned long id; // The server unique ID of the monster.
	// The coordinate in the server system.
	signed long long x;
	signed long long y;
	signed long long z;
	unsigned int level;
	unsigned char hp;
	unsigned char prevHp; // Used to measure delta hp for presentation
	float fDir; // The direction, in degrees, the monster is facing
	double fUpdateTime; // The last time when this monster was updated from the server.
	double lastTimeMoved; // The time when the monster last moved
	virtual unsigned long GetId() const { return this->id; }
	virtual int GetType() const { return ObjTypeMonster; }
	virtual int GetLevel() const { return this->level; }
	virtual glm::vec3 GetPosition() const; // Get relative coordinate compared to player chunk, in OpenGL coordinates
	virtual glm::vec3 GetSelectionColor() const; // The color to draw on the ground when object selected
	virtual bool IsDead(void) const { return hp == 0; }
	virtual void RenderHealthBar(View::HealthBar *, float angle) const;
	virtual bool InGame(void) const { return true;}
};

glm::vec3 Monsters::OneMonster::GetSelectionColor() const {
	return glm::vec3(0.2f, -0.2f, -0.2f);
}

glm::vec3 Monsters::OneMonster::GetPosition() const {
	ChunkCoord cc;
	gPlayer.GetChunkCoord(&cc);
	float dx = (this->x - (signed long long)cc.x*BLOCK_COORD_RES * CHUNK_SIZE)/float(BLOCK_COORD_RES);
	float dy = (this->y - (signed long long)cc.y*BLOCK_COORD_RES * CHUNK_SIZE)/float(BLOCK_COORD_RES);
	float dz = (this->z - (signed long long)cc.z*BLOCK_COORD_RES * CHUNK_SIZE)/float(BLOCK_COORD_RES);
	return glm::vec3(dx, dz, -dy);
}

void Monsters::SetMonster(unsigned long id, unsigned char hp, unsigned int level, signed long long x, signed long long y, signed long long z, float dir) {
	if (fMonsters.find(id) == fMonsters.end()) {
		fMonsters[id] = std::make_shared<OneMonster>();
	}
	auto mon = fMonsters[id];

	// printf("Store info for monster %d in slot %d (max: %d, fMaxIndex %d)\n", id, i, max, fMaxIndex);
	if (hp != mon->hp)
		mon->prevHp = mon->hp; // New hp value, remember the old
	if (mon->prevHp < hp)
		mon->prevHp = hp;			   // The hp was increasing, only remember old if it was bigger
	if (mon->x != x || mon->y != y || mon->z != z) {
		if (!mon->IsDead()) {
			// TODO: There is a server bug that will make dead monsters move.
			mon->x = x;
			mon->y = y;
			mon->z = z;
			mon->lastTimeMoved = gCurrentFrameTime;
		}
	}
	mon->id = id;
	mon->hp = hp;
	mon->level = level;
	mon->fDir = dir;
	mon->fUpdateTime = gCurrentFrameTime;
}

shared_ptr<const Object> Monsters::Find(unsigned long id) const {
	auto it = fMonsters.find(id);
	if (it == fMonsters.end())
		return nullptr;
	return it->second;
}

void Monsters::RenderMonsters(bool forShadows, bool selectionMode, const View::AnimationModels *animationModels) const {
	float sun = 1.0f;
	if (gPlayer.BelowGround()) {
		// A gross simplification. If underground, disable all sun.
		sun = 0.0f;
	}
	for (const auto &mon : fMonsters) {
		unsigned int level = mon.second->level;
		glm::vec3 pos = mon.second->GetPosition();
		float size = this->Size(level);

		glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
		model = glm::rotate(model, -mon.second->fDir, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::translate(model, glm::vec3(-size/3, 0.0f, size/3));

		View::AnimationModels::AnimationModelId anim;
		// Choose a monster model, depending on the level.
		switch(level % 3) {
		case 2:
			anim = View::AnimationModels::AnimationModelId::Alien;
			break;
		case 1:
			anim = View::AnimationModels::AnimationModelId::Frog;
			break;
		case 0:
			anim = View::AnimationModels::AnimationModelId::Morran;
			break;
		}

		if (mon.second->IsDead())
			animationModels->Draw(anim, model, mon.second->lastTimeMoved, true);
		else if (mon.second->lastTimeMoved + 0.2 > gCurrentFrameTime)
			animationModels->Draw(anim, model, 0.0, false);
		else
			animationModels->Draw(anim, model, gCurrentFrameTime-0.22, false); // Offset in time where model is not in a stride.

		if (forShadows)
			continue;
		if (!gOptions.fDynamicShadows || sun == 0)
			gShadows.Add(pos.x, pos.y, pos.z, size);
	}
}

void Monsters::RenderMinimap(const glm::mat4 &miniMap, View::HealthBar *hb) const {
	for (const auto &mon : fMonsters) {
		if (mon.second->IsDead())
			continue;
		float dx = float(gPlayer.x - mon.second->x)/CHUNK_SIZE/BLOCK_COORD_RES/2;
		float dy = float(gPlayer.y - mon.second->y)/CHUNK_SIZE/BLOCK_COORD_RES/2;
		float dz = float(gPlayer.z - mon.second->z)/CHUNK_SIZE/BLOCK_COORD_RES/2;
		if (dx > 1.0f || dx < -1.0f || dy > 1.0f || dy < -1.0f || dz > 1.0f || dz < -1.0f)
			continue; // To far away on the radar
		// printf("Monsters::RenderMinimap (%.2f, %.2f, %.2f)\n", dx, dy, dz);
		glm::mat4 model = glm::translate(miniMap, glm::vec3(0.5f-dx, 0.5f-dy, 0.0f));
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		hb->DrawSquare(model, 1.0f, 0.4f, 0.4f, 1.0f);
	}
}

void Monsters::OneMonster::RenderHealthBar(View::HealthBar *hb, float angle) const {
	auto pos = this->GetPosition() + glm::vec3(0.0f, 3.0f, 0.0f);
	glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
	model = glm::rotate(model, -angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Need to counter the rotation from the view matrix
	model = glm::scale(model, glm::vec3(2.0f, 0.2f, 1.0f));
	float dmg = (this->prevHp - this->hp)/255.0f;
	hb->DrawHealth(gProjectionMatrix, gViewMatrix * model, this->hp/255.0f, dmg, true);

	glm::vec4 v = glm::vec4(pos, 1.0f);
	v.y += 4.0f; // Just above
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

shared_ptr<const Object> Monsters::GetNext(shared_ptr<const Object> current) const {
	float curr_z = 0.0f;
	if (current)
		curr_z = glm::project(current->GetPosition(), gViewMatrix, gProjectionMatrix, gViewport).z;
	shared_ptr<const Object> best, first;
	float best_z = 1.0f, first_z = 1.0f;
	for (const auto mon : fMonsters) {
		auto mp = mon.second;
		if (current == mp || mon.second->IsDead())
			continue; // No monster at this slot, or it was the one already selected
		// Transform monster coordinates into projection coordinates
		glm::vec3 pos = mon.second->GetPosition();
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
	auto it = fMonsters.begin();
	while (it != fMonsters.end()) {
		if (now - it->second->fUpdateTime < 5.0) {
			it++;
		} else {
			// Remove this monster as a creature in SoundControl
			// printf("Remove monster %d\n", it->second->id);
			gSoundControl.RemoveCreatureSound(SoundControl::SMonster, it->second->id);
			fMonsters.erase(it++);
		}
	}
}

float Monsters::Size(unsigned int level) {
	// This algorithm is the same one used by the server. Do not change it, as the server must have the same
	// knowledge of monster sizes as the client
	unsigned int rnd = (level + 137) * 871; // pseudo random 32-bit number
	float rnd2 = float(rnd & 0xff) / 255.0f; // Random number 0-1
	rnd2 *= rnd2;
	rnd2 *= rnd2;
	rnd2 = 1.0f + rnd2*4.0f; // The monster size will range from 1 blocks to 5 blocks
	return rnd2;
}
