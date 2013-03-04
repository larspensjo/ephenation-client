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
#include <entityx/Event.h>

#include "SoundControl.h"

class DrawTexture;
class DrawFont;

/// @brief Manage the inventory of a player, and the objects in the inventory.
/// @todo This class is a bad mixture of Model/Class/Controller.
class Inventory : public entityx::System<Inventory> {
public:
	Inventory(void);
	~Inventory(void);
	enum InventoryCategory { ICWeapon, ICArmor, ICHead, ICPotion, ICScroll };
	struct ObjectMap { const char *descr, *code; float xOffset; float yOffset; bool dependOnLevel; InventoryCategory category; };
	static struct ObjectMap fsObjectMap[];
	static size_t fsObjectMapSize;

	/// Set the amount of items.
	/// @param code Identified by 4-character 'code'.
	/// @param n The amount.
	/// @param level The level of the item (if any).
	void SetAmount(const char *code, int n, unsigned int level);

	/// Use an object, corresponding to the function key
	void UseObjectFunctionKey(int key);
	void DrawInventory(DrawTexture *);

	/// As long as the inventory screen is shown, this function will be called.
	/// @return true if window shall be closed.
	bool HandleMouseClick(int button, int action, int x, int y);

	/// Apply System behavior.
	///
	/// Called every game step.
	virtual void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) override;

	/// Called automatically after System initialization
	virtual void configure(entityx::EventManager &events) override;

	/// Event generated when the player is hit by a monster
	struct AddObjectToPlayerEvt : public entityx::Event<AddObjectToPlayerEvt> {
		/// Add a message originating at an object
		AddObjectToPlayerEvt(ObjectMap *map) : map(map) {}

		ObjectMap *map; /// Describes the object
	};
private:
	struct Item;
	Item *fItemList;
	int fNumItems;

	int IdentifyInv(int x, int y);
	void UseObjectMessage(Item *item, unsigned char cmd);
};

// Only one inventory instance is needed.
extern boost::shared_ptr<Inventory> gInventory;
