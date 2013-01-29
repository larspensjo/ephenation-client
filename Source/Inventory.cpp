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

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glfw.h>
#include <string>
#include <iostream>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Inventory.h"
#include "modes.h"
#include "textures.h"
#include "DrawTexture.h"
#include "player.h"
#include "client_prot.h"
#include "connection.h"
#include "msgwindow.h"
#include "DrawText.h"
#include "primitives.h"
#include "parse.h"
#include "vsfl/vsfl.h"
#include "ScrollingMessages.h"
#include "gamedialog.h"
#include "msgwindow.h"

using namespace std;

enum DragAndDropState {
	DadNone,     // No Drag and drop in progress
	DadDragging, // An object is being dragged
} sDadState = DadNone;

static int dragItem = -1; // The inventory id of the item currently being dragged.

// Data structure to manage the player inventory
struct Inventory::Item {
	Inventory::ObjectMap *fInfo;
	unsigned long fLevel;
	unsigned long fAmount;
};

Inventory gInventory;

#define IS 8.0f // The icon scaling. Has to be a float.

// Define the list of all possible objects that can be created.
// The 'code' is used by the server to identify the object.
// The offsets are offsets into the EquipmentIcons.bmp bitmap, where to find a corresponding icon for the item.
Inventory::ObjectMap Inventory::fsObjectMap[] = {
	{ " ", 0, 0, 3/IS,  4/IS}, // null element to allow the use of no objects.
	{ "healing potion",                        "POTH", SoundControl::SDropPotion, 0/IS, 6/IS, false, ICPotion },
	{ "mana potion",                           "POTM", SoundControl::SDropPotion, 1/IS, 6/IS, false, ICPotion },
	{ "scroll of resurrection point",          "S001", SoundControl::SDropScroll, 0/IS, 4/IS, true, ICScroll },
	{ "sword",                                 "WEP1", SoundControl::SDropWeapon, 0/IS, 7/IS, true, ICWeapon },
	{ "fine sword",                            "WEP2", SoundControl::SDropWeapon, 1/IS, 7/IS, true, ICWeapon },
	{ "mighty sword",                          "WEP3", SoundControl::SDropWeapon, 2/IS, 7/IS, true, ICWeapon },
	{ "shining blue sword with engraved gems", "WEP4", SoundControl::SDropWeapon, 3/IS, 7/IS, true, ICWeapon },
	{ "shirt",                                 "ARM1", SoundControl::SDropArmor,  0/IS, 5/IS, true, ICArmor },
	{ "armor",                                 "ARM2", SoundControl::SDropArmor,  1/IS, 5/IS, true, ICArmor },
	{ "shiny armor",                           "ARM3", SoundControl::SDropArmor,  2/IS, 5/IS, true, ICArmor },
	{ "plate mail with magic reinforcement",   "ARM4", SoundControl::SDropArmor,  3/IS, 5/IS, true, ICArmor },
	{ "straw hat",                             "HLM1", SoundControl::SDropArmor,  0/IS, 0/IS, true, ICHead },
	{ "helmet",                                "HLM2", SoundControl::SDropArmor,  1/IS, 0/IS, true, ICHead },
	{ "shiny helmet",                          "HLM3", SoundControl::SDropArmor,  2/IS, 0/IS, true, ICHead },
	{ "dragon helmet",                         "HLM4", SoundControl::SDropArmor,  3/IS, 0/IS, true, ICHead },
};

#define NUM_OBJECTS (sizeof Inventory::fsObjectMap / sizeof Inventory::fsObjectMap[0])

// This is currently the same as the number of slots in the inventory screen
#define MAX_ITEM_LIST 42

size_t Inventory::fsObjectMapSize = NUM_OBJECTS;

// Make it easy and quick to find a weapon type. TODO: Not a flexible solution.
static Inventory::ObjectMap *sFirstWeapon;
static Inventory::ObjectMap *sFirstArmor;
static Inventory::ObjectMap *sFirstHelmet;

