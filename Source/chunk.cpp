// Copyright 2012-2014 The Ephenation Authors
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

#include <GL/glew.h>
#include <GL/glfw.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

#include "chunk.h"
#include "chunkcache.h"
#include "connection.h"
#include "parse.h"
#include "textures.h"
#include "shaders/StageOneShader.h"
#include "shaders/ChunkShaderPicking.h"
#include "ui/Error.h"
#include "ChunkProcess.h"
#include "shapes/Cube.h"
#include "Options.h"
#include "SuperChunkManager.h"
#include "client_prot.h"
#include "modes.h"

#define cChecksumTimeout 15.0

using namespace View;

// Find out if a block at (ox,oy,oz) is in a line of sight of the sky, as given by the direction (dx,dy,dz).
// The algorithm isn't perfect when near corners of other blocks, but it isn't important.
static bool LineToSky(const Chunk *cp, int ox, int oy, int oz, float dx, float dy, float dz) {
	ASSERT(dz > 0);
	float fx = float(ox), fy = float(oy), fz = float(oz);

	ChunkCoord cc = cp->cc;
	// Simply never use sun light below a certain point
	if (cc.z < -1)
		return false;
	const Chunk *currentChunk = cp;
	while(1) {
		if (fx<0) { fx += CHUNK_SIZE; cc.x--; currentChunk = ChunkFind(&cc, false); }
		if (fy<0) { fy += CHUNK_SIZE; cc.y--; currentChunk = ChunkFind(&cc, false); }
		if (fz<0) { fz += CHUNK_SIZE; cc.z--; currentChunk = ChunkFind(&cc, false); }
		if (fx>=CHUNK_SIZE) { fx -= CHUNK_SIZE; cc.x++; currentChunk = ChunkFind(&cc, false); }
		if (fy>=CHUNK_SIZE) { fy -= CHUNK_SIZE; cc.y++; currentChunk = ChunkFind(&cc, false); }
		if (fz>=CHUNK_SIZE) { fz -= CHUNK_SIZE; cc.z++; currentChunk = ChunkFind(&cc, false); }

		// If the distance to the next chunk above is big enough, ignore shadows from it.
		if (cc.z > cp->cc.z + 2)
			return true; // Reached the sky!
		if (currentChunk == 0)
			return true; // We don't know, so assume there is sky. TODO: Not clever enough.
		int x = int(floorf(fx)), y = int(floorf(fy)), z = int(floorf(fz));
		if (!Model::ChunkBlocks::blockIsSemiTransp(currentChunk->GetBlock(x, y, z)))
			return false;
		fx += dx; fy += dy; fz += dz;
	}
}

// Find out if a surface is in the direction of sun light. The in falling angle is handled by the renderer.
bool Chunk::InSunLight(int ox, int oy, int oz) const {
	return LineToSky(this, ox, oy, oz, -0.577350269f, -0.577350269f, 0.577350269f);
}

// Find out if a surface is in the direction sky, from a limited number of directions. Return
// a value from 0 to 1. The given coordinate may be just outside the chunk!
// A value of 1.0 means full view of sky in all directions.
float Chunk::ComputeAmbientLight(int ox, int oy, int oz) const {
	// The direction pointing to the sun is not included.
	float sky =
	    LineToSky(this, ox, oy, oz, 0.0f, 0.0f, 1.0f) +
	    LineToSky(this, ox, oy, oz, 0.707106781f, 0.0f, 0.707106781f) +
	    LineToSky(this, ox, oy, oz, 0.0f, 0.707106781f, 0.707106781f) +
	    LineToSky(this, ox, oy, oz, -0.707106781f, 0.0f, 0.707106781f) +
	    LineToSky(this, ox, oy, oz, 0.0f, -0.707106781f, 0.707106781f);
	float num = 5.0f;
	if (gOptions.fPerformance > 2) {
		// Additional tests give even better transition from light to dark, but requires more CPU
		sky +=
		    LineToSky(this, ox, oy, oz, 0.577350269f, -0.577350269, 0.577350269) +
		    LineToSky(this, ox, oy, oz, -0.577350269f, 0.577350269, 0.577350269) +
		    LineToSky(this, ox, oy, oz, 0.577350269f, 0.577350269, 0.577350269) +
		    LineToSky(this, ox, oy, oz, -0.577350269f, -0.577350269, 0.577350269);
		num += 4.0f;
	}
	glm::vec3 block(ox, oy, oz);
	float sum = sky * 1.0f / num;
	if (sum > 1.0f)
		sum = 1.0f;
	return sum;
}

