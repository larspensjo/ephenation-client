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

#include "primitives.h"

#include <memory>
#include <vector>

using std::unique_ptr;
struct TriangleSurfacef;

namespace View {
	class Chunk;

/// Manage visible attributes of a chunk.
/// This is part of the View.
/// A chunk is 32x32x32 blocks, which can't be used for drawing as it is. It is converted into two
/// types: visible blocks and special objects. It is the responsibility of this class to take the chunk
/// data and compute what can be drawn from the chunk.
class ChunkObject {
public:
	// The triangle list. There is one list for each block type, as they can't be drawn in
	// a single call (using different textures).
	// TODO: Should not be public
	std::vector<TriangleSurfacef> fVisibleTriangles[256];

	// A boundary box of the chunk
	char fBoundXMin, fBoundXMax, fBoundYMin, fBoundYMax, fBoundZMin, fBoundZMax;

	// Get the number of vertices for a block type.
	int VertexSize(int bl) const { return fVisibleTriangles[bl].size()*3; }

	// Create and initialize a ChunkObject from a raw chunk. 'pickingmode' is true if the data shall
	// be used for picking. 'pdx', etc, are used in picking mode to identify which chunk it is (relative the player).
	static unique_ptr<ChunkObject> Make(const View::Chunk *cp, bool pickingMode, int pdx, int pdy, int pdz);

	// True if there are no graphical objects to show in this chunk
	bool Empty(void) const;

	// To improve the look, many block types are not drawn as blocks, but as a more advanced 3D model (e.g. trees and lamps).
	void FindSpecialObjects(const View::Chunk *cp);

	// For each special object, save information about it.
	struct SpecialObject {
		float ambient;         // Amount of ambient light at this object
		unsigned char x, y, z; // The local coordinate of the tree
		unsigned char type;    // Block type
	};

	// TODO: These should be std::vector.
	// Manage objects found in the chunk. It is a static list, decoded when the chunk is loaded.
	// TODO: These should not be public
	unique_ptr<SpecialObject[]> fTreeList; int fNumTree;
	unique_ptr<SpecialObject[]> fLampList; int fNumLamps;
	unique_ptr<SpecialObject[]> fFogList; int fNumFogs;
	unique_ptr<SpecialObject[]> fTreasureList; int fNumTreasures;
	std::vector<SpecialObject> fSpecialObject; // The list of all special objects found in this chunk.

	// Cyclic pointer back to the chunk the data belongs to. TODO: can this be avoided?
	View::Chunk *fChunk;
private:
	// Code an array of struct TriangleSurfacef.
	// The arguments are used for selection mode.
	void FindTriangles(const View::Chunk *cp, bool pickingMode, int pdx, int pdy, int pdz, bool smoothing, bool mergeNormals, bool addNoise);

	ChunkObject(); // Private; only the Make() can create this object
};

}
