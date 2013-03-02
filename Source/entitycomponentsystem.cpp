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
#include <GL/glfw.h>
#include <glm/glm.hpp>

#include "object.h"
#include "entitycomponentsystem.h"
#include "ScrollingMessages.h"
#include "Inventory.h"
#include "otherplayers.h"

using namespace Controller;

EntityComponentSystem::EntityComponentSystem() : fEntityManager(fEventManager), fSystemManager(fEntityManager, fEventManager)
{
}

void EntityComponentSystem::Init() {
	auto sm = boost::make_shared<View::ScrollingMessages>(fEntityManager);
	fSystemManager.add(sm);
	fSystemManager.add(gInventory);
	fSystemManager.add(Model::gOtherPlayers);

	// After all systems have been added, configure them.
	fSystemManager.configure();
}

void EntityComponentSystem::Update() {
	static double last = 0.0;
	double now = glfwGetTime();
	double dt = now - last;
	last = now;
	fSystemManager.update<View::ScrollingMessages>(dt);
	fSystemManager.update<Inventory>(dt);
	fSystemManager.update<Model::OtherPlayers>(dt);
}

EntityComponentSystem Controller::gEntityComponentSystem;