void Chunk::UpdateGraphics(void) {
	unsigned char x, y, z;
	if (gMode.CommunicationAllowed() && Model::gSuperChunkManager.GetTeleport(&x, &y, &z, &this->cc)) {
		// This is kind of a fake. There are no TP blocks at the server, they are managed in
		// other types of data structures. But the server understands the command to remove a
		// TP block.
		this->fChunkBlocks->SetTeleport(x, y, z);
	}
	gChunkProcess.AddTaskComputeChunk(this);
}

//
// All chunks loaded in memory are managed by a hash table to make them quick and
// easy to find.
// TODO: A mechanism is needed to throw away chunks when there are too many
//

static std::map<ChunkCoord, Chunk*> sWorldCache;

void Chunk::UpdateNeighborChunks(void) {
	for (int dx=-1; dx<2; dx++) for (int dy=-1; dy<2; dy++) for (int dz=-1; dz<2; dz++) {
				if (dx == 0 && dy == 0 && dz == 0)
					continue; // Skip self
				if ((dx != 0 && dy != 0) || (dx != 0 && dz != 0) || (dy != 0 && dz != 0))
					continue; // Only diagonal contact, no effect
				ChunkCoord cc = this->cc;
				cc.x = this->cc.x + dx;
				cc.y = this->cc.y + dy;
				cc.z = this->cc.z + dz;
				Chunk *cp = ChunkFind(&cc, false);
				if (cp && cp->fChunkBlocks) {
					cp->SetDirty(true);
				}
			}
}

// Making all chunks "dirty" will force them all to be recomputed.
void Chunk::MakeAllChunksDirty(void) {
	for (auto it = sWorldCache.begin(); it != sWorldCache.end(); it++) {
		Chunk *cp = it->second;
		cp->SetDirty(true);
	}
}

// Given a chunk coordinate, find the chunk.
// If "force" is true and the chunk isn't loaded yet, a request is sent to the server to load it, but a temporary empty chunk is returned.
Chunk* ChunkFind(const ChunkCoord *cc, bool force) {
	auto it = sWorldCache.find(*cc);

	if (it != sWorldCache.end())
		return it->second;

	if (!force) {
		// It wasn't found, and it will be ignored if it isn't available
		return 0;
	}

	// Didn't find the chunk, create one.
	// printf("Created a new chunk at (%d,%d,%d)\n", cc->x, cc->y, cc->z);
	Chunk *pc = new(Chunk);
	// Chunk coordinate has to be defined before the chunk is added to the sWorldCache map.
	pc->cc.x = cc->x; pc->cc.y = cc->y; pc->cc.z = cc->z;

	// Check for chunk in cache
	auto cb = ChunkCache::fgChunkCache.LoadChunkFromCache(cc);
	if (cb != nullptr) {
		// printf("ChunkFind: Load chunk from cache (%d,%d,%d)\n", cc->x, cc->y, cc->z);
		cb->fChecksumTestNeeded = true; // Checksum need to be tested, as the chunk may have changed on the server
		cb->fChunk = pc;    // Needed by Uncompress
		cb->Uncompress();
		pc->fChunkBlocks = cb;

		sWorldCache[pc->cc] = pc;
		pc->UpdateNeighborChunks(); // This will now use the new ChunkBlocks. 'cp' must be in sWorldCache map before this call.
	} else {
		// A dummy ChunkBlocks is needed. That way, every chunk always havea a ChunkBlocks.
		auto nc = std::make_shared<Model::ChunkBlocks>();
		// Even though the chunk coordinates are not unsigned, they can be parsed as such.
		nc->flag = 0;
		nc->fChecksum = 0;
		nc->fChecksumTestNeeded = false; // Not needed as there is a request further below for an update.
		nc->fChecksumTimeout = 0.0;
		nc->fOwner = -1;
		nc->compressSize = 0;
		nc->fChunkData.reset(new(unsigned char[CHUNK_VOL]));
		nc->fChunk = pc;
		pc->fChunkBlocks = nc;
		for (int i=0; i<CHUNK_VOL; i++)
			nc->fChunkData[i] = BT_Air; // Not perfect, but something to start with

		sWorldCache[pc->cc] = pc;
		// Initiate the action to get the chunk from the server. It will arrive a little later on.
		//printf("ChunkFind: Request chunk (%d,%d,%d)\n", cc->x, cc->y, cc->z);
		unsigned char b[15];
		b[0] = sizeof b;
		b[1] = 0;
		b[2] = CMD_READ_CHUNK;
		EncodeUint32(b+3, (unsigned int)cc->x);
		EncodeUint32(b+7, (unsigned int)cc->y);
		EncodeUint32(b+11, (unsigned int)cc->z);
		SendMsg(b, sizeof b);
	}

	return pc;
}

