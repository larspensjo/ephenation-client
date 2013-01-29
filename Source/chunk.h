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

#pragma once

#include <GL/glew.h>
#include <memory>

#include "render.h"
#include "ChunkBlocks.h"
#include "ChunkObject.h"

#define CHUNK_SIZE 32
#define WORLD_HEIGHT 8 // World height measured in chunks
#define CHUNK_VOL (CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE)
#define BLOCK_COORD_RES 100 // This is the scale for the player coordinate fixed point

/// @file chunk.h
/// Block types in the chunk
/// @todo Copied manually from worldDB.go, should be generated automatically
#define BT_Unused	0
#define BT_Stone	1
#define BT_Water	2
#define BT_Air		3
#define BT_Brick    4
#define BT_Soil     5
#define BT_Logs	    6
#define BT_Sand		7
#define BT_Tree1	8 // Bush
#define BT_Tree2	9 // Tree
#define BT_Tree3	10 // big tree
#define BT_Lamp1    11 // Lamp with little light
#define BT_Lamp2    12 // Lamp with much light
#define BT_CobbleStone 13
#define BT_Ladder   14
#define BT_Hedge    15
#define BT_Window   16
#define BT_Snow     17
#define BT_BrownWater 18
#define BT_Black    19
#define BT_Concrete 20 // Grey concrete
#define BT_WhiteConcrete 21 // White concrete
#define BT_Gravel 	22
#define BT_TiledStone 23
#define BT_SmallFog 24
#define BT_BigFog   25
#define BT_Treasure 26
#define BT_Quest    27
#define BT_Tuft     28
#define BT_Flowers  29
#define BT_Bark     30
#define BT_RedLight      31 // Add red light
#define BT_GreenLight    32 // Add green light
#define BT_BlueLight     33 // Add blue light

#define BT_Stone2	127 // Use a free block type
#define BT_TopSoil	128
#define BT_Teleport 129

#define BT_Text      251 // Text trigger message
#define BT_DeTrigger 252 // The opposite of a trigger. Will reset activator blocks.
// #define BT_Spawn     253 // Spawn a monster. Activated from a trig block. Now replaced by BT_Text
#define BT_Link      254 // Link a trig action.
#define BT_Trigger   255 // Trig an action when a player pass through

//
// Constants used by the drawing.
//
#define CH_TopFace		1
#define CH_BottomFace	2
#define CH_LeftFace		3
#define CH_RightFace	4
#define CH_FrontFace	5
#define CH_BackFace		6

// Lamp 2 shall be stronger than lamp 1. Using linear diminishing with distance isn't physical correct,
// but looks fine.
// The same constants are used for small and big fogs also.
#define LAMP1_DIST 8 // Lamp 1 will go linearly from light to dark in this number of blocks.
#define LAMP2_DIST 14 // Lamp 2 will go linearly from light to dark in this number of blocks.

/// A coordinate in the Ephenation system, not in the OpenGL system. The chunk
/// coordinates are numbered 0, 1, 2, etc.
/// This means that real coordinate is chunk coordinate * CHUNK_SIZE.
struct ChunkCoord {
	int x, y, z;

	// This operator is used for sorting. Most significant criteria is 'z', to ensure the
	// chunks are sorted with high 'z' first.
	// TODO: The current sorting criteria is optimized order for drawing chunks for shadows.
	//       But the shadow drawing should use it's own sorting criteria.
	bool operator<(const ChunkCoord &other) const {
		if (z > other.z) return true;
		else if (z < other.z) return false;

		if (y < other.y) return true;
		else if (y > other.y) return false;

		if (x < other.x) return true;
		return false;
	}
};

struct ChunkOffsetCoord {
	unsigned short x, y, z; // TODO: This should be unsigned char? But it doesn't work.
};

struct VertexDataf;
struct TriangleSurfacef;
class StageOneShader;
class ChunkShaderPicking;

namespace Model {
	class ChunkBlocks;
}

using std::shared_ptr;

