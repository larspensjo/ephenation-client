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

#include <string>
#include <string.h>
#include <stdio.h>
#include <sstream>

#include "ChunkBlocks.h"
#include "chunk.h"
#include "assert.h"
#include "primitives.h"
#include "gamedialog.h"
#include "ui/Activator.h"
#include "FL/Fl_Text_Buffer.H"
#include "Inventory.h"
#include "client_prot.h"
#include "connection.h"
#include "modes.h"
#include "player.h"

#define NELEM(x) (sizeof x/sizeof x[0])

using std::stringstream;
using std::string;
using std::endl;

// Macro that converts a local block dx,dy,dz into the index to be used in the one dimensional vector.
#define INDEX(dx,dy,dz) ((dx*CHUNK_SIZE+dy)*CHUNK_SIZE+dz)

// To get some variation, someof the stone1 materials are shown as stone2. This is only used
// as a visual effect.
static int randomize(int bt, int seed) {
	if (bt != BT_Stone)
		return bt;
	if (((seed*0x87231) ^ 0x12389291) % 5 == 0)
		return BT_Stone2;
	else
		return BT_Stone;
}

// This is done from a separate process!
void ChunkBlocks::Uncompress(void) {
	int from = 0;
	int to = 0;

	std::unique_ptr <unsigned char[]>chunkData(new (unsigned char[CHUNK_VOL]));

	// The data is organized in pairs. First is the block type, then the count of that type.
	do {
		int cnt = fCompressedChunk[from+1];
		// printf("%dx%d ", cnt, fCompressedChunk[from]);
		for (int i=0; i<cnt; i++) {
			int copy = to;
			chunkData[to++] = randomize(fCompressedChunk[from], copy);
		}
		from += 2;
	} while (from < this->compressSize);

	// printf("chunk::Uncompress: unpacked size %d, compressed size %d\n", to, this->compressSize);
	ASSERT(to == CHUNK_VOL);
	fChunkData = std::move(chunkData); // Do this as a last thing, when the uncompress is done
	fChunk->SetDirty(true);
}

ChunkBlocks::ChunkBlocks() {
	flag = 0;
	fChecksum = 0;
	compressSize = 0;
	fChecksumTestNeeded = false;
	fChecksumTimeout = 0.0;
	fOwner = 1; // This means reserved.
	fChunk = 0;
}

ChunkBlocks::~ChunkBlocks() {
}

// Send a string to the server
static void SendString(const char *str) {
	unsigned int len = strlen(str)+3;
	unsigned char buff[3];
	buff[0] = len & 0xFF;
	buff[1] = len >> 8;
	buff[2] = CMD_DEBUG;
	SendMsg(buff, 3);
	SendMsg((const unsigned char *)str, len-3);
}

static struct {
	const char *descr;
	const char *code;
	int dx, dy, dz;
} jellyList[] = {
	{ "None", "" },
	{ "West", "/jelly:w",  -1,0,0 },
	{ "North", "/jelly:n", 0,1,0 },
	{ "East", "/jelly:e",  1,0,0 },
	{ "South", "/jelly:s", 0,-1,0 },
	{ "Up", "/jelly:u",    0,0,1 },
	{ "Down", "/jelly:d",  0,0,-1 },
};

// For the given chunk, coordinate and direction, determine if the specified direction
// is a valid jelly direction.
static bool validJellyDir(const chunk *cp, int dir, int dx, int dy, int dz) {
	dx += jellyList[dir].dx;
	dy += jellyList[dir].dy;
	dz += jellyList[dir].dz;
	// First make sure destination block is inside current chunk.
	if (dx < 0 || dz >= CHUNK_SIZE || dy < 0 || dy >= CHUNK_SIZE || dz < 0 || dz >= CHUNK_SIZE)
		return false;
	if (cp == 0)
		return true; // Safe guard, should not happen
	int bl = cp->GetBlock(dx, dy, dz);
	return bl != BT_Air && bl != BT_Trigger && bl != BT_Text && bl != BT_Link;
}