void Chunk::Draw(StageOneShader *shader, ChunkShaderPicking *pickShader, DL_Type dlType) {
	// printf("chunk::Draw buffers for (%d,%d,%d)\n", this->cc.x, this->cc.y, this->cc.z);
	if (this->fPrev_gl) {
		// Remove the chunk from the previous linked list.
		*this->fPrev_gl = this->fNext_gl;
	}
	if (this->fNext_gl) {
		this->fNext_gl->fPrev_gl = this->fPrev_gl;
	}
	// Add the chunk to the linked list of OpenGL busy chunks
	this->fNext_gl = sfBusyList_gl;
	if (sfBusyList_gl) {
		sfBusyList_gl->fPrev_gl = &this->fNext_gl;
	}
	sfBusyList_gl = this;
	this->fPrev_gl = &sfBusyList_gl;

	if (this->IsDirty() && !this->fScheduledForLoading) {
		this->UpdateGraphics();
	}

	ASSERT(fChunkBlocks);
	auto cb = fChunkBlocks;
	// ASSERT(cb);

	if (gMode.CommunicationAllowed() && cb->fChecksumTestNeeded && gCurrentFrameTime > cb->fChecksumTimeout) {
		// This chunk need to verify the checksum
		cb->fChecksumTestNeeded = false;
		cb->fChecksumTimeout = gCurrentFrameTime+cChecksumTimeout; // Reset checksum timer when request is sent

		// TODO: Put this is another wrapper and send several requests at the same time...
		unsigned char b[10];
		b[0] = sizeof b;
		b[1] = 0;
		b[2] = CMD_VRFY_CHUNK_CS;
		b[3] = this->cc.x & 0xFF;
		b[4] = this->cc.y & 0xFF;
		b[5] = this->cc.z & 0xFF;
		b[6] = cb->fChecksum & 0xFF;
		b[7] = (cb->fChecksum >> 8) & 0xFF;
		b[8] = (cb->fChecksum >> 16 ) & 0xFF;
		b[9] = (cb->fChecksum >> 24 ) & 0xFF;
		// printf("Sending checksum: %u\n", this->fCheckSum );
		SendMsg(b, sizeof b);
	}

	if (!fChunkObject)
		return; // There are no grapical objects to draw.

	if (!this->fBuffersDefined) {
		// It may be that this->fScheduledForComputation or this->fScheduledForLoading is true, but it is not a common thing,
		// so we may as well find something to draw.
		if (dlType == DL_Picking)
			this->PrepareOpenGL(0, pickShader, dlType);
		else
			this->PrepareOpenGL(shader, 0, dlType);
	}

	for (int blockType = 1; blockType < 256; blockType++) {
		int triSize = fChunkObject->VertexSize(blockType);
		if (triSize == 0)
			continue; // No blocks of this type to draw.
		if (Model::ChunkBlocks::blockIsSemiTransp(blockType) && dlType == DL_NoTransparent)
			continue;
		if (!Model::ChunkBlocks::blockIsSemiTransp(blockType) && dlType == DL_OnlyTransparent)
			continue;
		if (dlType == DL_OnlyTransparent && (blockType == BT_Water || blockType == BT_BrownWater)) {
			glDisable(GL_CULL_FACE);
			shader->DrawingWater(true);
		}

		glBindTexture(GL_TEXTURE_2D, BlockTypeTotextureId[blockType]);
		glBindVertexArray(fVao[blockType]);
		glDrawArrays(GL_TRIANGLES, 0, triSize);
		gNumDraw++;
		gDrawnQuads += triSize/3;
		if (dlType == DL_OnlyTransparent && (blockType == BT_Water || blockType == BT_BrownWater)) {
			glEnable(GL_CULL_FACE);
			shader->DrawingWater(false);
		}
	}
	glBindVertexArray(0);
}

