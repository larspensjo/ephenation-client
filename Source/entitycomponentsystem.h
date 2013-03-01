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

#include <entityx/System.h>

namespace Controller {

/// Manage the Entities, Components and System.
///
/// See http://cowboyprogramming.com/2007/01/05/evolve-your-heirachy/ for information.
class EntityComponentSystem
{
public:
	EntityComponentSystem();

	/// Do the actual initialization
	void Init();

	/// @todo Make this private
	entityx::EventManager fEventManager;

	/// @todo Make this private
	entityx::EntityManager fEntityManager;

	/// @todo Make this private
	entityx::SystemManager fSystemManager;
private:
};

/// @todo This should not need be global, except during a period of transition.
extern EntityComponentSystem gEntityComponentSystem;

}