// Ask the user for an activator message
static void CreateActivatorMessage(int dx, int dy, int dz, const ChunkCoord *cc) {
	// Show a dialog where the player can configure the activator block.
	Fl_Text_Buffer buf;
	Activator act(&buf+0);
	for (size_t i=0; i < gSoundControl.fNumTrigSounds; i++) {
		const char *descr = gSoundControl.fTrigSoundList[i].description;
		// A null pointer indicates that the rest are "private" sounds that may not be used in construction mode
		if (descr == 0)
			break;
		act.fSoundeffect->add(descr, 0, 0);
	}
	for (size_t i=0; i < gInventory.fsObjectMapSize; i++) {
		act.fObject->add(gInventory.fsObjectMap[i].descr);
	}
	chunk *cp = ChunkFind(cc, false);
	for (size_t i = 0; i < 7; i++)
		act.fJellyBlock->add(jellyList[i].descr, 0, 0, 0, !validJellyDir(cp, i, dx, dy, dz));
	act.fJellyBlock->value(0);
	Fl::run();
	Fl::flush();
	if (!act.fOk)
		return;

	// Interpret the result, constructing a string with all of it. Add the conditions first, and then the actions.
	string s;
	if (act.fKeyCondition->value()) {
		// A key condition is defined. This shall come before the broadcast flag, is the test is only applied to one player.
		// As the rest of the line is used as a description, a newline is needed.
		s = s + "/keycond:" + act.fCondKeyId->value() + "," + act.fCondOwnerKeyId->value() + " " + act.fCondKeyDescr->value() + "\n";
	}
	const char *p = act.fBlocks->value();
	if (p[0] == 0)
		p = "10";
	if (atoi(p) > 20)
		p = "20"; // Maximum distance limitation.
	if (act.fBroadcast->value()) // Is the broadcast checkbox enabled?
		s = s + "/broadcast:" + p + " ";
	p = act.fMonsterLevel->text();
	if (p == 0)
		p = "0";
	if (act.fSpawn->value())
		s = s + "/monster:" + p + " ";
	p = act.fInhibit->value();
	if (p[0] != 0) {
		s = s + "/inhibit:" + p + " ";
	}
	// TODO: Should use stringstream all the way
	stringstream ss;
	p = act.fMaxLevel->value();
	if (p[0] != 0) {
		int v = atoi(p)+1;
		ss << "/level<" << v << " ";
	}
	p = act.fMinLevel->value();
	if (p[0] != 0) {
		int v = atoi(p)-1;
		ss << "/level>" << v << " ";
	}
	s += ss.str();

	// Handle all actions
	int v = act.fObject->value();
	if (v != -1 && gInventory.fsObjectMap[v].code != 0)
		s = s + "/invadd:" + gInventory.fsObjectMap[v].code + " ";
	v = act.fJellyBlock->value();
	if (v > 0) {
		s = s + jellyList[v].code + " ";
	}
	// The sound effect must come last, but before the other texts.
	v = act.fSoundeffect->value();
	if (v != -1)
		s = s + "#" + gSoundControl.fTrigSoundList[v].id + " ";
	if (act.fAddKey->value()) {
		// The key is special, as it contains a key description until end of line. Because of that,
		// a newline is needed after the description.
		char buff[10];
#ifdef WIN32
		_snprintf(buff, sizeof buff, "%d", act.fKeyPicture->value());
#else
		snprintf(buff, sizeof buff, "%d", act.fKeyPicture->value());
#endif
		s = s + "/addkey:" + act.fKeyId->value() + "," + buff + " " + act.fKeyDescr->value() + "\n";
	}
	// Finally, add the message itself
	s += buf.text();

	// Strip trailing spaces, if any
	size_t end = s.find_last_not_of(' ');
	if (end != string::npos)
		s = s.substr(0, end+1);
	size_t start = 0;
	while(start < s.length()) {
		end = s.find('\n', start);
		if (end == s.npos) {
			end = s.length();
		}
		char buff[200];
#ifdef WIN32
		_snprintf(buff, sizeof buff, "/activator add %d %d %d %d %d %d %s", cc->x, cc->y, cc->z, dx, dy, dz, s.substr(start, end-start).c_str());
#else
		snprintf(buff, sizeof buff, "/activator add %d %d %d %d %d %d %s", cc->x, cc->y, cc->z, dx, dy, dz, s.substr(start, end-start).c_str());
#endif
		SendString(buff);
		start = end+1; // Skip the newline
	}
}