void Chunk::DrawObjects(StageOneShader *shader, int dx, int dy, int dz, bool forShadows) const {
	if (!fChunkObject)
		return; // Nothing to show yet

	// Create the offset in OpenGL coordinates
	glm::vec3 offset(dx*CHUNK_SIZE + 0.5f, dz*CHUNK_SIZE, - dy*CHUNK_SIZE - 0.5f);

	fChunkObject->DrawTrees(shader, offset, forShadows);

	if (forShadows)
		return; // Skip the rest of the objects

	fChunkObject->FindSpecialObjects(offset, 12.0f);
	fChunkObject->FindFogs(offset);
	fChunkObject->DrawLamps(shader, offset);
	fChunkObject->DrawTreasures(shader, offset);
}

Chunk::~Chunk() {
	// It is not strictly necessary to clear deleted pointers, but it will help to find bugs.
	this->ReleaseOpenGLBuffers();
}

void Chunk::ReleaseOpenGLBuffers(void) {
	if (!this->fBuffersDefined)
		return;
	glDeleteVertexArrays(256, fVao); // A buffer with value 0 is ignored.
	// glDeleteBuffers(256, fBufferId); // Seems to be a bug in AMD. Deleting a buffer 0 will destroy the glBindBufferBase binding
	for (int blockType = 0; blockType < 256; blockType++) {
		if (fBufferId[blockType] != 0)
			glDeleteBuffers(1, &fBufferId[blockType]);
		fBufferId[blockType] = 0;
		fVao[blockType] = 0;
	}
	this->fBuffersDefined = false;
}

