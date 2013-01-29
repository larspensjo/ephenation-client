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

#include <stdio.h>
#include <string.h>

#include "SuperChunkManager.h"
#include "client_prot.h"
#include "connection.h"

#define flagTPDefined 0x01 // TOOD: This has to match the flag defined by the server. Move it to client_prot.h?

using namespace Model;

SuperChunkManager Model::gSuperChunkManager;

SuperChunkManager::SuperChunkManager() {
	// TODO Auto-generated constructor stub
}

SuperChunkManager::~SuperChunkManager() {
	for (mp::iterator it = fMap.begin(); it != fMap.end(); it++)
		delete it->second;
	fMap.clear();
}

SuperChunk *SuperChunkManager::Find(int x, int y, int z) {
	ChunkCoord cc = {x, y, z};
	mp::iterator it = fMap.find(cc);
	if (it == fMap.end()) {
		// This super chunk was not available yet. Create an empty one to use meanwhile.
		SuperChunk *sc = new SuperChunk;
		sc->fCheckSum = 0xFFFFFFFF;
		memset(sc, '\0', sizeof (SuperChunk));
		fMap[cc] = sc;

		// Request an update of this Super Chunk
		unsigned char msg[10];
		msg[0] = sizeof msg;
		msg[1] = 0;
		msg[2] = CMD_VRFY_SUPERCHUNCK_CS;
		msg[3] = x & 0xFF;
		msg[4] = y & 0xFF;
		msg[5] = z & 0xFF;
		msg[6] = 0; // Checksum bytes
		msg[7] = 0;
		msg[8] = 0;
		msg[9] = 0;
		// printf("SuperChunkManager::Find requesting %d,%d,%d\n", x, y, z);
		SendMsg(msg, sizeof msg);
		sc->fUpdateRequested = true;
		return sc;
	} else {
		// printf("Find: Found, size %ld\n", fMap.size());
		return it->second;
	}
}

static int trunc(int x) {
	if (x < 0)
		x -= SCH_SIZE-1;
	return (x/SCH_SIZE)*SCH_SIZE;
}

bool SuperChunkManager::GetTeleport(unsigned char *px, unsigned char *py, unsigned char *pz, const ChunkCoord *cc) {
	int x = trunc(cc->x), y = trunc(cc->y), z = trunc(cc->z);
	SuperChunk *sc = this->Find(x, y, z);
	int xBase = cc->x - x, yBase = cc->y - y, zBase = cc->z - z;
	SuperChunk::chunkInfo *ci = &sc->fChunkInfo[xBase][yBase][zBase];
	*px = ci->x; *py = ci->y; *pz = ci->z;
	if (ci->flag && flagTPDefined)
		return true;
	return false;
}

void SuperChunkManager::Data(const ChunkCoord *cc, const unsigned char *p) {
	SuperChunk *sc = this->Find(cc->x, cc->y, cc->z);
	// printf("SuperChunkManager::Data for %d,%d,%d (size %ld)\n", cc->x, cc->y, cc->z, sizeof sc->fChunkInfo + sizeof sc->fCheckSum);
	memcpy(sc, p, sizeof sc->fChunkInfo + sizeof sc->fCheckSum);
#if 0
	for (int x=0; x<SCH_SIZE; x++) for (int y=0; y<SCH_SIZE; y++) for (int z=0; z < SCH_SIZE; z++) {
				SuperChunk::chunkInfo *ci = &sc->fChunkInfo[x][y][z];
				if (ci->flag != 0 || ci->x != 0 || ci->y != 0 || ci->z != 0) {
					printf("SuperChunkManager::Data found for (flag %d) chunk %d,%d,%d: at %d,%d,%d\n", ci->flag, cc->x+x, cc->y+y, cc->z+z, ci->x, ci->y, ci->z);
				}
			}
#endif
}
