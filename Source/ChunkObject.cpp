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

#include <memory.h>
#include <set>

#include "primitives.h"
#include "ChunkObject.h"
#include "chunk.h"
#include "modes.h"
#include "Options.h"
#include <glm/gtc/noise.hpp>
#include <glm/gtx/closest_point.hpp>

//
// All functions in this file must be thread safe. They will be called from more than one thread. The class ChunkObject need not be
// thread safe in itself.
//

ChunkObject::ChunkObject() {
	// Initialize the boundaries to the extreme limits.
	fBoundXMin = CHUNK_SIZE; fBoundXMax = 0; fBoundYMin = CHUNK_SIZE; fBoundYMax = 0; fBoundZMin = CHUNK_SIZE; fBoundZMax = 0;
	fNumTree = 0;
	fNumLamps = 0;
	fNumFogs = 0;
	fNumTreasures = 0;
	fChunk = 0;
}

unique_ptr<ChunkObject> ChunkObject::Make(const chunk *cp, bool pickingMode, int pdx, int pdy, int pdz) {
	unique_ptr<ChunkObject> co(new ChunkObject);
	co->FindTriangles(cp, pickingMode, pdx, pdy, pdz, gOptions.fSmoothTerrain, gOptions.fMergeNormals, gOptions.fAddNoise);
	return co;
}

bool ChunkObject::Empty(void) const {
	// If the boundary box doesn't exist, then there are no graphical objects
	return fBoundXMax < fBoundXMin;
}

// If the coordinate being analyzed is on chunk border, the adjacent chunks need to be queried. That
// is the purpose of 'neighborChunks'.
static void FillCube(unsigned char cube[][2][2], int x, int y, int z, const chunk *neighborChunks[][3][3]) {
	for (int dx=-1; dx<1; dx++) for (int dy=-1; dy<1; dy++) for (int dz=-1; dz<1; dz++) {
				int nx = x+dx, ny = y+dy, nz = z+dz;
				int cx=1, cy=1, cz = 1; // Used to address chunk

				// Update chunk selection (c*) and chunk offset (n*) if at border.
				if (nx == -1) {
					cx = 0;
					nx = CHUNK_SIZE-1;
				} else if (nx == CHUNK_SIZE) {
					cx = 2;
					nx = 0;
				}
				if (ny == -1) {
					cy=0;
					ny = CHUNK_SIZE-1;
				} else if (ny == CHUNK_SIZE) {
					cy = 2;
					ny = 0;
				}
				if (nz == -1) {
					cz = 0;
					nz = CHUNK_SIZE-1;
				} else if (nz == CHUNK_SIZE) {
					cz = 2;
					nz = 0;
				}
				const chunk *cp = neighborChunks[cx][cy][cz];
				if (cp == 0)
					cube[dx+1][dy+1][dz+1] = BT_Air; // Chunk isn't loaded yet.
				else
					cube[dx+1][dy+1][dz+1] = cp->GetBlock(nx, ny, nz);
			}
}

// Same as FillCube, but transformed from an X point of view.
static void FillCubeX(const unsigned char srcCube[][2][2], unsigned char destCube[][2][2]) {
	for (int dx=-1; dx<1; dx++) for (int dy=-1; dy<1; dy++) for (int dz=-1; dz<1; dz++)
				destCube[dy+1][dz+1][dx+1] = srcCube[dx+1][dy+1][dz+1];
}

// Same as FillCube, but transformed from an Y point of view.
static void FillCubeY(const unsigned char srcCube[][2][2], unsigned char destCube[][2][2]) {
	for (int dx=-1; dx<1; dx++) for (int dy=-1; dy<1; dy++) for (int dz=-1; dz<1; dz++)
				destCube[-dx][dz+1][dy+1] = srcCube[dx+1][dy+1][dz+1];
}

// Compute neighbor effects. Algorithm is using number of non transparent blocks in upper layer subtracted by number
// of transparent blocks in lower layer. This was not trivial to design.
// The return value is a number between -4 and +4.
static int height(unsigned char cube[][2][2]) {
	int countEmptyUp = 0, countEmptyDown = 0;
	for (int dx=-1; dx<1; dx++) for (int dy=-1; dy<1; dy++) {
			if (ChunkBlocks::blockIsSemiTransp(cube[dx+1][dy+1][1]))
				countEmptyUp++;
			if (ChunkBlocks::blockIsSemiTransp(cube[dx+1][dy+1][0]))
				countEmptyDown++;

		}
	if (countEmptyUp == 4 && countEmptyDown == 4)
		return 0;
	if (countEmptyUp == 4)
		return -countEmptyDown; // Totally empty upper side
	if (countEmptyDown == 4)
		return countEmptyUp;    // Totally empty lower side
	if ((countEmptyUp == 3 && countEmptyDown == 2) || (countEmptyUp == 2 && countEmptyDown == 3))
		return countEmptyDown - countEmptyUp; // Special case
	if (countEmptyDown == 0)
		return 4 - countEmptyUp;
	if (countEmptyUp == 0)
		return countEmptyDown - 4;
	return countEmptyUp - countEmptyDown;
}

// Examine the 8 near blocks and make sure there is no rigid neighbor
static bool rigidNeighbor(unsigned char cube[][2][2]) {
	for (int dx=0; dx<2; dx++) for (int dy=0; dy<2; dy++) for (int dz=0; dz<2; dz++) {
				if (ChunkBlocks::rigid(cube[dx][dy][dz]))
					return true;
			}
	return false;
}