void ChunkBlocks::SetTeleport(int x, int y, int z) {
	if (fChunkData[INDEX(x,y,z)] == BT_Air)
		fChunkData[INDEX(x,y,z)] = BT_Teleport;
}

void ChunkBlocks::CommandBlockUpdate(const ChunkCoord &cc, int dx, int dy, int dz, int type) {
	if (type == BT_Text && dx == gRequestActivatorX && dy == gRequestActivatorY && dz == gRequestActivatorZ &&
	        cc.x == gRequestActivatorChunk.x &&
	        cc.y == gRequestActivatorChunk.y &&
	        cc.z == gRequestActivatorChunk.z) {
		CreateActivatorMessage(dx, dy, dz, &cc);
		gRequestActivatorX = -1; // Ensure it will not match by accident
	}
	// printf("chunk (%d,%d,%d) offset (%d,%d,%d) updated to %d from %d\n", cc.x, cc.y, cc.z, dx, dy, dz, type, this->chunkData[INDEX(dx,dy,dz)]);
	fChunkData[INDEX(dx,dy,dz)] = type;
	// If a block was changed near another chunk, this other chunk may have to update visible surfaces
	if (dx == 0) {
		ChunkCoord cc2 = cc;
		cc2.x = cc.x-1;
		if (chunk *cp2 = ChunkFind(&cc2,false)) cp2->SetDirty(true);
	}
	if (dy == 0) {
		ChunkCoord cc2 = cc;
		cc2.y = cc.y-1;
		if (chunk *cp2 = ChunkFind(&cc2,false)) cp2->SetDirty(true);
	}
	if (dz == 0) { // This is safe even if there are no chunks below 0
		ChunkCoord cc2 = cc;
		cc2.z = cc.z-1;
		if (chunk *cp2 = ChunkFind(&cc2,false)) cp2->SetDirty(true);
	}
	if (dx == CHUNK_SIZE-1) {
		ChunkCoord cc2 = cc;
		cc2.x = cc.x+1;
		if (chunk *cp2 = ChunkFind(&cc2,false)) cp2->SetDirty(true);
	}
	if (dy == CHUNK_SIZE-1) {
		ChunkCoord cc2 = cc;
		cc2.y = cc.y+1;
		if (chunk *cp2 = ChunkFind(&cc2,false)) cp2->SetDirty(true);
	}
	if (dz == CHUNK_SIZE-1) {
		ChunkCoord cc2 = cc;
		cc2.z = cc.z+1;
		if (chunk *cp2 = ChunkFind(&cc2,false)) cp2->SetDirty(true);
	}
}

struct ChunkBlocks::JellyBlock {
	unsigned char dx, dy, dz;
	unsigned char type;
	double timeout;
};

void ChunkBlocks::AddTimerJellyBlock(unsigned char dx, unsigned char dy, unsigned char dz, double deltaTime) {
	JellyBlock *jb = new JellyBlock;
	jb->dx = dx;
	jb->dy = dy;
	jb->dz = dz;
	jb->type = fChunkData[INDEX(dx, dy, dz)];
	jb->timeout = gCurrentFrameTime + deltaTime;
	fJellyBlockList.push_front(jb);
}