Inventory::Inventory(void) {
	fItemList = new Item[MAX_ITEM_LIST];
	fNumItems = 0;

	for (size_t i = 0; i < NUM_OBJECTS; i++) {
		if (fsObjectMap[i].code && strncmp(fsObjectMap[i].code, "WEP1", 4) == 0) {
			sFirstWeapon = &fsObjectMap[i];
		}
		if (fsObjectMap[i].code && strncmp(fsObjectMap[i].code, "ARM1", 4) == 0) {
			sFirstArmor = &fsObjectMap[i];
		}
		if (fsObjectMap[i].code && strncmp(fsObjectMap[i].code, "HLM1", 4) == 0) {
			sFirstHelmet = &fsObjectMap[i];
		}
	}
}

Inventory::~Inventory(void) {
	delete []fItemList;
}

static Inventory::ObjectMap *FindInfo(const char *code) {
	for (size_t i=0; i < NUM_OBJECTS; i++) {
		Inventory::ObjectMap *info = &Inventory::fsObjectMap[i];
		if (info->code && strncmp(info->code , code, 4) == 0)
			return info;
	}
	return 0;
}

void Inventory::SetAmount(const char *code, int n, unsigned lvl) {
	// Search for this item in the inventory list
	for (int i=0; i < fNumItems; i++) {
		if (strncmp(fItemList[i].fInfo->code, code, 4) != 0)
			continue;
		if (fItemList[i].fInfo->dependOnLevel && fItemList[i].fLevel != lvl)
			continue;
		int prevAmount = fItemList[i].fAmount;
		fItemList[i].fAmount = n;
		// Give a sound feed back to the player, but not if this is the initial list sent in the login process.
		if (n > prevAmount && gMode.Get() != GameMode::WAIT_ACK) {
			gSoundControl.RequestSound(fItemList[i].fInfo->song);
			gScrollingMessages.AddMessagePlayer(fItemList[i].fInfo->descr);
		}
		// printf("Inventory::SetAmount code '%s': count %d descr '%s' level %d\n", code, n, fsObjectMap[i].descr, lvl);
		if (n == 0) {
			// Remove this item completely from the list, overwriting it from the last one.
			fItemList[i] = fItemList[fNumItems-1];
			fNumItems--;
		}
		return;
	}

	// The item wasn't already in the list, add it
	if (fNumItems == MAX_ITEM_LIST)
		return; // Sorry, can't show more
	Inventory::ObjectMap *info = FindInfo(code);
	if (info == 0)
		return; // Unknown type, can't handle it
	fItemList[fNumItems].fAmount = n;
	fItemList[fNumItems].fLevel = lvl;
	fItemList[fNumItems].fInfo = info;
	if (gMode.Get() != GameMode::WAIT_ACK) {
		gSoundControl.RequestSound(fItemList[fNumItems].fInfo->song);
		gScrollingMessages.AddMessagePlayer(fItemList[fNumItems].fInfo->descr);
	}
	fNumItems++;
	return;
}

void Inventory::UseObjectMessage(Item *item, unsigned char cmd) {
	// Request the server to use the object. This may change the amount, in which case the server will
	// send an message update.
	unsigned char b[11];
	b[0] = sizeof b; b[1] = 0; b[2] = cmd;
	for (int j=0; j<4; j++)
		b[j+3] = item->fInfo->code[j];
	EncodeUint32(b+7, item->fLevel);
	SendMsg(b, sizeof b);
}

void Inventory::UseObjectFunctionKey(int key) {
	size_t ind = size_t(key - GLFW_KEY_F1 + 1);
	if (ind == 0 || ind >= NUM_OBJECTS)
		return;
	ObjectMap *info = &fsObjectMap[ind];

	// Find if there is such an object in the inventory list
	for (int i=0; i<fNumItems; i++) {
		if (fItemList[i].fInfo == info) {
			// Request the server to use the object. This may change the amount, in which case the server will
			// send an message update.
			this->UseObjectMessage(&fItemList[i], CMD_USE_ITEM);
			return;
		}
	}

	gSoundControl.RequestTrigSound("FAIL");
	gMsgWindow.Add("You don't have any object of that type");
}