// Examine the 8 near blocks and make sure there is no water
static bool waterNeighbor(unsigned char cube[][2][2]) {
	for (int dx=0; dx<2; dx++) for (int dy=0; dy<2; dy++) for (int dz=0; dz<2; dz++) {
				if (cube[dx][dy][dz] == BT_Water || cube[dx][dy][dz] == BT_BrownWater)
					return true;
			}
	return false;
}

// Find the delta offset of one single coorinate in a chunk. This depends on adjacent coordinates.
// The adjustment is done in Ephenation coordinates, not in OpenGL coordinates.
static glm::vec3 GetDelta(int x, int y, int z, bool addNoise, const chunk *neighborChunks[][3][3]) {
	glm::vec3 ret(0.0f);

	// This scales how big the delta can be.
	const float FACT = 1.0f / 3.0f / 3.0f;

	// When analyzing a vertex, the 8 surrounding vertices have to be considered. Copy data into
	// 'cube', to make it easy to apply the same algorithm every time.
	unsigned char cube[2][2][2];
	FillCube(cube, x, y, z, neighborChunks); // Initialize the 'cube' with the 8 vertices around x,y,z.

	// Special case: There are some rigid block types that are not supposed to be adjusted, e.g. bricks.
	// If any of the neighbor blocks is of this type, then the current vertice shall not be adjusted.
	if (rigidNeighbor(cube))
		return ret;
	bool water = waterNeighbor(cube);
	// Special case: When any of the neighbor blocks is of water type, do not adjust height. Otherwise,
	// there would be sloping water blocks.
	if (!water)
		ret.z = float(height(cube)) * FACT; // Do not adjust z if water

	// Make another cube for adjusting in x instead.
	unsigned char cube2[2][2][2];
	FillCubeX(cube, cube2); // Copy the same cube data, but transform 'z' for 'x'.
	ret.x = float(height(cube2)) * FACT;

	// Make another cube for adjusting in y instead.
	FillCubeY(cube, cube2);
	ret.y = float(height(cube2)) * FACT;

	if (addNoise && !water) {
		// Use a 2D simplex noise to get a random height delta. It shall depend on the world coordinate, to
		// make sure it is always the same.
		const chunk *cp = neighborChunks[1][1][1];
		glm::dvec2 pos(double(cp->cc.x)*CHUNK_SIZE + x, double(cp->cc.y)*CHUNK_SIZE + y);
		auto noise = glm::simplex(pos/4.3);
		ret.z += noise/6.0;
	}

	return ret;
}

// Compute all delta adjustments for each coordinate in the chunk.
// The world consists of blocks, which all have the same size. Every face of a block is defined by four vertex coordinates.
// To get a smooth world, the coordinates are adjusted with a delta. The delta is computed for each of the three dimensions.
// If 'addNoise' is true, some simplex noise is added. This is an expensive computation.
// 'neighborChunks' is pointers to adjacent chunks. This is needed, as delta computation depends on adjacent points.
// The 'delta' is larger than a standard chunk, to allow for one neighbor block.
static void ComputeDelta(glm::vec3 delta[][CHUNK_SIZE+1][CHUNK_SIZE+1], bool addNoise, const chunk *neighborChunks[][3][3]) {
	// In construction mode, leave the world as blocky. This makes it easier to build.
	if (gMode.Get() == GameMode::CONSTRUCT)
		return;

	for (int x=0; x<CHUNK_SIZE+1; x++) for (int y=0; y<CHUNK_SIZE+1; y++) for (int z=0; z<CHUNK_SIZE+1; z++) {
				delta[x][y][z] = GetDelta(x, y, z, addNoise, neighborChunks);
			}
}

struct CompareVertexDataf {
	bool operator()(VertexDataf*a, VertexDataf*b) {
		return a->LessVertex(b);
	}
};

//
// Iterate over all vertices and replace normals with the average value for all normals sharing the
// same vertex. OpenGL will use nice interpolations to produce a nice curve-looking view on surfaces.
static void MergeNormals(std::vector<TriangleSurfacef> &b) {
	typedef std::multiset<VertexDataf*, CompareVertexDataf> srt;

	unsigned size = 3 * b.size();
	if (size == 0) return;
	VertexDataf *v = &b[0].v[0]; // Set v to point to the first vertex

	srt sorted;
	for (unsigned j=0; j<size; j++) {
		sorted.insert(&v[j]);
	}

	// Iterate over each triangle that can potentially have merged normals with other triangles. There is
	// no need to test the last one, as it has either already been merged with another one, or is the only one
	// for that vertex.
	srt::iterator first = sorted.begin(), last = first;
	// Find first and last vertex with the same
	int count = 0;
	glm::vec3 sum;
	for (; first != sorted.end();) {
		sum += (*last)->GetNormal();
		last++; count++;
		if (last != sorted.end()) {
			glm::vec3 f = (*first)->GetVertex();
			glm::vec3 l = (*last)->GetVertex();
			if (f == l) continue;
		}
		// All vertices from 'first' to (and not including) 'last', have the same coordinate.
		if (count > 1 && sum != glm::vec3(0,0,0)) { // Ignore null vectors
			sum /= count; // Compute the average
			sum = glm::normalize(sum);
			// Update all vertices with the new normal
			for (; first != last; first++) {
				(*first)->SetNormal(sum);
			}
		}
		first = last;
		count = 0;
		sum = glm::vec3(0,0,0);
	}
}