void ChunkBlocks::TestJellyBlockTimeout(bool unconditionally) {
	bool updated = false;
	while(fJellyBlockList.size() > 0) {
		JellyBlock *jb = fJellyBlockList.back();
		// The timers are sorted, with the oldest one last. If the oldest has not timed out, none have.
		if (jb->timeout > gCurrentFrameTime && !unconditionally)
			break;
		this->CommandBlockUpdate(fChunk->cc, jb->dx, jb->dy, jb->dz, jb->type);
		fJellyBlockList.pop_back();
		updated = true;
	}
	if (updated)
		fChunk->SetDirty(true);
}

enum {
	BLT_RIGID =      1<<0,
	BLT_PLASTIC =    1<<1,
	BLT_SEMITRANSP = 1<<2,
	BLT_TRANSP =     1<<3, // Completely transparent
};

static const struct { unsigned char block; unsigned char flag; } propertybase[] = {
	{ BT_Stone,                                   BLT_PLASTIC           },
	{ BT_Water,                    BLT_SEMITRANSP|BLT_PLASTIC           },
	{ BT_BrownWater,               BLT_SEMITRANSP|BLT_PLASTIC           },
	{ BT_Air,           BLT_TRANSP                                      },
	{ BT_Brick,                                               BLT_RIGID },
	{ BT_Soil,                                    BLT_PLASTIC           },
	{ BT_Logs,                                                BLT_RIGID },
	{ BT_Sand,                                    BLT_PLASTIC           },
	{ BT_Tree1,         BLT_TRANSP                                      },
	{ BT_Tree2,         BLT_TRANSP                                      },
	{ BT_Tree3,         BLT_TRANSP                                      },
	{ BT_Tuft,          BLT_TRANSP                                      },
	{ BT_Flowers,       BLT_TRANSP                                      },
	{ BT_Lamp1,         BLT_TRANSP                                      },
	{ BT_Lamp2,         BLT_TRANSP                                      },
	{ BT_CobbleStone,                                         BLT_RIGID },
	{ BT_Ladder,                                              BLT_RIGID },
	{ BT_Hedge,                                   BLT_PLASTIC           },
	{ BT_Window,                   BLT_SEMITRANSP|            BLT_RIGID },
	{ BT_Snow,                                    BLT_PLASTIC           },
	{ BT_Black,                                   BLT_PLASTIC           },
	{ BT_Concrete,                                BLT_PLASTIC           },
	{ BT_WhiteConcrete,                           BLT_PLASTIC           },
	{ BT_Gravel,                                  BLT_PLASTIC           },
	{ BT_TiledStone,                                          BLT_RIGID },
	{ BT_SmallFog,      BLT_TRANSP                                      },
	{ BT_BigFog,        BLT_TRANSP                                      },
	{ BT_Treasure,      BLT_TRANSP                                      },
	{ BT_Quest,         BLT_TRANSP                                      },
	{ BT_Stone2,                                  BLT_PLASTIC           },
	{ BT_Text,          BLT_TRANSP                                      },
	{ BT_Link,          BLT_TRANSP|               BLT_PLASTIC           },
	{ BT_Trigger,       BLT_TRANSP|               BLT_PLASTIC           },
	{ BT_Teleport,      BLT_TRANSP                                      },
};

unsigned char property[256];

void ChunkBlocks::InitStatic(void) {
	for (size_t i=0; i < NELEM(propertybase); i++)
		property[propertybase[i].block] = propertybase[i].flag;
}

bool ChunkBlocks::blockIsSemiTransp(unsigned char bl) {
	return (property[bl] & BLT_SEMITRANSP) || (property[bl] & BLT_TRANSP);
}

bool ChunkBlocks::blockIsComplTransp(unsigned char bl) const {
	bool owner = true;
	if (this->fOwner != gPlayer.GetId() && gPlayer.fAdmin == 0)
		owner = false;
	if (owner && gMode.Get() == GameMode::CONSTRUCT && bl != BT_Air)
		return false;
	return property[bl] & BLT_TRANSP;
}

bool ChunkBlocks::rigid(unsigned char bl) {
	return property[bl] & BLT_RIGID;
}

bool ChunkBlocks::plastic(unsigned char bl) {
	return property[bl] & BLT_PLASTIC;
}
