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

#include <map>

#include "chunk.h"

#define SCH_SIZE 10

// The order of the content of the SuperChunk is important, as it matches exactly the order that
// data is sent from the server.
struct SuperChunk {
	unsigned int fCheckSum;
	struct chunkInfo {
		unsigned char flag;
		unsigned char x, y, z;
	};

	chunkInfo fChunkInfo[SCH_SIZE][SCH_SIZE][SCH_SIZE];
	bool fUpdateRequested;
};

class SuperChunkManager {
public:
	SuperChunkManager();
	virtual ~SuperChunkManager();
	bool GetTeleport(unsigned char *px, unsigned char *py, unsigned char *pz, const ChunkCoord *cc);

	// Take provided data and install it with the corresponding SuperChunk.
	void Data(const ChunkCoord *cc, const unsigned char *p);
private:
	SuperChunk *Find(int x, int y, int z);
	typedef std::map<ChunkCoord, SuperChunk*> mp;
	mp fMap;
};

extern SuperChunkManager gSuperChunkManager;