// Given the verices for a triangle, compute the normals.
static void ComputeNormals(TriangleSurfacef &tri1, TriangleSurfacef &tri2) {
	tri1.v[0].SetNormal(glm::normalize(glm::cross(tri1.v[1].GetVertex() - tri1.v[0].GetVertex(), tri1.v[2].GetVertex() - tri1.v[0].GetVertex())));
	tri1.v[1].SetNormal(tri1.v[0].GetNormal());
	tri1.v[2].SetNormal(tri1.v[0].GetNormal());
	tri2.v[0].SetNormal(glm::normalize(glm::cross(tri2.v[1].GetVertex() - tri2.v[0].GetVertex(), tri2.v[2].GetVertex() - tri2.v[0].GetVertex())));
	tri2.v[2].SetNormal(tri2.v[0].GetNormal());
	tri2.v[1].SetNormal(tri2.v[0].GetNormal());
}

// In picking mode, the normals are used to find what surface was selected.
static void ComputePickingNormals(TriangleSurfacef &tri1, TriangleSurfacef &tri2, PickingData coding) {
	tri1.v[0].SetNormal(coding);
	tri1.v[1].SetNormal(coding);
	tri1.v[2].SetNormal(coding);
	tri2.v[0].SetNormal(coding);
	tri2.v[1].SetNormal(coding);
	tri2.v[2].SetNormal(coding);
}

// Compute 6 UV coordinates (two triangles), the triangles are defined by a quad
// of 4 vertices (v0-v3).
//
// v3  v2
//
// v0  v1
//
// T1=v0,v2,v3
// T2=v0,v1,v2
#if 0
static void computeUV(glm::vec2 &t1p0, glm:: vec2 &t1p2, glm:: vec2 &t1p3,
                      glm:: vec2 &t2p0, glm:: vec2 &t2p1, glm:: vec2 &t2p2,
                      glm::vec3 const &v0, glm::vec3 const &v1, glm::vec3 const &v2, glm::vec3 const &v3) {
	glm::vec3 x1 = glm::closestPointOnLine(v2, v0, v1); // x1 will be near v1

	t1p0 = t2p0 = glm::vec2(0,0);
	t2p1 = glm::vec2(glm::distance(v0, v1), 0);
	t1p2 = t2p2 = glm::vec2(glm::distance(v0, x1), glm::distance(x1, v2));

	glm::vec3 x3 = glm::closestPointOnLine(v0, v3, v2); // x3 will be near v3

	t1p3 = glm::vec2(glm::distance(v3, x3), glm::distance(v0, x3));
	// std::cout << t1p0.x << t1p0.y << std::endl;
}
#endif

static bool MostlyFacingUpwards(const VertexDataf &v) {
	return v.GetNormal().y > 0.4f;
}

#define MAX_SUN_INTENSITY 	230		// 0-255. Don't saturate, to allow a small contribution from ambient light