#define INVENTORY_SCALE 1.3f // How big the inventory screen will be. 2.0 would be a full X width.
#define INVENTORY_ROWSIZE 6 // Number of items per row
#define INVENTORY_SCREEN_X_OFFS (-0.67292f)
#define INVENTORY_SCREEN_Y_OFFS (-0.69492f)

// The following are the pixel coordinates read out from the bitmap.
// The Y values are counting from bottom.
#define WEAPON_SCREEN_X 189 // Read left pixel from bitmap
#define WEAPON_SCREEN_Y 216 // 600 - lower pixel from bitmap
#define ARMOR_SCREEN_X 39   // Read left pixel from bitmap
#define ARMOR_SCREEN_Y 411  // 600 - lower pixel from bitmap
#define HELMET_SCREEN_X 39   // Read left pixel from bitmap
#define HELMET_SCREEN_Y 486  // 600 - lower pixel from bitmap
#define MSG1_SCREEN_X 39    // Read left pixel from bitmap
#define MSG1_SCREEN_Y 182   // 600 - lower pixel from bitmap
#define CLOSE_BTNX1 962     // The red button in the upper right corner used to close the window
#define CLOSE_BTNY1 564
#define CLOSE_BTNX2 980
#define CLOSE_BTNY2 582

static float screenRatio;

// Given a pixel x coordinate of the inventory screen, return the window x position
static float xPixelToScreen(int x) {
	return INVENTORY_SCALE/1000*x;
}

// Given a pixel y coordinate of the inventory screen, return the window y position
static float yPixelToScreen(int y) {
	return screenRatio*INVENTORY_SCALE/600*y;
}

static float X(int col) {
	return xPixelToScreen(512+75*col); // Constants are pixels taken from the bitmap
}

static float Y(int row) {
	return yPixelToScreen(36+75*(INVENTORY_ROWSIZE-row)); // Constants are pixels taken from the bitmap
}

// Compensate for a fighter at level 'pLevel' using a weapon at level 'itemLevel'.
// The principle is that a weapon at wrong level will give a penalty.
// Return a multiplier near 1. The same algorithm is used by the server in the combat
// algorithms. If it is changed here, make sure it is also changed in the server, and vice versa.
// See https://docs.google.com/drawings/d/1cmObDuDgBvYhpsQagfjHPlckXjluU3CR9utVkTSh_rE/edit?hl=en_US
static string WeaponLevelDiffMultiplier(unsigned long pLevel, unsigned long itemLevel, unsigned char itemType) {
	unsigned char detract = 0;
	if (pLevel > itemLevel+itemType) {
		detract = pLevel - itemLevel - itemType; // This will be greater than zero
	}
	if (pLevel+itemType < itemLevel) {
		detract = itemLevel - pLevel - itemType; // This will be greater than zero
	}
	if (detract > itemType) {
		detract = itemType;
	}
	itemType -= detract;
	float ret = 1.0f;
	switch (itemType) {
	case 0: ret = 0.9; break;
	case 1: ret = 1.0; break;
	case 2: ret = 1.1; break;
	case 3: ret = 1.2; break;
	case 4: ret = 1.3; break;
	default: ret = 1.0; break; // Shouldn't happen
	}
	stringstream ss;
	ss.precision(2);
	ss << (ret-1)*100;
	return string("Weapon modifier: ") + ss.str() + " %";
}

// Compensate for a fighter at level 'pLevel' using an armor at level 'aLevel'.
// The same principle is used as for a weapon, see above. The damage is divided by this factor.
const float COMB_ArmorModifierCal = 0.282f;
static string ArmorLevelDiffMultiplier(unsigned long pLevel, unsigned long itemLevel, unsigned char itemType) {
	unsigned char detract = 0;
	if (pLevel > itemLevel+itemType) {
		detract = pLevel - itemLevel - itemType; // This will be greater than zero
	}
	if (pLevel+itemType < itemLevel) {
		detract = itemLevel - pLevel - itemType; // This will be greater than zero
	}
	if (detract > itemType) {
		detract = itemType;
	}
	itemType -= detract;
	float ret = 1.0f;
	switch (itemType) {
	case 0: ret = 0.9; break;
	case 1: ret = 1.0; break;
	case 2: ret = 1.1; break;
	case 3: ret = 1.2; break;
	case 4: ret = 1.3; break;
	default: ret = 1.0; break; // Shouldn't happen
	}
	stringstream ss;
	ss.precision(2);
	ss << (ret-1) * COMB_ArmorModifierCal*100;
	return string("modifier: ") + ss.str() + " %";
}

