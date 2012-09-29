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

#include "SoundControl.h"

//
// Manage the inventory of a player, and the objects in the inventory.
//
// This class is almost empty for now.
//

class DrawTexture;
class DrawFont;

class Inventory {
public:
	Inventory(void);
	~Inventory(void);
	enum InventoryCategory { ICWeapon, ICArmor, ICHead, ICPotion, ICScroll };
	struct ObjectMap { const char *descr, *code; SoundControl::Sound song; float xOffset; float yOffset; bool dependOnLevel; InventoryCategory category; };
	static struct ObjectMap fsObjectMap[];
	static size_t fsObjectMapSize;

	// Set the amount of items identified by 4-character 'code' to 'n'. 'level' is the level of the item (if any).
	void SetAmount(const char *code, int n, unsigned int level);

	// Use an object, corresponding to the function key
	void UseObjectFunctionKey(int key);
	void DrawInventory(DrawTexture *);

	// As long as the inventory screen is shown, this function will be called. Return 'true' if window shall be closed.
	bool HandleMouseClick(int button, int action, int x, int y);
private:
	struct Item;
	Item *fItemList;
	int fNumItems;

	int IdentifyInv(int x, int y);
	void UseObjectMessage(Item *item, unsigned char cmd);
};

// Only one inventory instance is needed.
extern Inventory gInventory;