// Create an array of struct TriangleSurfacef for each block type. The arguments are used for picking mode.
// The reason that the chunk pointer is a const is that nothing is allowed be changed in it. This is because this update function
// is done in a separate process.
// 'addNoise' will add some random height, but it is an expensive function.
void ChunkObject::FindTriangles(const chunk *cp, bool pickingMode, int pdx, int pdy, int pdz, bool smoothing, bool mergeNormals, bool addNoise) {
	PickingData coding;
	// dx, dy and dz are in the interval [-1,1]. Move them to the interval [0,2]
	coding.bitmap.dx = pdx+1; coding.bitmap.dy = pdy+1; coding.bitmap.dz = pdz+1;
	// Find all neighbor chunks, if they exist. This will speed up the next phase.
	const chunk *neighbor[3][3][3]; // Const, as they are not allowed to be modified.

	for (int dx=-1; dx<2; dx++) for (int dy=-1; dy<2; dy++) for (int dz=-1; dz<2; dz++) {
				if (dx == 0 && dy == 0 && dz == 0) {
					neighbor[1][1][1] = cp; // The chunk pointer in the middle is already known.
					continue;
				}
				ChunkCoord cc = cp->cc;
				cc.x = cp->cc.x + dx;
				cc.y = cp->cc.y + dy;
				cc.z = cp->cc.z + dz;
				neighbor[dx+1][dy+1][dz+1] = ChunkFind(&cc, false); // May be initialized empty or a null pointer
			}

	// A table that defines a delta movement of each coordinate in the chunk. This is an adjustment done to make
	// the world less cubic.
	// The delta table also need to contain adjacent points (+1), that originate from adjacent chunks. For now,
	// they are always 0.
	glm::vec3 delta[CHUNK_SIZE+1][CHUNK_SIZE+1][CHUNK_SIZE+1];
	if (smoothing)
		ComputeDelta(delta, addNoise, neighbor);

	std::vector<TriangleSurfacef> b[256]; // One list for each block type
	for (int z=0; z<CHUNK_SIZE; z++) for (int x=0; x<CHUNK_SIZE; x++) for (int y=0; y<CHUNK_SIZE; y++) {
				coding.bitmap.x = x; coding.bitmap.y = y; coding.bitmap.z = z;
				// NOTICE: in the game server, 'z' is the height, but in OpenGL, 'y' is the height. Because of that,
				// 'z' and 'y' are changed with each other when creating OpenGL graphics from server coordinates.
				int bl = cp->GetBlock(x, y, z);
				if (cp->fChunkBlocks->blockIsComplTransp(bl))
					continue;	// Nothing is drawn from air

				// Update the boundary box. Special care has to be taken because of lamps shining outside of the chunk
				// normal borders, and fogs also reaching farther.
				switch(bl) {
				case BT_Lamp1:
				case BT_SmallFog:
					if (x-LAMP1_DIST < fBoundXMin) fBoundXMin = x-LAMP1_DIST;
					if (y-LAMP1_DIST < fBoundYMin) fBoundYMin = y-LAMP1_DIST;
					if (z-LAMP1_DIST < fBoundZMin) fBoundZMin = z-LAMP1_DIST;
					if (x+LAMP1_DIST > fBoundXMax) fBoundXMax = x+LAMP1_DIST;
					if (y+LAMP1_DIST > fBoundYMax) fBoundYMax = y+LAMP1_DIST;
					if (z+LAMP1_DIST > fBoundZMax) fBoundZMax = z+LAMP1_DIST;
					break;
				case BT_Lamp2:
				case BT_BigFog:
					if (x-LAMP2_DIST < fBoundXMin) fBoundXMin = x-LAMP2_DIST;
					if (y-LAMP2_DIST < fBoundYMin) fBoundYMin = y-LAMP2_DIST;
					if (z-LAMP2_DIST < fBoundZMin) fBoundZMin = z-LAMP2_DIST;
					if (x+LAMP2_DIST > fBoundXMax) fBoundXMax = x+LAMP2_DIST;
					if (y+LAMP2_DIST > fBoundYMax) fBoundYMax = y+LAMP2_DIST;
					if (z+LAMP2_DIST > fBoundZMax) fBoundZMax = z+LAMP2_DIST;
					break;
				default:
					if (x < fBoundXMin) fBoundXMin = x;
					if (y < fBoundYMin) fBoundYMin = y;
					if (z < fBoundZMin) fBoundZMin = z;
					if (x > fBoundXMax) fBoundXMax = x;
					if (y > fBoundYMax) fBoundYMax = y;
					if (z > fBoundZMax) fBoundZMax = z;
					break;
				}
				int blxm1, blxp1, blym1, blyp1, blzm1, blzp1; // The nearest blocks to x,y,z. blxm1 means "block at x minus 1".

				// Find all adjacent blocks, in the 6 directions. It can be a hard if the block is in another chunk,
				// in which case is is assumed that the adjacent block is air if the neighbor chunk is not found.
				// This can lead to a lot of squares being drawn, but as soon as the neighbor is found there will be an update.
				if (x>0) blxm1 = cp->GetBlock(x-1, y, z);
				else if (neighbor[0][1][1]) blxm1 = neighbor[0][1][1]->GetBlock(CHUNK_SIZE-1,y,z);
				else blxm1 = BT_Air;

				if (x<CHUNK_SIZE-1) blxp1 = cp->GetBlock(x+1, y, z);
				else if (neighbor[2][1][1]) blxp1 = neighbor[2][1][1]->GetBlock(0,y,z);
				else blxp1 = BT_Air;

				if (y>0) blym1 = cp->GetBlock(x, y-1, z);
				else if (neighbor[1][0][1]) blym1 = neighbor[1][0][1]->GetBlock(x,CHUNK_SIZE-1,z);
				else blym1 = BT_Air;

				if (y<CHUNK_SIZE-1) blyp1 = cp->GetBlock(x, y+1, z);
				else if (neighbor[1][2][1]) blyp1 = neighbor[1][2][1]->GetBlock(x,0,z);
				else blyp1 = BT_Air;

				if (z>0) blzm1 = cp->GetBlock(x, y, z-1);
				else if (neighbor[1][1][0]) blzm1 = neighbor[1][1][0]->GetBlock(x,y,CHUNK_SIZE-1);
				else blzm1 = BT_Air;

				if (z<CHUNK_SIZE-1) blzp1 = cp->GetBlock(x, y, z+1);
				else if (neighbor[1][1][2]) blzp1 = neighbor[1][1][2]->GetBlock(x,y,0);
				else blzp1 = BT_Air;

				TriangleSurfacef tri1, tri2; // Need two triangles for each square
				tri1.v[0].SetIntensity(255); tri1.v[1].SetIntensity(255); tri1.v[2].SetIntensity(255);
				tri2.v[0].SetIntensity(255); tri2.v[1].SetIntensity(255); tri2.v[2].SetIntensity(255);

				// Some helper variables that are used frequently
				auto d000 = delta[x][y][z], d001 = delta[x][y][z+1], d010 = delta[x][y+1][z], d011 = delta[x][y+1][z+1];
				auto d100 = delta[x+1][y][z], d101 = delta[x+1][y][z+1], d110 = delta[x+1][y+1][z], d111 = delta[x+1][y+1][z+1];

				// If dynamic shadows are used, the sun flag is only used to enable shadows in the renderer.
				bool forceSunFlag = (gOptions.fDynamicShadows || gOptions.fStaticShadows) && cp->cc.z >= -1;

				// This block is not water and not air, check if near sides are visible from the outside
				// TODO: Lot of repetitive things here, should be possible to generalize and compress.
				// The rule used to decide if a face shall be drawn is:
				// * The adjacent block must be at least semi transparent, or noone could see the face
				// * If the current block is also semi transparent, it must not be of the same type (e.g. don't draw faces between water blocks).
				if (ChunkBlocks::blockIsSemiTransp(blxm1) && (!ChunkBlocks::blockIsSemiTransp(bl) || bl != blxm1)) { // Left face
					if (pickingMode) {
						coding.bitmap.facing = CH_LeftFace;
						ComputePickingNormals(tri1, tri2, coding);
					}

					tri1.v[0].SetVertex(x+d010.x, z+d010.z, -y-1-d010.y);
					tri1.v[0].SetTexture(1+d010.y, d010.z);
					tri2.v[0] = tri1.v[0];

					tri1.v[1].SetVertex(x+d001.x, z+1+d001.z, -y-d001.y);
					tri1.v[1].SetTexture(d001.y, 1+d001.z);

					tri1.v[2].SetVertex(x+d011.x, z+1+d011.z, -y-1-d011.y);
					tri1.v[2].SetTexture(1+d011.y, 1+d011.z);
					tri2.v[2] = tri1.v[1];

					tri2.v[1].SetVertex(x+d000.x, z+d000.z, -y-d000.y);
					tri2.v[1].SetTexture(d000.y, d000.z);

					if (!pickingMode && !forceSunFlag && !cp->InSunLight(x-1, y, z)) {
						tri1.v[0].SetIntensity(0); tri1.v[1].SetIntensity(0); tri1.v[2].SetIntensity(0);
						tri2.v[0].SetIntensity(0); tri2.v[1].SetIntensity(0); tri2.v[2].SetIntensity(0);
					} else {
						tri1.v[0].SetIntensity(MAX_SUN_INTENSITY); tri1.v[1].SetIntensity(MAX_SUN_INTENSITY); tri1.v[2].SetIntensity(MAX_SUN_INTENSITY);
						tri2.v[0].SetIntensity(MAX_SUN_INTENSITY); tri2.v[1].SetIntensity(MAX_SUN_INTENSITY); tri2.v[2].SetIntensity(MAX_SUN_INTENSITY);
					}

					if (!pickingMode) {
						ComputeNormals(tri1, tri2);
						unsigned char ambient = (unsigned char)(255 * cp->ComputeAmbientLight(x-1, y, z) + 0.5f);
						tri1.v[0].SetAmbient(ambient); tri1.v[1].SetAmbient(ambient); tri1.v[2].SetAmbient(ambient);
						tri2.v[0].SetAmbient(ambient); tri2.v[1].SetAmbient(ambient); tri2.v[2].SetAmbient(ambient);
					}

					if (bl == BT_Soil && MostlyFacingUpwards(tri1.v[0])) {
						b[BT_TopSoil].push_back(tri1);
						b[BT_TopSoil].push_back(tri2);
					} else {
						b[bl].push_back(tri1);
						b[bl].push_back(tri2);
					}
				}
				if (ChunkBlocks::blockIsSemiTransp(blym1) && (!ChunkBlocks::blockIsSemiTransp(bl) || bl != blym1)) { // Front face
					if (pickingMode) {
						coding.bitmap.facing = CH_FrontFace;
						ComputePickingNormals(tri1, tri2, coding);
					}

					tri1.v[0].SetVertex(x+d000.x, z+d000.z, -y-d000.y);
					tri1.v[0].SetTexture(d000.x, d000.z);
					tri2.v[0] = tri1.v[0];

					tri1.v[1].SetVertex(x+1+d101.x, z+1+d101.z, -y-d101.y);
					tri1.v[1].SetTexture(1+d101.x, 1+d101.z);

					tri1.v[2].SetVertex(x+d001.x, z+1+d001.z, -y-d001.y);
					tri1.v[2].SetTexture(d001.x, 1+d001.z);
					tri2.v[2] = tri1.v[1];

					tri2.v[1].SetVertex(x+1+d100.x, z+d100.z, -y-d100.y);
					tri2.v[1].SetTexture(1+d100.x, d100.z);

					if (!pickingMode && !forceSunFlag && !cp->InSunLight(x, y-1, z)) {
						tri1.v[0].SetIntensity(0); tri1.v[1].SetIntensity(0); tri1.v[2].SetIntensity(0);
						tri2.v[0].SetIntensity(0); tri2.v[1].SetIntensity(0); tri2.v[2].SetIntensity(0);
					} else {
						tri1.v[0].SetIntensity(MAX_SUN_INTENSITY); tri1.v[1].SetIntensity(MAX_SUN_INTENSITY); tri1.v[2].SetIntensity(MAX_SUN_INTENSITY);
						tri2.v[0].SetIntensity(MAX_SUN_INTENSITY); tri2.v[1].SetIntensity(MAX_SUN_INTENSITY); tri2.v[2].SetIntensity(MAX_SUN_INTENSITY);
					}

					if (!pickingMode) {
						ComputeNormals(tri1, tri2);
						unsigned char ambient = (unsigned char)(255 * cp->ComputeAmbientLight(x, y-1, z) + 0.5f);
						tri1.v[0].SetAmbient(ambient); tri1.v[1].SetAmbient(ambient); tri1.v[2].SetAmbient(ambient);
						tri2.v[0].SetAmbient(ambient); tri2.v[1].SetAmbient(ambient); tri2.v[2].SetAmbient(ambient);
					}

					if (bl == BT_Soil && MostlyFacingUpwards(tri1.v[0])) {
						b[BT_TopSoil].push_back(tri1);
						b[BT_TopSoil].push_back(tri2);
					} else {
						b[bl].push_back(tri1);
						b[bl].push_back(tri2);
					}
				}
				if (ChunkBlocks::blockIsSemiTransp(blzm1) && (!ChunkBlocks::blockIsSemiTransp(bl) || bl != blzm1)) { // Bottom face
					if (pickingMode) {
						coding.bitmap.facing = CH_BottomFace;
						ComputePickingNormals(tri1, tri2, coding);
					}

					tri1.v[0].SetVertex(x+d010.x, z+d010.z, -y-1-d010.y);
					tri1.v[0].SetTexture(d010.x, -1-d010.y);
					tri2.v[0] = tri1.v[0];

					tri1.v[1].SetVertex(x+1+d100.x, z+d100.z, -y-d100.y);
					tri1.v[1].SetTexture(1+d100.x, -d100.y);

					tri1.v[2].SetVertex(x+d000.x, z+d000.z, -y-d000.y);
					tri1.v[2].SetTexture(d000.x, -d000.y);
					tri2.v[2] = tri1.v[1];

					tri2.v[1].SetVertex(x+1+d110.x, z+d110.z, -y-1-d110.y);
					tri2.v[1].SetTexture(1+d110.x, -1-d110.y);

					if (!pickingMode && !forceSunFlag && !cp->InSunLight(x, y, z-1)) {
						tri1.v[0].SetIntensity(0); tri1.v[1].SetIntensity(0); tri1.v[2].SetIntensity(0);
						tri2.v[0].SetIntensity(0); tri2.v[1].SetIntensity(0); tri2.v[2].SetIntensity(0);
					} else {
						tri1.v[0].SetIntensity(MAX_SUN_INTENSITY); tri1.v[1].SetIntensity(MAX_SUN_INTENSITY); tri1.v[2].SetIntensity(MAX_SUN_INTENSITY);
						tri2.v[0].SetIntensity(MAX_SUN_INTENSITY); tri2.v[1].SetIntensity(MAX_SUN_INTENSITY); tri2.v[2].SetIntensity(MAX_SUN_INTENSITY);
					}

					if (!pickingMode) {
						ComputeNormals(tri1, tri2);
						unsigned char ambient = (unsigned char)(255 * cp->ComputeAmbientLight(x, y, z-1) + 0.5f);
						tri1.v[0].SetAmbient(ambient); tri1.v[1].SetAmbient(ambient); tri1.v[2].SetAmbient(ambient);
						tri2.v[0].SetAmbient(ambient); tri2.v[1].SetAmbient(ambient); tri2.v[2].SetAmbient(ambient);
					}

					b[bl].push_back(tri1);
					b[bl].push_back(tri2);
				}
				if (ChunkBlocks::blockIsSemiTransp(blxp1) && (!ChunkBlocks::blockIsSemiTransp(bl) || bl != blxp1)) { // Right face
					if (pickingMode) {
						coding.bitmap.facing = CH_RightFace;
						ComputePickingNormals(tri1, tri2, coding);
					}

					tri1.v[0].SetVertex(x+1+d100.x, z+d100.z, -y-d100.y);
					tri1.v[0].SetTexture(d100.y, d100.z);
					tri2.v[0] = tri1.v[0];

					tri1.v[1].SetVertex(x+1+d111.x, z+1+d111.z, -y-1-d111.y);
					tri1.v[1].SetTexture(1+d111.y, 1+d111.z);

					tri1.v[2].SetVertex(x+1+d101.x, z+1+d101.z, -y-d101.y);
					tri1.v[2].SetTexture(d101.y, 1+d101.z);

					tri2.v[2] = tri1.v[1];
					tri2.v[1].SetVertex(x+1+d110.x, z+d110.z, -y-1-d110.y);
					tri2.v[1].SetTexture(1+d110.y, d110.z);

					if (!pickingMode && !forceSunFlag && !cp->InSunLight(x+1, y, z)) {
						tri1.v[0].SetIntensity(0); tri1.v[1].SetIntensity(0); tri1.v[2].SetIntensity(0);
						tri2.v[0].SetIntensity(0); tri2.v[1].SetIntensity(0); tri2.v[2].SetIntensity(0);
					} else {
						tri1.v[0].SetIntensity(MAX_SUN_INTENSITY); tri1.v[1].SetIntensity(MAX_SUN_INTENSITY); tri1.v[2].SetIntensity(MAX_SUN_INTENSITY);
						tri2.v[0].SetIntensity(MAX_SUN_INTENSITY); tri2.v[1].SetIntensity(MAX_SUN_INTENSITY); tri2.v[2].SetIntensity(MAX_SUN_INTENSITY);
					}

					if (!pickingMode) {
						ComputeNormals(tri1, tri2);
						unsigned char ambient = (unsigned char)(255 * cp->ComputeAmbientLight(x+1, y, z) + 0.5f);
						tri1.v[0].SetAmbient(ambient); tri1.v[1].SetAmbient(ambient); tri1.v[2].SetAmbient(ambient);
						tri2.v[0].SetAmbient(ambient); tri2.v[1].SetAmbient(ambient); tri2.v[2].SetAmbient(ambient);
					}

					if (bl == BT_Soil && MostlyFacingUpwards(tri1.v[0])) {
						b[BT_TopSoil].push_back(tri1);
						b[BT_TopSoil].push_back(tri2);
					} else {
						b[bl].push_back(tri1);
						b[bl].push_back(tri2);
					}
				}
				if (ChunkBlocks::blockIsSemiTransp(blyp1) && (!ChunkBlocks::blockIsSemiTransp(bl) || bl != blyp1)) { // Back face
					if (pickingMode) {
						coding.bitmap.facing = CH_BackFace;
						ComputePickingNormals(tri1, tri2, coding);
					}

					tri1.v[0].SetVertex(x+1+d110.x, z+d110.z, -y-1-d110.y);
					tri1.v[0].SetTexture(-1-d110.x, d110.z);
					tri2.v[0] = tri1.v[0];

					tri1.v[1].SetVertex(x+d011.x, z+1+d011.z, -y-1-d011.y);
					tri1.v[1].SetTexture(-d011.x, 1+d011.z);

					tri1.v[2].SetVertex(x+1+d111.x, z+1+d111.z, -y-1-d111.y);
					tri1.v[2].SetTexture(-1-d111.x, 1+d111.z);

					tri2.v[2] = tri1.v[1];
					tri2.v[1].SetVertex(x+d010.x, z+d010.z, -y-1-d010.y);
					tri2.v[1].SetTexture(-d010.x, d010.z);

					if (!pickingMode && !forceSunFlag && !cp->InSunLight(x, y+1, z)) {
						tri1.v[0].SetIntensity(0); tri1.v[1].SetIntensity(0); tri1.v[2].SetIntensity(0);
						tri2.v[0].SetIntensity(0); tri2.v[1].SetIntensity(0); tri2.v[2].SetIntensity(0);
					} else {
						tri1.v[0].SetIntensity(MAX_SUN_INTENSITY); tri1.v[1].SetIntensity(MAX_SUN_INTENSITY); tri1.v[2].SetIntensity(MAX_SUN_INTENSITY);
						tri2.v[0].SetIntensity(MAX_SUN_INTENSITY); tri2.v[1].SetIntensity(MAX_SUN_INTENSITY); tri2.v[2].SetIntensity(MAX_SUN_INTENSITY);
					}

					if (!pickingMode) {
						ComputeNormals(tri1, tri2);
						unsigned char ambient = (unsigned char)(255 * cp->ComputeAmbientLight(x, y+1, z) + 0.5f);
						tri1.v[0].SetAmbient(ambient); tri1.v[1].SetAmbient(ambient); tri1.v[2].SetAmbient(ambient);
						tri2.v[0].SetAmbient(ambient); tri2.v[1].SetAmbient(ambient); tri2.v[2].SetAmbient(ambient);
					}

					if (bl == BT_Soil && MostlyFacingUpwards(tri1.v[0])) {
						b[BT_TopSoil].push_back(tri1);
						b[BT_TopSoil].push_back(tri2);
					} else {
						b[bl].push_back(tri1);
						b[bl].push_back(tri2);
					}
				}
				if (ChunkBlocks::blockIsSemiTransp(blzp1) && (!ChunkBlocks::blockIsSemiTransp(bl) || bl != blzp1)) { // Top face
					if (pickingMode) {
						// if (x == 0 && z == 2) printf("(%d,%d,%d)", coding.rgb[0], coding.rgb[1], coding.rgb[2]);
						coding.bitmap.facing = CH_TopFace;
						ComputePickingNormals(tri1, tri2, coding);
					}

					tri1.v[0].SetVertex(x+d001.x, z+1+d001.z, -y-d001.y);
					tri1.v[0].SetTexture(d001.x, -d001.y);

					tri1.v[1].SetVertex(x+1+d111.x, z+1+d111.z, -y-1-d111.y);
					tri1.v[1].SetTexture(1+d111.x, -1-d111.y);

					tri1.v[2].SetVertex(x+d011.x, z+1+d011.z, -y-1-d011.y);
					tri1.v[2].SetTexture(d011.x, -1-d011.y);

					tri2.v[0] = tri1.v[0];
					tri2.v[2] = tri1.v[1];
					tri2.v[1].SetVertex(x+1+d101.x, z+1+d101.z, -y-d101.y);
					tri2.v[1].SetTexture(1+d101.x, -d101.y);
#if 0
					computeUV(tri1.v[0].fTexture, tri1.v[1].fTexture, tri1.v[2].fTexture,
					          tri2.v[0].fTexture, tri2.v[1].fTexture, tri2.v[2].fTexture,
					          tri1.v[0].fVertex, tri2.v[1].fVertex, tri1.v[1].fVertex, tri1.v[2].fVertex);
#endif
					if (!pickingMode && !forceSunFlag && !cp->InSunLight(x, y, z+1)) {
						tri1.v[0].SetIntensity(0); tri1.v[1].SetIntensity(0); tri1.v[2].SetIntensity(0);
						tri2.v[0].SetIntensity(0); tri2.v[1].SetIntensity(0); tri2.v[2].SetIntensity(0);
					} else {
						tri1.v[0].SetIntensity(MAX_SUN_INTENSITY); tri1.v[1].SetIntensity(MAX_SUN_INTENSITY); tri1.v[2].SetIntensity(MAX_SUN_INTENSITY);
						tri2.v[0].SetIntensity(MAX_SUN_INTENSITY); tri2.v[1].SetIntensity(MAX_SUN_INTENSITY); tri2.v[2].SetIntensity(MAX_SUN_INTENSITY);
					}

					if (!pickingMode) {
						ComputeNormals(tri1, tri2);
						unsigned char ambient = (unsigned char)(255 * cp->ComputeAmbientLight(x, y, z+1) + 0.5f);
						tri1.v[0].SetAmbient(ambient); tri1.v[1].SetAmbient(ambient); tri1.v[2].SetAmbient(ambient);
						tri2.v[0].SetAmbient(ambient); tri2.v[1].SetAmbient(ambient); tri2.v[2].SetAmbient(ambient);
					}

					// This is a special case. Replace the texture of the top soil with another block type (green grass).
					if (bl == BT_Soil && MostlyFacingUpwards(tri1.v[0])) {
						b[BT_TopSoil].push_back(tri1);
						b[BT_TopSoil].push_back(tri2);
					} else {
						b[bl].push_back(tri1);
						b[bl].push_back(tri2);
					}
				}
			}
	for (int i=0; i<256; i++) {
		if (b[i].size() > 0) {
			fVisibleTriangles[i] = std::move(b[i]);
			if (ChunkBlocks::rigid(i) || ChunkBlocks::blockIsSemiTransp(i))
				continue;
			if (mergeNormals && !pickingMode)
				MergeNormals(fVisibleTriangles[i]);
		} else {
			fVisibleTriangles[i].clear(); // Delete the previous one, if there was any.
		}
	}
}