void Inventory::DrawInventory(DrawTexture *drawTexture) {
	glm::mat4 baseModel = glm::translate(glm::mat4(1.0f), glm::vec3(INVENTORY_SCREEN_X_OFFS, INVENTORY_SCREEN_Y_OFFS, 0.0f));
	// The bitmap is 1000x600
	screenRatio = 600.0f/1000.0f*gViewport[2] / gViewport[3];
	glm::mat4 inventoryModel = glm::scale(baseModel, glm::vec3(INVENTORY_SCALE, screenRatio*INVENTORY_SCALE, 1.0f));
	static const glm::mat4 ident(1.0f); // No need to create a new one every time.
	glBindTexture(GL_TEXTURE_2D, GameTexture::InventoryId);
	drawTexture->Draw(ident, inventoryModel);
	const glm::vec3 itemIconScale(72.0f/1000*INVENTORY_SCALE, 71.0f/600*screenRatio*INVENTORY_SCALE, 1.0f);

	// Iterate over each item in the equipment list, and draw it on the inventory screen.
	// The EquipmentId is an atlas with all different types of equipment icons
	glBindTexture(GL_TEXTURE_2D, GameTexture::EquipmentId);

	int x, y;
	glfwGetMousePos(&x, &y);
	// Compute mouse screen coordinates relative inventory screen.
	float xMouse = x / gViewport[2] * 2 - 1 - INVENTORY_SCREEN_X_OFFS;
	float yMouse = 1 - y / gViewport[3] * 2 - INVENTORY_SCREEN_Y_OFFS;
	float dX = X(1) - X(0); // On screen distance from one column to the next
	float dY = Y(0) - Y(1);

	// Optional tooltip information, depending on where the mouse is
	char buff[100];
	float tooltipX=0, tooltipY=0;
	bool drawTooltip = false;

	for (int ind=0; ind<fNumItems; ind++) {
		ObjectMap *info = fItemList[ind].fInfo;
		glm::mat4 iconBase;
		glm::mat4 iconModel;
		if (ind == dragItem && sDadState == DadDragging) {
			iconBase = glm::translate(baseModel, glm::vec3(xMouse, yMouse, 0.0f));
			iconModel = glm::scale(iconBase, itemIconScale);
		} else {
			int row = ind / INVENTORY_ROWSIZE;
			int col = ind % INVENTORY_ROWSIZE;
			iconBase = glm::translate(baseModel, glm::vec3(X(col), Y(row), 0.0f));
			iconModel = glm::scale(iconBase, itemIconScale);
			if (xMouse > X(col) && xMouse < X(col+1) && yMouse > Y(row) && yMouse < Y(row-1)) {
				tooltipX = (iconModel[3].x+1)/2 * gViewport[2];
				tooltipY = (1-iconModel[3].y - dY)/2 * gViewport[3];
#ifdef WIN32
				_snprintf(buff, sizeof buff, "%s (%ld)\nlevel %ld\nQuick key F%ld", info->descr, fItemList[ind].fAmount, fItemList[ind].fLevel, info - fsObjectMap);
#else
				snprintf(buff, sizeof buff, "%s (%ld)\nlevel %ld\nQuick key F%d", info->descr, fItemList[ind].fAmount, fItemList[ind].fLevel, unsigned(info - fsObjectMap));
#endif
				drawTooltip = true;
			}
		}
		drawTexture->DrawOffset(ident, iconModel, info->xOffset, info->yOffset, 1/IS);
	}

	// Draw the equipped weapon
	int wepType = Model::gPlayer.fWeaponType;
	if (wepType > 0) {
		Inventory::ObjectMap *o = &sFirstWeapon[wepType-1];
		float xOffset = xPixelToScreen(WEAPON_SCREEN_X);
		float yOffset = yPixelToScreen(WEAPON_SCREEN_Y);
		glm::mat4 weaponModel = glm::translate(baseModel, glm::vec3(xOffset, yOffset, 0.0f));
		weaponModel = glm::scale(weaponModel, itemIconScale);
		drawTexture->DrawOffset(ident, weaponModel, o->xOffset, o->yOffset, 1/IS);
		if (xMouse > xOffset && xMouse < xOffset + dX && yMouse > yOffset && yMouse < yOffset + dY) {
			tooltipX = (weaponModel[3].x+1)/2 * gViewport[2];
			tooltipY = (1-weaponModel[3].y - dY)/2 * gViewport[3];
#ifdef WIN32
			_snprintf(buff, sizeof buff, "%s\nlevel %ld", o->descr, Model::gPlayer.fWeaponLevel);
#else
			snprintf(buff, sizeof buff, "%s\nlevel %ld", o->descr, Model::gPlayer.fWeaponLevel);
#endif
			drawTooltip = true;
		}
	}

	// Draw the equipped armor
	int armType = Model::gPlayer.fArmorType;
	if (armType > 0) {
		Inventory::ObjectMap *o = &sFirstArmor[armType-1];
		float xOffset = xPixelToScreen(ARMOR_SCREEN_X);
		float yOffset = yPixelToScreen(ARMOR_SCREEN_Y);
		glm::mat4 itemModel = glm::translate(baseModel, glm::vec3(xOffset, yOffset, 0.0f));
		itemModel = glm::scale(itemModel, itemIconScale);
		drawTexture->DrawOffset(ident, itemModel, o->xOffset, o->yOffset, 1/IS);
		if (xMouse > xOffset && xMouse < xOffset + dX && yMouse > yOffset && yMouse < yOffset + dY) {
			tooltipX = (itemModel[3].x+1)/2 * gViewport[2];
			tooltipY = (1-itemModel[3].y - dY)/2 * gViewport[3];
#ifdef WIN32
			_snprintf(buff, sizeof buff, "%s\nlevel %ld", o->descr, Model::gPlayer.fArmorLevel);
#else
			snprintf(buff, sizeof buff, "%s\nlevel %ld", o->descr, Model::gPlayer.fArmorLevel);
#endif
			drawTooltip = true;
		}
	}

	// Draw the equipped helmet
	int hlmType = Model::gPlayer.fHelmetType;
	if (hlmType > 0) {
		Inventory::ObjectMap *o = &sFirstHelmet[hlmType-1];
		float xOffset = xPixelToScreen(HELMET_SCREEN_X);
		float yOffset = yPixelToScreen(HELMET_SCREEN_Y);
		glm::mat4 itemModel = glm::translate(baseModel, glm::vec3(xOffset, yOffset, 0.0f));
		itemModel = glm::scale(itemModel, itemIconScale);
		drawTexture->DrawOffset(ident, itemModel, o->xOffset, o->yOffset, 1/IS);
		if (xMouse > xOffset && xMouse < xOffset + dX && yMouse > yOffset && yMouse < yOffset + dY) {
			tooltipX = (itemModel[3].x+1)/2 * gViewport[2];
			tooltipY = (1-itemModel[3].y - dY)/2 * gViewport[3];
#ifdef WIN32
			_snprintf(buff, sizeof buff, "%s\nlevel %ld", o->descr, Model::gPlayer.fArmorLevel);
#else
			snprintf(buff, sizeof buff, "%s\nlevel %ld", o->descr, Model::gPlayer.fArmorLevel);
#endif
			drawTooltip = true;
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	gDrawFont.Enable();
	gDrawFont.UpdateProjection();
	if (drawTooltip) {
		gDrawFont.SetOffset(tooltipX, tooltipY);
		gDrawFont.vsfl.renderAndDiscard(buff);
	}
	// TODO: Complex algorithm, should be simplified. But it works...
	float xOffset = xPixelToScreen(MSG1_SCREEN_X);
	float yOffset = yPixelToScreen(MSG1_SCREEN_Y);
	glm::mat4 msgModel = glm::translate(baseModel, glm::vec3(xOffset, yOffset, 0.0f));
	msgModel = glm::scale(msgModel, itemIconScale);
	int msgX = (msgModel[3].x+1)/2 * gViewport[2];
	int msgY = (1-msgModel[3].y)/2 * gViewport[3];
	gDrawFont.SetOffset(msgX, msgY);
	gDrawFont.vsfl.renderAndDiscard(WeaponLevelDiffMultiplier(Model::gPlayer.fLevel, Model::gPlayer.fWeaponLevel, Model::gPlayer.fWeaponType));
	gDrawFont.SetOffset(msgX, msgY+20);
	gDrawFont.vsfl.renderAndDiscard("Armor " + ArmorLevelDiffMultiplier(Model::gPlayer.fLevel, Model::gPlayer.fArmorLevel, Model::gPlayer.fArmorType));
	gDrawFont.SetOffset(msgX, msgY+40);
	gDrawFont.vsfl.renderAndDiscard("Helmet " + ArmorLevelDiffMultiplier(Model::gPlayer.fLevel, Model::gPlayer.fHelmetLevel, Model::gPlayer.fHelmetType));
	gDrawFont.SetOffset(msgX, msgY+60);
	gDrawFont.vsfl.renderAndDiscard("Shield modifier: 0 % (TBD)");
	gDrawFont.SetOffset(msgX, msgY+80);
	gDrawFont.vsfl.renderAndDiscard("Boots modifier: 0 % (TBD)");
	gDrawFont.SetOffset(msgX, msgY+100);
	gDrawFont.vsfl.renderAndDiscard("Legs modifier: 0 % (TBD)");
	gDrawFont.Disable();
}

// The following code is used for drag-and-drop of items in the inventory
// ======================================================================

// Identify what inventory item is at specified screen coordinate
int Inventory::IdentifyInv(int x, int y) {
	// Compute mouse screen coordinates relative inventory screen.
	float xMouse = x / gViewport[2] * 2 - 1 - INVENTORY_SCREEN_X_OFFS;
	float yMouse = 1 - y / gViewport[3] * 2 - INVENTORY_SCREEN_Y_OFFS;
	for (int ind=0; ind<fNumItems; ind++) {
		int row = ind / INVENTORY_ROWSIZE;
		int col = ind % INVENTORY_ROWSIZE;
		if (xMouse > X(col) && xMouse < X(col+1) && yMouse > Y(row) && yMouse < Y(row-1)) {
			return ind;
		}
	}
	return -1;
}

// Validate that an item of category 'ic' can be dropped outside of the inventory
static bool validateDropOutside(Inventory::InventoryCategory ic, int x, int y) {
	// Compute mouse screen coordinates relative inventory screen.
	float xMouse = x / gViewport[2] * 2 - 1 - INVENTORY_SCREEN_X_OFFS;
	float yMouse = 1 - y / gViewport[3] * 2 - INVENTORY_SCREEN_Y_OFFS;
	bool ret = false;
	float xOffset = xPixelToScreen(1000);
	float yOffset = yPixelToScreen(600);
	switch(ic) {
	case Inventory::ICWeapon:
	case Inventory::ICArmor:
	case Inventory::ICHead:
	case Inventory::ICScroll:
		if (xMouse < 0 || yMouse < 0 || xMouse > xOffset || yMouse > yOffset)
			ret = true;
		break;
	case Inventory::ICPotion:
		ret = false;
		break;
	}
	return ret;
}

// Validate that an item of category 'ic' can be dropped at the specified screen position
static bool validateDropZone(Inventory::InventoryCategory ic, int x, int y) {
	// Compute mouse screen coordinates relative inventory screen.
	float xMouse = x / gViewport[2] * 2 - 1 - INVENTORY_SCREEN_X_OFFS;
	float yMouse = 1 - y / gViewport[3] * 2 - INVENTORY_SCREEN_Y_OFFS;
	float dX = X(1) - X(0); // On screen distance from one column to the next
	float dY = Y(0) - Y(1);
	bool ret = false;
	float xOffset, yOffset;
	switch(ic) {
	case Inventory::ICArmor: {
		xOffset = xPixelToScreen(ARMOR_SCREEN_X);
		yOffset = yPixelToScreen(ARMOR_SCREEN_Y);
		if (xMouse > xOffset && xMouse < xOffset + dX && yMouse > yOffset && yMouse < yOffset + dY) {
			// printf("Dropped on armor!\n");
			ret = true;
		}
		break;
	}
	case Inventory::ICWeapon:
		xOffset = xPixelToScreen(WEAPON_SCREEN_X);
		yOffset = yPixelToScreen(WEAPON_SCREEN_Y);
		if (xMouse > xOffset && xMouse < xOffset + dX && yMouse > yOffset && yMouse < yOffset + dY) {
			// printf("Dropped on weapon!\n");
			ret = true;
		}
		break;
	case Inventory::ICHead:
		xOffset = xPixelToScreen(HELMET_SCREEN_X);
		yOffset = yPixelToScreen(HELMET_SCREEN_Y);
		if (xMouse > xOffset && xMouse < xOffset + dX && yMouse > yOffset && yMouse < yOffset + dY) {
			// printf("Dropped on weapon!\n");
			ret = true;
		}
		break;
	case Inventory::ICPotion:
	case Inventory::ICScroll:
		ret = false;
		break;
	}
	return ret;
}

// Detect if the mouse is pointing at the close button of the inventory screen.
static bool PointingAtCloseButton(int x, int y) {
	// Compute mouse screen coordinates relative inventory screen.
	float xMouse = x / gViewport[2] * 2 - 1 - INVENTORY_SCREEN_X_OFFS;
	float yMouse = 1 - y / gViewport[3] * 2 - INVENTORY_SCREEN_Y_OFFS;
	// printf("Closing: (%f, %f) (%f-%f), (%f-%f)\n", xMouse, yMouse, xPixelToScreen(CLOSE_BTNX1), xPixelToScreen(CLOSE_BTNX2), yPixelToScreen(CLOSE_BTNY1), yPixelToScreen(CLOSE_BTNY2));
	return xMouse >= xPixelToScreen(CLOSE_BTNX1) &&
	       xMouse <= xPixelToScreen(CLOSE_BTNX2) &&
	       yMouse >= yPixelToScreen(CLOSE_BTNY1) &&
	       yMouse <= yPixelToScreen(CLOSE_BTNY2);
}

bool Inventory::HandleMouseClick(int button, int action, int x, int y) {
	// printf("Inventory::HandleMouseClick: button %d, action %d, coord %d,%d\n", button, action , x, y);
	if (button != GLFW_MOUSE_BUTTON_LEFT)
		return false;
	switch (sDadState) {
	case DadNone:
		if (action == GLFW_PRESS) {
			int id = IdentifyInv(x, y);
			if (id != -1) {
				dragItem = id;
				static int previousItem;
				static double previousClick;
				if (previousItem == id && glfwGetTime() - previousClick < 0.2) {
					// This was a double click on the same item, use it instead of dragging it.
					this->UseObjectMessage(&fItemList[id], CMD_USE_ITEM);
					previousClick = 0.0; // Prevent a second double click
					break;
				}
				previousItem = id;
				previousClick = glfwGetTime();
				sDadState = DadDragging;
				break;
			}
		}
		if (action == GLFW_RELEASE && PointingAtCloseButton(x, y))
			return true;
		break;
	case DadDragging:
		if (action == GLFW_RELEASE) {
			ObjectMap *info = fItemList[dragItem].fInfo;
			// printf("Drop %s at %d,%d\n", info->descr, x, y);
			sDadState = DadNone;
			InventoryCategory ic = info->category;
			if (validateDropZone(ic, x, y)) {
				this->UseObjectMessage(&fItemList[dragItem], CMD_USE_ITEM);
			} else if (validateDropOutside(ic, x, y)) {
				gMsgWindow.SetAlternatePosition(x, y, true);
				this->UseObjectMessage(&fItemList[dragItem], CMD_DROP_ITEM);
			}
		}
		break;
	}
	return false;
}
