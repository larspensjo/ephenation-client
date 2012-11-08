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

//
// This is a class that manages other monsters that we know of.
//

#include "object.h"

class RandomMonster;
class AnimationModels;

class Monsters {
private:
	enum { MAXMONSTERS=1000 }; // This is the max number of monsters that the player knows about, not the total number in the server
	struct OneMonster : public Object {
		unsigned long id; // The server unique ID of the monster.
		signed long long x;
		signed long long y;
		signed long long z;
		bool ingame;
		bool slotFree; // If this slot is free
		unsigned int level;
		unsigned char hp;
		unsigned char prevHp; // Used to measure delta hp for presentation
		float fDir; // The direction, in degrees, the monster is facing
		double fUpdateTime; // The last time when this monster was updated from the server.
		double lastTimeMoved; // The time when the monster last moved
		virtual unsigned long GetId() const { return this->id; }
		virtual int GetType() const { return ObjTypeMonster; }
		virtual int GetLevel() const { return this->level; }
		virtual glm::vec3 GetPosition() const; // Get relative coordinate compared to player chunk
		virtual glm::vec3 GetSelectionColor() const; // The color to draw on the ground when object selected
		bool IsDead(void) const { return hp == 0; }
		virtual void RenderHealthBar(HealthBar *, float angle) const;
		virtual bool InGame(void) const { return ingame;}
	};
	OneMonster fMonsters[MAXMONSTERS];
	int fMaxIndex; // index+1 of last in game monster
public:
	Monsters();
	void SetMonster(unsigned long id, unsigned char hp, unsigned int level, signed long long x, signed long long y, signed long long z, float dir);
	Object *Find(unsigned long id); // Get a pointer to a monster, or 0 if not found.
	void RenderMonsters(bool forShadows, bool selectionMode, const AnimationModels *animationModels) const; // draw all near monsters
	void RenderMinimap(const glm::mat4 &model, HealthBar *hb) const; // draw all near monsters
	OneMonster *GetSelection(unsigned char R, unsigned char G, unsigned char B);
	void Cleanup(void); // Throw away "old" monsters
	// Find the next monster after 'current', based on distance from player.
	Object *GetNext(Object *current);
	RandomMonster *fRandomMonster;
};

extern Monsters gMonsters;
