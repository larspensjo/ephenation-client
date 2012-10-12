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

//
// Manage the actual blocks inside a chunk, as well as some information about the chunk as given by the server.
// Chunks from the server, or from the local cache, are compressed.
//
// Jelly blocks is a special mechanism that can temporarily turn any block type into air. This is controlled from
// activator blocks, and it is used for making trap doors and other ways to open a passage. The reason that the
// management of this is in ChunkBlocks is that it will actually change the raw data of the chunk.
//
// TODO: Most data in ChunkBlocks is as decoded from the cache or as received from the server. It should be packged
// in private structure.
//

#include <deque>
#include <memory>

class chunk;
class ChunkCoord;

struct ChunkBlocks {
	unsigned long flag;
	unsigned int fChecksum;
	unsigned long fOwner;
	chunk *fChunk; // The chunk that shall be updated
	double fChecksumTimeout;
	std::unique_ptr <unsigned char[]> fCompressedChunk;
	int compressSize;
	std::unique_ptr <unsigned char[]> fChunkData; // The unpacked data
	bool fChecksumTestNeeded;			  // True if this chunk should verify the checksum from the server.

	void Uncompress(void);

	ChunkBlocks();
	~ChunkBlocks();

	// Add timer for a jelly block
	void AddTimerJellyBlock(unsigned char dx, unsigned char dy, unsigned char dz, double deltaTime);

	// Look at the list of jelly blocks and restore those that have timed out.
	void TestJellyBlockTimeout(bool unconditionally = false);

	// Update a block in a chunk to another type. Any calls of SetDirty() has  to be done afterwards
	void CommandBlockUpdate(const ChunkCoord &cc, int dx, int dy, int dz, int type);

	// Special case. Override block and set to TP type. Only works for air.
	void SetTeleport(int x, int y, int z);

	static void InitStatic(void); // Initialize some static data
	bool blockIsComplTransp(unsigned char bl) const; // Depends dynamically on class data
	static bool blockIsSemiTransp(unsigned char bl);

	// Return true if block 'bl' shall not be shown as smooth
	static bool rigid(unsigned char bl);

	// Return true if block 'bl' shall not be shown as smooth
	static bool plastic(unsigned char bl);
private:
	// Maintain a list of blocks that have been turned invisible.
	struct JellyBlock;
	std::deque<JellyBlock*> fJellyBlockList;
};
