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
#include <string>
#include <entityx/System.h>

#include "client_prot.h"

namespace View {
	class HealthBar;
}

class AnimationShader;

namespace Model {

/// @brief Manages other players that we know of.
/// The client will only know of near players, which may need to be drawn.
class OtherPlayers : public entityx::System<OtherPlayers> {
private:
	/// A convenience map, to make it easy to find the monster entity for the id given by the server
	std::map<unsigned long, entityx::Entity::Id > fEntities;
	entityx::EntityManager *fEntityManager; /// @todo Change this into a reference instead of a pointer
	void Cleanup(void);
public:
	OtherPlayers() : fEntityManager(0) {}
	void Init(entityx::EntityManager &em) { fEntityManager = &em; }
	void SetPlayer(unsigned long id, unsigned char hp, unsigned int level, signed long long x, signed long long y, signed long long z, float dir);
	void SetPlayerName(unsigned long uid, const char *name, int adminLevel);
	/// draw all near players.
	/// 'selectionMode' is true when actively selecting players.
	void RenderPlayers(AnimationShader *animShader, bool selectionMode) const;
	/// Render the stats of players. This has to be done after the deferred shader.
	/// 'angle' is the viewing angle, used to draw player data rotated correctly to the camera.
	void RenderPlayerStats(View::HealthBar *hb, float angle) const;
	void RenderMinimap(const glm::mat4 &miniMap, View::HealthBar *hb) const;
	virtual void configure(entityx::EventManager &events) override;
	virtual void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) override;
};

extern boost::shared_ptr<OtherPlayers> gOtherPlayers;

}