void Chunk::PrepareOpenGL(StageOneShader *shader, ChunkShaderPicking *pickShader, DL_Type dlType) {
	this->ReleaseOpenGLBuffers(); // Release the old buffers, if any.
	fBuffersDefined = true;
	// printf("PrepareOpenGL: chunk (%d,%d,%d)\n", this->cc.x, this->cc.y, this->cc.z);

	enum { stride = sizeof(VertexDataf), };

	ASSERT(fChunkObject);
	for (int blockType = 1; blockType < 256; blockType++) {
		int triSize = fChunkObject->VertexSize(blockType);
		if (triSize == 0)
			continue; // No blocks of this type to draw.
		glGenVertexArrays(1, &fVao[blockType]);
		glBindVertexArray(fVao[blockType]);
		glGenBuffers(1, &fBufferId[blockType]);
		glBindBuffer(GL_ARRAY_BUFFER, fBufferId[blockType]);
		glBufferData(GL_ARRAY_BUFFER, triSize * sizeof(VertexDataf), &fChunkObject->fVisibleTriangles[blockType][0], GL_STATIC_DRAW);
		// Can't release vertex buffer, as it may be reused after a call to chunk::ReleaseOpenGLBuffers().
		// check that data size in VBO is the same as the input array, if not return 0 and delete VBO
		int bufferSize = 0;
		glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
		if ((unsigned)bufferSize != triSize * sizeof(VertexDataf)) {
			checkError("Chunk::PrepareOpenGL data size mismatch");
			glDeleteBuffers(1, &fBufferId[blockType]);
			fBufferId[blockType] = 0;
			ErrorDialog("Chunk::PrepareOpenGL: Data size %d is mismatch with input array %d\n", bufferSize, triSize * sizeof(VertexDataf));
		}

		switch(dlType) {
		case DL_Picking:
			pickShader->EnableVertexAttribArray();
			// The normals are stored in "char", but they are in practice of type "unsigned char", and we don't
			// want OpenGL to convert them to signed char.
			pickShader->NormalAttribPointer(GL_UNSIGNED_BYTE, stride, VertexDataf::GetNormalOffset());
			pickShader->VertexAttribPointer(GL_SHORT, stride, VertexDataf::GetVertexOffset());
			break;
		case DL_NoTransparent:
		case DL_OnlyTransparent:
			shader->EnableVertexAttribArray();
			shader->VertexAttribPointer();
			break;
		}
		// glBindBuffer(GL_ARRAY_BUFFER, 0);  // Deprecated
	}
	glBindVertexArray(0);
}

Chunk::Chunk() {
	fBuffersDefined = false;
	fNext_gl = 0;
	fPrev_gl = 0;
	fDirty = false;
	fScheduledForComputation = false;
	fScheduledForLoading = false;
	for (int i=0; i<256; i++) {
		fBufferId[i] = 0;
		fVao[i] = 0;
	}
}

Chunk *Chunk::sfBusyList_gl = 0;
Chunk *Chunk::sfBusyListOld_gl = 0;

// Find out what chunks are currently not drawn, and release OpenGL buffers for them
void Chunk::DegradeBusyList_gl(void) {
	while(sfBusyListOld_gl) {
		//printf("chunk::PurgeBusyList_gl release gl buffers for (%d,%d,%d)\n", sfBusyListOld_gl->cc.x, sfBusyListOld_gl->cc.y, sfBusyListOld_gl->cc.z);
		//printf("RELEASE(%d,%d,%d)\n", sfBusyListOld_gl->cc.x, sfBusyListOld_gl->cc.y, sfBusyListOld_gl->cc.z);
		sfBusyListOld_gl->fPrev_gl = 0;
		sfBusyListOld_gl->ReleaseOpenGLBuffers();
		sfBusyListOld_gl->fChunkBlocks->fChecksumTestNeeded = true; // The time out is set when the checksum is requested (next time)
		Chunk *ch = sfBusyListOld_gl;
		sfBusyListOld_gl = sfBusyListOld_gl->fNext_gl;
		ch->fNext_gl = 0;
	}

	// All unused chunks from the old busy list has been released. Move the current list to
	// the old list.
	if (sfBusyList_gl) {
		sfBusyList_gl->fPrev_gl = &sfBusyListOld_gl;
		sfBusyListOld_gl = sfBusyList_gl;
		sfBusyList_gl = 0;
	}
}

void Chunk::PushTriangles(shared_ptr<const ChunkObject> co) {
	// printf("Push %d,%d,%d, comp %d, loading %d, 0x%x\n", cc.x, cc.x, cc.z, fScheduledForComputation, fScheduledForLoading, fChunkObject.get());
	if (fPushedValues) {
		ErrorDialog("Illegal Chunk::PushTriangles\n");
	}
	fPushedValues = fChunkObject;
	fChunkObject = co;
	this->ReleaseOpenGLBuffers();
}

void Chunk::PopTriangles(void) {
	// printf("Pop %d,%d,%d, comp %d, loading %d\n", cc.x, cc.x, cc.z, fScheduledForComputation, fScheduledForLoading);
	if (!fPushedValues) {
		ErrorDialog("Chunk::PopTriangles: Pop without a push first");
	}
	this->ReleaseOpenGLBuffers();
	fChunkObject = fPushedValues;
	fPushedValues.reset();
}

