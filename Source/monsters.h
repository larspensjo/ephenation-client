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
// TODO: The OtherPlayers class is very similar, with too much duplication
//
#include <map>
#include <memory>

#include "object.h"

class RandomMonster;
class AnimationModels;

class Monsters {
private:
	struct OneMonster : public Object {
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
		virtual void RenderHealthBar(HealthBar *, float angle) const;
		virtual bool InGame(void) const { return true;}
	};
	std::map<unsigned long, std::shared_ptr<OneMonster> > fMonsters;
public:
	void Cleanup(void); // Throw away "old" monsters

	// Monster information, identified by 'id'. The coordinates are absolute world coordinates.
	void SetMonster(unsigned long id, unsigned char hp, unsigned int level, signed long long x, signed long long y, signed long long z, float dir);
	std::shared_ptr<const Object> Find(unsigned long id) const; // Get a pointer to a monster, or 0 if not found.
	void RenderMonsters(bool forShadows, bool selectionMode, const AnimationModels *animationModels) const; // draw all near monsters
	void RenderMinimap(const glm::mat4 &model, HealthBar *hb) const; // draw all near monsters
	std::shared_ptr<const Object> GetSelection(unsigned char R, unsigned char G, unsigned char B);

	// Find the next monster after 'current', based on distance from player.
	std::shared_ptr<const Object> GetNext(std::shared_ptr<const Object> current) const;

	// All monsters for a given level has the same size, a value from 1 to 5.
	static float Size(unsigned int level);
};

extern Monsters gMonsters;
