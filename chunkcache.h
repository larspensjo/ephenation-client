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
// Compressed chunks use 200 to several kBytes of memory. Instead of asking for these chunks
// from the server every time the client is restarted, a local cache is maintained.
//

#include <memory>

#include "chunk.h"

class ChunkBlocks;
class ChunkCoord;

class ChunkCache {
public:
	struct cachunk {
		ChunkCoord cc;

		unsigned long flag;
		unsigned int fCheckSum;
		unsigned long fOwner;

		unsigned char *compressedChunk;
		int compressSize;
	};

	static ChunkCache fgChunkCache;

	void SetCacheDir(const char *);
	bool IsChunkInCache(const ChunkCoord *cc);
	void SaveChunkInCache(const cachunk *);
	std::shared_ptr<ChunkBlocks> LoadChunkFromCache(const ChunkCoord *cc);
private:
	ChunkCache();
	~ChunkCache();

	unsigned long fTotalSize;
	unsigned long fTotalFiles;

	const char *fCacheDir;
};
