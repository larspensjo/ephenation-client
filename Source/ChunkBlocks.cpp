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

void ChunkBlocks::SetTeleport(int x, int y, int z) {
	if (fChunkData[INDEX(x,y,z)] == BT_Air)
		fChunkData[INDEX(x,y,z)] = BT_Teleport;
}

void ChunkBlocks::CommandBlockUpdate(const ChunkCoord &cc, int dx, int dy, int dz, int type) {
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
	if (this->fOwner != gPlayer.GetId() && gPlayer.fAdmin == 0 && !gPlayer.fTestPlayer)
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
