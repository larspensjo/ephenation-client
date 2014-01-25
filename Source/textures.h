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

#include <memory>

using std::shared_ptr;

// Use a block type and translate it into texture id.
extern GLuint BlockTypeTotextureId[];

extern int currentBuildBlockIndex;

struct GameTexture {
	int blType;
	GLuint id;
	const char *fileName;
	const char *descr;
	unsigned flag;

	static void Init(void);
	static GLuint GrassTextureId, TuftOfGrass, Flowers;
	static GLuint LeafTextureId;
	static GLuint StoneTexture2Id, LadderOnStone;
	static GLuint TreeBarkId, Hedge, Window, Snow, Branch;
	static GLuint Sky1Id, Sky2Id, Sky3Id, Sky4Id, SkyupId;
	static GLuint RedScalesId, Morran;
	static GLuint Fur1Id;
	static GLuint LanternSideId, Teleport;
	static GLuint LightBallsHeal, InventoryId, EquipmentId;
	static GLuint RedColor, GreenColor, BlueColor, DarkGray;
	static GLuint RedChunkBorder, BlueChunkBorder, GreenChunkBorder;
	static GLuint CompassRose, DamageIndication;
	static GLuint TextureBlock;
	static GLuint WEP1, WEP2, WEP3, WEP4;
	static GLuint WEP1Text, WEP2Text, WEP3Text, WEP4Text;
	static GLuint Coin, Quest;
	static GLuint MousePointerId;

	static const int fgNumBuildBlocks; // Number of available building blocks

	static const GameTexture *GetGameTexture(int index);
};

class Image;

// Load bitmaps to be used for the GUI.
extern GLuint LoadBitmapForGui(shared_ptr<Image>);

// Load bitmaps to be used for animated models.
extern GLuint LoadBitmapForModels(shared_ptr<Image>);