void ChunkObject::FindSpecialObjects(const chunk *cp) {
	// First count, then allocate
	fNumTree = 0;
	fNumLamps = 0;
	fNumFogs = 0;
	fNumTreasures = 0;
	for (int x=0; x<CHUNK_SIZE; x++) for (int y=0; y<CHUNK_SIZE; y++) for (int z=0; z<CHUNK_SIZE; z++) {
				int bl = cp->GetBlock(x, y, z);
				switch(bl) {
				case BT_Tree1:
				case BT_Tree2:
				case BT_Tree3:
				case BT_Tuft:
				case BT_Flowers:
					fNumTree++;
					break;
				case BT_Lamp1:
				case BT_Lamp2:
					fNumLamps++;
					break;
				case BT_SmallFog:
				case BT_BigFog:
					fNumFogs++;
					break;
				case BT_Treasure:
				case BT_Quest:
					fNumTreasures++;
					break;
				}
			}
	if (fNumTree > 0)
		fTreeList.reset(new SpecialObject[fNumTree]);
	if (fNumLamps > 0)
		fLampList.reset(new SpecialObject[fNumLamps]);
	if (fNumFogs > 0)
		fFogList.reset(new SpecialObject[fNumFogs]);
	if (fNumTreasures > 0)
		fTreasureList.reset(new SpecialObject[fNumTreasures]);
	int treeInd = 0, lampInd = 0, fogInd = 0, treasureInd = 0;
	for (int x=0; x<CHUNK_SIZE; x++) for (int y=0; y<CHUNK_SIZE; y++) for (int z=0; z<CHUNK_SIZE; z++) {
				int bl = cp->GetBlock(x, y, z);
				switch(bl) {
				case BT_Tree1:
				case BT_Tree2:
				case BT_Tree3:
				case BT_Tuft:
				case BT_Flowers:
					fTreeList[treeInd].x = x;
					fTreeList[treeInd].y = y;
					fTreeList[treeInd].z = z;
					fTreeList[treeInd].type = bl;
					fTreeList[treeInd].ambient = cp->ComputeAmbientLight(x, y, z) + cp->InSunLight(x, y, z) * 0.15; // TODO: Not used
					treeInd++;
					break;
				case BT_Lamp1:
				case BT_Lamp2:
					fLampList[lampInd].x = x;
					fLampList[lampInd].y = y;
					fLampList[lampInd].z = z;
					fLampList[lampInd].type = bl;
					lampInd++;
					break;
				case BT_SmallFog:
				case BT_BigFog: {
					fFogList[fogInd].x = x;
					fFogList[fogInd].y = y;
					fFogList[fogInd].z = z;
					fFogList[fogInd].type = bl;
					float tmpamb = cp->ComputeAmbientLight(x, y, z)*0.25 + cp->InSunLight(x, y, z) * 0.75;
					fFogList[fogInd].ambient = tmpamb;
					fogInd++;
					break;
				}
				case BT_Treasure:
				case BT_Quest:
					fTreasureList[treasureInd].x = x;
					fTreasureList[treasureInd].y = y;
					fTreasureList[treasureInd].z = z;
					fTreasureList[treasureInd].type = bl;
					fTreasureList[treasureInd].ambient = cp->ComputeAmbientLight(x, y, z) + cp->InSunLight(x, y, z) * 0.15; // TODO: Not used
					treasureInd++;
					break;
				}
			}
}