namespace View {
	class ChunkObject;

/// Definition of the View side of a chunk.
/// @todo This started as a struct for data, but methods were added. The public data should be made private.
class Chunk {
public:
	ChunkCoord cc;		// The chunk coordinate of this chunk

	bool fScheduledForComputation;
	bool fScheduledForLoading;

	/// Information about graphical objects.
	/// The referenced object is a 'const', as the content must not change asynchronosuly.
	shared_ptr<const ChunkObject> fChunkObject;

	/// The actual blocks in the chunk. The content may change asynchronously, so it can't be a const.
	shared_ptr<Model::ChunkBlocks> fChunkBlocks;

	/// OpenGL data. One Vertex Array Object for each block type. Usually, only a limited num ber of the block
	/// types are needed.
	GLuint fBufferId[256];
	GLuint fVao[256];
	bool fBuffersDefined;

	Chunk();

	void Uncompress();

	/// x is east/west, y is north/south and z is height in a chunk. The content may change asynchorously.
	unsigned char GetBlock(int x, int y, int z) const {
		return fChunkBlocks->fChunkData[(x*CHUNK_SIZE+y)*CHUNK_SIZE+z];
	}

	/// Given a coordinate, find the chunk at this place and the block in that chunk.
	static unsigned char GetChunkAndBlock(signed long long x, signed long long y, signed long long z);

	~Chunk();

	/// The outer graphical presentation of a chunk depends on the neighbor chunk blocks.
	/// This function will tell all near chunk to update.
	void UpdateNeighborChunks(void);

	/// Interpret the binary chunk definition and transform it into graphical objects. OpenGL is done elsewhere.
	void UpdateGraphics(void);

	/// Draw the chunk itself
	void Draw(StageOneShader *shader, ChunkShaderPicking *pickShader, DL_Type dlType);

	/// Draw a bounding box for a chunk. Note, 'this' pointer can be zero, in which case
	/// a rough approximation has to be used for the bounding box.
	void DrawBoundingBox(StageOneShader *shader, int dx, int dy, int dz);

	/// Draw all objects in the chunk. If 'forShadows' is true, skip doing things that shall not be
	/// used in the shadow map. dx, dy and dz is the relative distance to the player chunk.
	void DrawObjects(StageOneShader *shader, int dx, int dy, int dz, bool forShadows);
	void PrepareOpenGL(StageOneShader *shader, ChunkShaderPicking *pickShader, DL_Type dlType);
	void ReleaseOpenGLBuffers(void);

	/// Unload all old chunks, and then move all chunks from the current busy list to the old busy list
	static void DegradeBusyList_gl(void);
	void SetDirty(bool flag);             // Update the fDirty flag
	bool IsDirty(void) const { return fDirty; }
	static void MakeAllChunksDirty(void); // Force all chunk to be recomputed.
	/// Stash away the triangles to enable a quick restore, and use a new one list
	void PushTriangles(shared_ptr<const ChunkObject>);
	/// Restore the pushed triangles
	void PopTriangles(void);

	bool InSunLight(int ox, int oy, int oz) const;
	float ComputeAmbientLight(int ox, int oy, int oz) const;
private:
	bool fDirty;						  // True if something changed so that the graphics need to be recomputed
	Chunk *fNext_gl;		// Used for linked list of chunks that need to free gl resources.
	Chunk **fPrev_gl;		// The previous item in the list.
	static Chunk *sfBusyList_gl; // The linked list of chunks that was used in the last draw operation.
	static Chunk *sfBusyListOld_gl; // The linked list of chunks with active OpenGL buffers, but not use for drawing currently.
	shared_ptr<const ChunkObject> fPushedValues;
};

}

/// Find an old Chunk.
/// If it doesn't exist yet and force is true, data will be requested from
/// the server. The content will be empty for a little while. If 'force' is false, a null pointer
/// will be returned if the Chunk isn't found.
extern View::Chunk* ChunkFind(const ChunkCoord *coord, bool force);