#define FACT ((long long)BLOCK_COORD_RES * CHUNK_SIZE)

static int ScaleToChunk(long long x) {
	if (x >= 0)
		return (int)(x / FACT);
	else
		return (int)((x - FACT + 1) / FACT);
}

static void GetChunkCoord(ChunkCoord *cc, signed long long x, signed long long y, signed long long z) {
	cc->x = ScaleToChunk(x);
	cc->y = ScaleToChunk(y);
	cc->z = ScaleToChunk(z);
}

static void GetOffsetToChunk(ChunkOffsetCoord* coc, signed long long x, signed long long y, signed long long z) {
	ChunkCoord cc;
	GetChunkCoord(&cc, x, y, z);
	coc->x = (unsigned short)(x - cc.x*FACT);
	coc->y = (unsigned short)(y - cc.y*FACT);
	coc->z = (unsigned short)(z - cc.z*FACT);
	// printf("this->y = %lld, cc.y=%d, FACT*cc.y=%lld, diff=%lld\n", this->y, cc.y, FACT*cc.y, this->y - cc.y*FACT);
}

unsigned char Chunk::GetChunkAndBlock(signed long long x, signed long long y, signed long long z) {
	ChunkCoord cc;
	GetChunkCoord(&cc, x, y, z);
	Chunk *cp = ChunkFind(&cc, false);
	if (cp == 0)
		return BT_Unused;
	ChunkOffsetCoord coc;
	GetOffsetToChunk(&coc,  x, y, z);
	return cp->GetBlock(coc.x/BLOCK_COORD_RES, coc.y/BLOCK_COORD_RES, coc.z/BLOCK_COORD_RES);
}

void Chunk::SetDirty(bool flag) {
	if (flag && fScheduledForLoading)
		return; // Will be recomputed anyway
	ASSERT(flag == false || fScheduledForLoading == false);
	// if (gVerbose && flag) printf("Chunk::SetDirty %d,%d,%d\n", cc.x, cc.y, cc.z);
	fDirty = flag;
}

// Note, 'this' pointer can be 0!
void Chunk::DrawBoundingBox(StageOneShader *shader, int dx, int dy, int dz) {
	char bxmin = -LAMP2_DIST, bymin = -LAMP2_DIST, bzmin = -LAMP2_DIST;
	char bxmax = CHUNK_SIZE+LAMP2_DIST, bymax = CHUNK_SIZE+LAMP2_DIST, bzmax = CHUNK_SIZE+LAMP2_DIST;
	if (this && !this->IsDirty()) {
		shared_ptr<const ChunkObject> co = this->fChunkObject;
		if (co != nullptr) {
			// There are real boundaries available
			bxmin = co->fBoundXMin; bxmax = co->fBoundXMax; bymin = co->fBoundYMin; bymax = co->fBoundYMax; bzmin = co->fBoundZMin; bzmax = co->fBoundZMax;
		}
	}

	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(dx*CHUNK_SIZE+16.0f-bxmin, dz*CHUNK_SIZE-bzmin, -dy*CHUNK_SIZE-16.0f+-bymin));
	modelMatrix = glm::rotate(modelMatrix, 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	const float scale = 32.0f * 1.414f; // Side was originally 1/sqrt(2) blocks, but height was 1.
	float xFact = float(bxmax-bxmin)/CHUNK_SIZE;
	float yFact = float(bymax-bymin)/CHUNK_SIZE;
	float zFact = float(bzmax-bzmin)/CHUNK_SIZE;
	modelMatrix = glm::scale(modelMatrix, glm::vec3(scale*xFact, 32.0f*zFact, scale*yFact));
	glBindTexture(GL_TEXTURE_2D, GameTexture::BlueChunkBorder); // Any texture will do
	shader->Model(modelMatrix);
	gLantern.Draw(); // TODO: This shape (gLantern) is rotated, a more proper cube should be used, avoiding complicated matrices.
}
