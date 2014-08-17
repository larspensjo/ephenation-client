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
#ifdef WIN32
#include <direct.h>
#include <Windows.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#ifdef unix
#include <ftw.h>
#endif

#include <iostream>
#include <fstream>
#include <iomanip>

#include "Options.h"
#include "chunkcache.h"
#include "ChunkBlocks.h"
#include "assert.h"
#include "worsttime.h"
#include "chunk.h"

using std::cout;
using std::fstream;
using std::ofstream;
using std::ifstream;
using std::ios;

ChunkCache ChunkCache::fgChunkCache;

ChunkCache::ChunkCache() : fCacheDir(0) {
	fTotalSize = 0;
	fTotalFiles = 0;
};

ChunkCache::~ChunkCache() {
	if (fCacheDir != 0)
		delete[] fCacheDir;
};

static unsigned long totalSize = 0;
static unsigned long totalFiles = 0;

#if 0
static int sum(const char *fpath, const struct stat *sb, int typeflag) {
	totalSize += sb->st_size;
	totalFiles++;
	return 0;
}
#endif

void ChunkCache::SetCacheDir(const char *dirName) {
	fCacheDir = dirName;

	// Verify that the directory exists and create if not existing
#ifdef unix
	struct stat st;
	if (stat(dirName,&st) != 0) {
		mkdir(dirName, 0777);
	}

#if 0
	// TODO: Enable again when there is a usage for the result
	if (ftw(dirName, &sum, 1)) {
		printf("ftw");
	}
#endif
#endif
#ifdef WIN32
	struct _stat st;
	if (_stat(dirName,&st) != 0) {
		_mkdir(dirName);
	}

#if 0
	// TODO: Enable again when there is a usage for the result
	WIN32_FIND_DATA data;
	HANDLE hFind;
	bool bContinue = true;
	char *szCurDirPath = new char[strlen(dirName)+10];

	sprintf(szCurDirPath, "%s*", dirName); // Add a wildcard to the search string
	//printf("Win path: %s\n", szCurDirPath);

	hFind = FindFirstFile(szCurDirPath, &data);

	if (hFind == INVALID_HANDLE_VALUE) {
		printf ("FindFirstFile failed (%ld)\n", GetLastError());
		bContinue = false; // This will send us straight through the file size counter...
	}

	while( hFind && bContinue ) {
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Directory, ignore
		} else {
			// File
			totalFiles++;
			totalSize += (data.nFileSizeHigh * (MAXDWORD+1)) + data.nFileSizeLow;
		}

		bContinue=FindNextFile(hFind, &data);
	}
#endif
#endif

	fTotalFiles = totalFiles;
	fTotalSize = totalSize;

	// printf("Cache in %s: %lu in %lu files\n", dirName, fTotalSize, fTotalFiles);
};

// TODO: Need to add initial chunk info as well!
bool ChunkCache::IsChunkInCache(const ChunkCoord *cc) {
	char *fileName;

	// TODO: remove magic number
	fileName = new char[strlen(fCacheDir)+100];
	sprintf( fileName, "%scf%d,%d,%d", fCacheDir, cc->x, cc->y, cc->z);

	ifstream TestFile(fileName);

	if( TestFile.fail() ) {
		//printf("File %s does not exist!\n", fileName);
		TestFile.close();
		return false;
	}

	//printf("File %s exists!\n", fileName);
	TestFile.close();
	return true;
};

// TODO: Need to add initial chunk info as well!
void ChunkCache::SaveChunkInCache(const cachunk *chunkdata) {
	static WorstTime tm("SaveChnkCache");
	tm.Start();

	unsigned char *buffer = chunkdata->compressedChunk;
	int len = chunkdata->compressSize;

	char *fileName;

	// TODO: remove magic number
	fileName = new char[strlen(fCacheDir)+100];
	sprintf( fileName, "%scf%d,%d,%d", fCacheDir, chunkdata->cc.x, chunkdata->cc.y, chunkdata->cc.z);

	ofstream WriteChunk(fileName, ios::binary);

	if( WriteChunk.fail() ) {
		printf("Failed to open %s\n", fileName);
	} else {
		WriteChunk.write((char *) &chunkdata->flag, sizeof(chunkdata->flag));
		WriteChunk.write((char *) &chunkdata->fCheckSum, sizeof(chunkdata->fCheckSum));
		WriteChunk.write((char *) &chunkdata->fOwner, sizeof(chunkdata->fOwner));

		for( int x=0; x<len; x++) {
			WriteChunk << buffer[x];
		}
	}

	// printf("Cache: %s\n", fileName);

	WriteChunk.close();
	tm.Stop();
	delete[] fileName;
};

std::shared_ptr<Model::ChunkBlocks> ChunkCache::LoadChunkFromCache(const ChunkCoord *cc) {
	char *fileName;

	// Check if chunk is in cache, for safety
	if( !IsChunkInCache(cc) )
		return nullptr;

	static WorstTime tm("LdChnkFrmCache");
	tm.Start();
	auto cachedata = std::make_shared<Model::ChunkBlocks>();

	// TODO: std::string
	fileName = new char[strlen(fCacheDir)+100];
	sprintf( fileName, "%scf%d,%d,%d", fCacheDir, cc->x, cc->y, cc->z);

	ifstream ReadChunk(fileName, ios::binary);

	if( ReadChunk.fail() ) {
		printf("Failed to open %s\n", fileName);
		return nullptr;
	} else {
		ReadChunk.seekg(0, ios::end);
		int size = ReadChunk.tellg();
		if (size == 0)
			return nullptr; // Safety precaution, should not happen unless corrupt file
		ReadChunk.seekg(0, ios::beg);

		ReadChunk.read((char *) &cachedata->flag, sizeof(cachedata->flag));
		ReadChunk.read((char *) &cachedata->fChecksum, sizeof(cachedata->fChecksum));
		ReadChunk.read((char *) &cachedata->fOwner, sizeof(cachedata->fOwner));

		cachedata->compressSize = size-sizeof(cachedata->flag)-sizeof(cachedata->fChecksum)-sizeof(cachedata->fOwner);
		cachedata->fCompressedChunk.reset(new unsigned char[cachedata->compressSize]);

		ReadChunk.read((char*)cachedata->fCompressedChunk.get(), cachedata->compressSize);
		ASSERT(ReadChunk.gcount() == cachedata->compressSize);
		// printf("Cache load: %s size (%i)\n", fileName, size);
	}

	ReadChunk.close();

	delete[] fileName;

	tm.Stop();
	return cachedata;
};

