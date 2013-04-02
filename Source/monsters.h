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

#include <map>
#include <memory>
#include <entityx/Entity.h>
#include <entityx/System.h>

#include "object.h"

class RandomMonster;

namespace View {
	class AnimationModels;
	class HealthBar;
}

namespace Model {

/// @brief Manage other monsters that we know of.
/// @todo The OtherPlayers class is very similar, with too much duplication
class Monsters : public entityx::System<Monsters> {
private:
	/// A convenience map, to make it easy to find the monster entity for the id given by the server
	std::map<unsigned long, entityx::Entity > fEntities;
	entityx::EntityManager *fEntityManager; /// @todo Change this into a reference instead of a pointer
	void Cleanup(void); // Throw away "old" monsters
public:
	Monsters() : fEntityManager(0) {}
	void Init(entityx::EntityManager &em) { fEntityManager = &em; }
	virtual void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt);

	/// Monster information, identified by 'id'. The coordinates are absolute world coordinates.
	void SetMonster(unsigned long id, unsigned char hp, unsigned int level, signed long long x, signed long long y, signed long long z, float dir);
	entityx::Entity Find(unsigned long id) const;
	void RenderMonsters(bool forShadows, const View::AnimationModels *) const; // draw all near monsters
	void RenderMinimap(const glm::mat4 &model, View::HealthBar *hb) const; // draw all near monsters

	/// Find the next monster after 'current', based on distance from player.
	entityx::Entity GetNext(entityx::Entity current) const;

	/// All monsters for a given level has the same size, a value from 1 to 5.
	static float Size(unsigned int level);

	/// Send command to initiate attack on a monster
	static void Attack(entityx::Entity);

	/// Return true if monster is dead
	static bool Dead(entityx::Entity);

	/// Draw a healthbar for a monster
	static void RenderHealthbar(entityx::Entity, View::HealthBar *, float angle);
};
extern std::shared_ptr<Monsters> gMonsters;

}
