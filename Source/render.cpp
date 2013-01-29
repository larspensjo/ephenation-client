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

//
// Do the actual rendering of objects.
//

#if defined(_WIN32)
#include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glfw.h>
#include <set>

#include <glm/glm.hpp>
#include "primitives.h"
#include "render.h"
#include "player.h"
#include "msgwindow.h"
#include "chunk.h"
#include "textures.h"
#include "shapes/Cube.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shaders/ChunkShader.h"
#include "shaders/ChunkShaderPicking.h"
#include "modes.h"
#include "ChunkObject.h"
#include "ChunkProcess.h"
#include "Options.h"
#include "SuperChunkManager.h"
#include "SoundControl.h"

#define NELEM(x) (sizeof(x)/sizeof(x[0]))

using namespace View;

// This is the list of chunks to be used for computing dynamic shadows.
// A set is used, as there shall not be duplicate entries.
static std::set<ChunkCoord> sShadowChunks;

// Add the specified chunk, as well as all other chunks that are allowed to make
// shadows. The algorithm knows the direction of the sun, and finds all chunks in
// that direction. But only to a limited height.
static void AddChunkToShadowList(Chunk *cp) {
	ChunkCoord cc = cp->cc;
	// For every level, 4 chunks have to be considered.
	const ChunkCoord delta[] = {
		{ 0, 0, 0},
		{ -1, 0, 0 },
		{ -1, -1, 0},
		{ 0, -1, 0},
	};
	for (int h=0; h < 3; h++) {
		for (size_t i=0; i<NELEM(delta); i++) {
			ChunkCoord cc2 = cc;
			// FOr every level, move one step south and west (x and y).
			cc2.x += delta[i].x * h;
			cc2.y += delta[i].y * h;
			cc2.z += h;
			sShadowChunks.insert(cc2);
		}
	}
}

int gDrawnQuads;
int gNumDraw;
float _angleHor = 0;            //The rotation of the player
float _angleVert = 0.0;

float maxRenderDistance = 50.0; // Number of blocks visible distance.

struct ChunkDist {
	char dx, dy, dz;
	float distance;
};

// Used by the qsort function.
int compare(const void * a, const void * b) {
	return int(( ((ChunkDist*)a)->distance - ((ChunkDist*)b)->distance )*100); // Two digits precision
}

#define NUMQUERIES 20 // Number of queries executed at the same time.

// All chunks need to be sorted, on distance to player, before they are drawn. That will increase the
// chance that near chunks occlude more distant chunks, leading to fewer updates of the fragment shader.
// Make an array of relative chunks that could be seen by the player and sort it offline, before the game starts.
static std::vector<ChunkDist> sChunkDistances;

// For each item in sChunkDistances, this will point to the actual chunk.
static std::vector<Chunk*> sListOfNearChunks;

// OpenGL queries are used to find if chunks can be seen, and thus need to bedrawn. This is done in groups of
// NUMQUERIES chunks. To minimize the risk that reading the result of a query will delay the graphics pipeline,
// The next group of NUMQUERIES queries are started before the first group is tested. That means that there will
// be up to NUMQUERIES*2 queries active at the same time.
static GLuint sQueryId[NUMQUERIES*2];

// This is the number of chunks that can be seen by the player at max viewing distance. It is usually 895 for 80m
// distance, but a more safe way is to actually test and see. It should probably be calculated theoretically.
static int sMaxVolume = 0;

// Create a sorted list of chunks. This is only done once.
void ComputeRelativeChunksSortedDistances() {
	int chunkHor = (int)(MAXRENDERDISTANCE / 32 + 1); // Maximum number of chunks radius
	// Lazy way to compute the possible volume
	sMaxVolume = 0;
	for (int dz=chunkHor-1; dz>-chunkHor; dz--) for (int dx= -chunkHor+1; dx<chunkHor; dx++) for (int dy= -chunkHor+1; dy<chunkHor; dy++) {
				sMaxVolume++;
			}
	sChunkDistances.resize(sMaxVolume);
	int i = 0;
	// First initialize the vector with an unsorted list of chunks
	for (char dz=chunkHor-1; dz>-chunkHor; dz--) for (char dx= -chunkHor+1; dx<chunkHor; dx++) for (char dy= -chunkHor+1; dy<chunkHor; dy++) {
				if (i >= sMaxVolume)
					abort();
				sChunkDistances[i].dx = dx;
				sChunkDistances[i].dy = dy;
				sChunkDistances[i].dz = dz;
				sChunkDistances[i].distance = sqrt((float)(dx*dx+dy*dy+dz*dz));
				i++;
			}
	if (i != sMaxVolume)
		abort();
	qsort(&sChunkDistances[0], sMaxVolume, sizeof(ChunkDist), compare); // This will sort on distance
	glGenQueries(NUMQUERIES*2, sQueryId);
	checkError("ComputeRelativeChunksSortedDistances");
	sListOfNearChunks.resize(sMaxVolume);
}

static void FindAllNearChunks(const std::vector<ChunkDist> &chunkDistances) {
	ChunkCoord player_cc;
	if (!Model::gPlayer.KnownPosition())
		return;
	Model::gPlayer.GetChunkCoord(&player_cc);
	for (int i=0; i<sMaxVolume; i++) {
		if (chunkDistances[i].distance > (int)(maxRenderDistance / 32 + 1)) {
			sListOfNearChunks[i] = 0; // Just a safety marker
			break;
		}
		int dx = chunkDistances[i].dx;
		int dy = chunkDistances[i].dy;
		int dz = chunkDistances[i].dz;
		ChunkCoord cc;
		cc.x = player_cc.x + dx;
		cc.y = player_cc.y + dy;
		cc.z = player_cc.z + dz;
		// Don't force loading yet if it doesn't exist. That can wait until we know better what chunks are visible.
		sListOfNearChunks[i] = ChunkFind(&cc, false);
	}
}

// Check if a list of vertices are all outside the frustum at the same side of a plane. This will find
// most of the chunks not inside the frustum.
bool AllVerticesOutsideFrustum(const glm::vec3 *v, int numVertices, const glm::mat4 &modelMatrix, const glm::vec4 &viewport) {
	int outside[6]; // Number of vertices outside each of 6 planes
	for (int i=0; i<6; i++)
		outside[i] = numVertices; // Assume all outside
	for (int i=0; i<numVertices; i++) {
		glm::vec3 screen = glm::project(v[i], gViewMatrix * modelMatrix, gProjectionMatrix, viewport);
		if (screen.x < 0.0f)
			outside[0]--;
		if (screen.x > viewport[2])
			outside[1]--;
		if (screen.y < 0.0f)
			outside[2]--;
		if (screen.y > viewport[3])
			outside[3]--;
		if (screen.z < 0.0f)
			outside[4]--;
		if (screen.z > 1.0f)
			outside[5]--;
	}
	for (int i=0; i<6; i++)
		if (outside[i] == 0)
			return true;
	return false;
}

// Can the user see this chunk? Test if any of the 8 corners are inside the display (view frustum).
// TODO: If the chunk is completely empty (only air), the following test could be skipped.
static bool Outside(int dx, int dy, int dz, const glm::mat4 &modelMatrix) {
	static const glm::vec3 v[] = {
		// All 8 corners
		glm::vec3( 0,			0,			0),
		glm::vec3( CHUNK_SIZE,	0,			0),
		glm::vec3( 0, 			CHUNK_SIZE, 0),
		glm::vec3( CHUNK_SIZE,	CHUNK_SIZE, 0),
		glm::vec3( 0,			0,			-CHUNK_SIZE),
		glm::vec3( CHUNK_SIZE,	0,			-CHUNK_SIZE),
		glm::vec3( 0,			CHUNK_SIZE, -CHUNK_SIZE),
		glm::vec3( CHUNK_SIZE,	CHUNK_SIZE, -CHUNK_SIZE),
	};
	bool outside = true; // Start with the supposition that all corners are outside of the screen.
	if (dx != 0 || dy != 0 || dz != 0) {
		outside = AllVerticesOutsideFrustum(v, sizeof v / sizeof v[0], modelMatrix, gViewport);
	} else {
		// Always assume that the chunk we are inside shall be drawn.
		outside = false;
	}
	return outside;
}

// Investigate chunks in the specified interval, and test if they are visible.
// This is done in a pipeline, which means enough cubes should be tested every time
// to make sure the wait for the pipe line isn't too long.
// Instead of drawing the complete chunk, a bounding box in the form of a cube is drawn.
// This may lead to false positives, but probably not that many.
static void QuerySetup(StageOneShader *shader, int from, int to, int *listOfVisibleChunks) {
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glBindTexture(GL_TEXTURE_2D, GameTexture::BlueChunkBorder); // Any texture will do
	for (int i=from; i<to; i++) {
		int ind = listOfVisibleChunks[i];
		int dx = sChunkDistances[ind].dx;
		int dy = sChunkDistances[ind].dy;
		int dz = sChunkDistances[ind].dz;

		Chunk *cp = sListOfNearChunks[ind]; // Can be 0!
		glBeginQuery(GL_SAMPLES_PASSED, sQueryId[i%(NUMQUERIES*2)]);
		cp->DrawBoundingBox(shader, dx, dy, dz); // Works also for null pointer
		glEndQuery(GL_SAMPLES_PASSED);
	}
	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

// In construction mode, the borders of the near chunks are drawn to make it easier to see
// where the chunk ends.
static void DrawChunkBorders(StageOneShader *shader) {
	ChunkCoord player_cc;
	Model::gPlayer.GetChunkCoord(&player_cc);
	for (int dz=-1; dz<2; dz++) for (int dx=-1; dx<2; dx++) for (int dy=-1; dy<2; dy++) {
				if (dx == 0 && dy == 0 && dz == 0)
					continue; // Can't see the boundary to the current chunk
				if ((dy != 0 && dx != 0) || (dy != 0 && dz != 0) || (dx != 0 && dz != 0))
					continue; // skip the chunks diagonally away from the current one.
				ChunkCoord cc;
				cc.x = player_cc.x + dx;
				cc.y = player_cc.y + dy;
				cc.z = player_cc.z + dz;
				Chunk *cp = ChunkFind(&cc, true);
				unsigned long owner = -1;
				owner = cp->fChunkBlocks->fOwner;
				GLuint text = GameTexture::BlueChunkBorder; // Blue means not allocated
				if (owner == Model::gPlayer.GetId())
					text = GameTexture::GreenChunkBorder; // Allocated by the current player
				else if (owner < 0x8FFFFFFF && owner > 0)
					text = GameTexture::RedChunkBorder; // Allocated by someone else
				glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(dx*CHUNK_SIZE+16.0f, dz*CHUNK_SIZE, -dy*CHUNK_SIZE-16.0f));
				modelMatrix = glm::rotate(modelMatrix, 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
				float scale = 32.0f * 1.414f; // Side was originally 1/sqrt(2) blocks, but height was 1.
				modelMatrix = glm::scale(modelMatrix, glm::vec3(scale, 32.0f, scale));
				glBindTexture(GL_TEXTURE_2D, text); // For now, also used on top and bottom.
				shader->Model(modelMatrix);
				gLantern.Draw();
			}
}

// TODO: This function should be split into a separate function for picking mode.
void DrawLandscape(StageOneShader *shader, DL_Type dlType) {
	if (!Model::gPlayer.KnownPosition())
		return;
	if (dlType == DL_NoTransparent) {
		// The lits of chunks used for shadowing is computed below, when in non-transparent mode.
		// Start the list empty.
		sShadowChunks.clear();
	}

	FindAllNearChunks(sChunkDistances);
	ChunkCoord player_cc;
	Model::gPlayer.GetChunkCoord(&player_cc);

	// Draw all visible chunks. Chunks that are not loaded will trigger a reload from the server. The top chunks
	// should be loaded first, as they affect the lighting on the chunks below.
	int chunkHor = (int)(maxRenderDistance / CHUNK_SIZE + 1);

	// At 80m viewing distance, there are 895 chunks. Using various filters, the actual chunks that can be seen are
	// much fewer. Save the filtered list, to speed up the drawing.
	int listOfVisibleChunks[sMaxVolume];
	int src, dst;
	// Create list of visible chunks. This check is faster than using queries, so it is a good filter to
	// start with.
	for (src=0, dst=0; sChunkDistances[src].distance < chunkHor; src++) {
		if (src > sMaxVolume)
			break; // Just a safety precaution, should never happen
		Chunk *cp = sListOfNearChunks[src];
		if (cp) {
			if (cp->fScheduledForLoading)
				continue; // Chunk doesn't exist yet
			// Do not ignore the chunk just because it doesn't have any data yet, as the data update is triggered
			// by the drawing function.
			if (!cp->IsDirty() && cp->fChunkObject && cp->fChunkObject->Empty()) {
				// This chunk exists, is updated, but contains nothing.
				continue;
			}
		}
		int dx = sChunkDistances[src].dx;
		int dy = sChunkDistances[src].dy;
		int dz = sChunkDistances[src].dz;
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(dx*CHUNK_SIZE, dz*CHUNK_SIZE, -dy*CHUNK_SIZE));
		if (Outside(dx, dy , dz, modelMatrix)) {
			continue; // Go to next chunk
		}
		listOfVisibleChunks[dst++] = src;
	}
	int visibleChunklistLength = dst;
	// printf("DrawLandscape: listOfVisibleChunks %d\n", visibleChunklistLength);
	bool insideAnyTeleport = false;

	// We need to know which TP is the nearest.
	float distanceToNearTP2 = 1000.0f*1000.0f; // Distance^2 to the nearest TP, initialized to something big.
	glm::vec3 TPPosition;
	for (int i=0; i<visibleChunklistLength; i++) {
		if (i%NUMQUERIES == 0 && dlType == DL_NoTransparent) {
			// Request NUMQUERIES queries, every NUMQUERIES chunk.
			// The chunks that are tested is not the same ones that will be drawn,
			// to allow the reading of the query result to be with a delay.
			// The first NUMQUERIES chunks are not tested. The occlusion test using queries is most effective
			// for long viewing distances, where there is a high chance of chunks not being visible.
			// TODO: The test could also be used for transparent objects, if it wasn't that QuerySetup() enables depth update again.
			int from = i+NUMQUERIES;
			int to = i+NUMQUERIES*2;
			if (to > visibleChunklistLength)
				to = visibleChunklistLength;
			if (to > from)
				QuerySetup(shader, from, to, listOfVisibleChunks); // Initiate the query for a group of chunks
		}
		int ind = listOfVisibleChunks[i];
		if (dlType == DL_OnlyTransparent)
			ind = listOfVisibleChunks[visibleChunklistLength-i-1]; // Read chunks in reverse order for transparent objects
		if (i >= NUMQUERIES && dlType == DL_NoTransparent) { // There is no query initiated for the first NUMQUERIES chunks.
			GLuint numSamples = 1;
			glGetQueryObjectuiv(sQueryId[i%(NUMQUERIES*2)], GL_QUERY_RESULT, &numSamples);
			if (numSamples == 0)
				continue; // No pixels changed by this chunk, skip it!
		}
		int dx = sChunkDistances[ind].dx;
		int dy = sChunkDistances[ind].dy;
		int dz = sChunkDistances[ind].dz;
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(dx*CHUNK_SIZE, dz*CHUNK_SIZE, -dy*CHUNK_SIZE));

		// Don't allow picking outside of near chunks.
		if (dlType == DL_Picking && (dx < -1 || dx > 1 || dy < -1 || dy > 1 || dz < -1 || dz > 1))
			break;

		ChunkCoord cc;
		cc.x = player_cc.x + dx;
		cc.y = player_cc.y + dy;
		cc.z = player_cc.z + dz;
		// If we have come this far, a null pointer is no longer accepted. The chunk is needed. Either we get
		// the real chunk, or an empty one.
		Chunk *cp = ChunkFind(&cc, true);

		if (dlType == DL_Picking) {
			// Picking mode. Throw away the previous triangles, and replace it with special triangles used
			// for the picking mode. These contain colour information to allow identification of cubes.
			cp->fChunkBlocks->TestJellyBlockTimeout(true);
			cp->PushTriangles(ChunkObject::Make(cp, true, dx, dy, dz)); // Stash away the old triangles for quick restore
			gChunkShaderPicking.Model(modelMatrix);
		} else {
			cp->fChunkBlocks->TestJellyBlockTimeout(false);
			shader->Model(modelMatrix);
		}

		if (dlType == DL_NoTransparent)
			AddChunkToShadowList(cp);

		// Draw the chunk
		cp->Draw(shader, &gChunkShaderPicking, dlType);

		if (dlType == DL_Picking) {
			// Picking mode. Restore the normal triangles again.
			cp->PopTriangles();
		}

		if (dlType == DL_NoTransparent) {
			cp->DrawObjects(shader, dx, dy, dz, false);
		}

		if (dlType == DL_OnlyTransparent) {
			unsigned char tx, ty, tz;
			if (gMode.CommunicationAllowed() && Model::gSuperChunkManager.GetTeleport(&tx, &ty, &tz, &cp->cc)) {
				double diffX = double(Model::gPlayer.x)/BLOCK_COORD_RES - cc.x*CHUNK_SIZE - tx - 0.5;
				double diffY = double(Model::gPlayer.y)/BLOCK_COORD_RES - cc.y*CHUNK_SIZE - ty - 0.5;
				double diffZ = double(Model::gPlayer.z)/BLOCK_COORD_RES - cc.z*CHUNK_SIZE - tz - PLAYER_HEIGHT*2;
				double d2 = diffX*diffX + diffY*diffY + diffZ*diffZ;
				if (diffX < 1.5 && diffX > -1.5 && diffY < 1.5 && diffY > -1.5 && diffZ < 3.0 && diffZ > 0.0)
					insideAnyTeleport = true;
				// printf("%.2f, %.2f, %.2f\n", diffX, diffY, diffZ);
				if (gMode.Get() == GameMode::GAME) {
					// When in teleport mode, the teleports are drawn elsewhere.
					float x = (float)tx + dx*CHUNK_SIZE + 0.5f;
					float y = (float)tz + dz*CHUNK_SIZE + 0.5f;
					float z = -(float)ty - dy*CHUNK_SIZE - 0.5f;
					gDrawObjectList.emplace_back(glm::vec3(x, y+2, z), BT_Teleport);
					// gMsgWindow.Add("chunk::DrawObjects Teleport at %f,%f,%f\n", x, y, z);
					glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
					glBindTexture(GL_TEXTURE_2D, GameTexture::Teleport);
					modelMatrix = glm::scale(modelMatrix, glm::vec3(4.0f, 4.0f, 4.0f));
					modelMatrix = glm::rotate(modelMatrix, 45.0f, glm::vec3(0,1,0));
					shader->Model(modelMatrix);
					gLantern.Draw();
				}
				if (d2 < distanceToNearTP2) {
					TPPosition.x = diffX;
					TPPosition.y = diffY;
					TPPosition.z = diffZ;
					distanceToNearTP2 = d2;
				}
			}
		}
	}

	if (dlType == DL_OnlyTransparent) {
		static bool wasNearTP = false;
		if (distanceToNearTP2 < 100) {
			// Update sound effect only if inside a limited distance.
			gSoundControl.SetEnvironmentSound(SoundControl::STeleport, 0, -800*TPPosition.x, -800*TPPosition.y, -800*TPPosition.z);
			wasNearTP = true;
		} else if (wasNearTP) {
			wasNearTP = false;
			gSoundControl.RemoveEnvironmentSound(SoundControl::STeleport, 0);
		}

		// Test for being inside teleports is only done in transparent mode
		switch(gMode.Get()) {
		case GameMode::GAME:
			if (insideAnyTeleport) {
				gMode.Set(GameMode::TELEPORT);
			}
			break;
		case GameMode::TELEPORT:
			if (!insideAnyTeleport && !gAdminTP) {
				gMode.Set(GameMode::GAME);
			}
			break;
		case GameMode::CONSTRUCT:
			// Draw the semi transparent boundaries between chunks.
			DrawChunkBorders(shader);
			break;
		case GameMode::INIT: case GameMode::ESC: case GameMode::EXIT: case GameMode::LOGIN: case GameMode::LOGIN_FAILED:
		case GameMode::PASSWORD: case GameMode::REQ_PASSWD: case GameMode::WAIT_ACK:
			break; // Ignore
		}
	}
}

void DrawLandscapeTopDown(StageOneShader *shader, int width, int height, bool forceload, DL_Type dlType) {
	if (!Model::gPlayer.KnownPosition())
		return;
	ChunkCoord player_cc;
	Model::gPlayer.GetChunkCoord(&player_cc);

	int verticallimit = (height+CHUNK_SIZE-1)/CHUNK_SIZE/2;
	int horisontallimit = (width+CHUNK_SIZE-1)/CHUNK_SIZE/2;

	// Draw all visible chunks. Chunks that are not loaded will trigger a reload from the server. The top chunks
	// should be loaded first, as they affect the lighting on the chunks below.
	for (int dz = verticallimit; dz >= -verticallimit; dz--) for (int dx = -horisontallimit; dx <= horisontallimit; dx++) for (int dy = -horisontallimit; dy <= horisontallimit; dy++) {
				glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(dx*CHUNK_SIZE, dz*CHUNK_SIZE, -dy*CHUNK_SIZE));

				ChunkCoord cc;
				cc.x = player_cc.x + dx;
				cc.y = player_cc.y + dy;
				cc.z = player_cc.z + dz;
				Chunk *cp = ChunkFind(&cc, forceload);
				if (cp == 0)
					continue;

				shader->Model(modelMatrix);

				// Draw the chunk
				if (!cp->IsDirty() && cp->fChunkObject && cp->fChunkObject->Empty()) {
					// This chunk exists, is updated, but contains nothing.
					continue;
				}
				cp->Draw(shader, 0, dlType);
				cp->DrawObjects(shader, dx, dy, dz, true);
			}
}

void DrawLandscapeForShadows(StageOneShader *shader) {
	if (!Model::gPlayer.KnownPosition())
		return;
	ChunkCoord player_cc;
	Model::gPlayer.GetChunkCoord(&player_cc);

	// Draw all visible chunks. Chunks that are not loaded will trigger a reload from the server. The top chunks
	// should be loaded first, as they affect the lighting on the chunks below.
	for (auto it=sShadowChunks.begin() ; it != sShadowChunks.end(); it++ ) {
		Chunk *cp = ChunkFind(&(*it), true);

		if (!cp->IsDirty() && cp->fChunkObject && cp->fChunkObject->Empty()) {
			// This chunk exists, is updated, but contains nothing.
			continue;
		}

		int dx = cp->cc.x - player_cc.x;
		int dy = cp->cc.y - player_cc.y;
		int dz = cp->cc.z - player_cc.z;
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(dx*CHUNK_SIZE, dz*CHUNK_SIZE, -dy*CHUNK_SIZE));

		shader->Model(modelMatrix);
		cp->Draw(shader, 0, DL_NoTransparent);
		cp->DrawObjects(shader, dx, dy, dz, true);
	}
}

LightSources gLightSources;
void LightSources::AddRed(float x, float y, float z, float radius) {
	fItems.push_back(glm::vec4(x, y, z, radius+RedOffset));
}

void LightSources::AddGreen(float x, float y, float z, float radius) {
	fItems.push_back(glm::vec4(x, y, z, radius+GreenOffset));
}

void LightSources::AddBlue(float x, float y, float z, float radius) {
	fItems.push_back(glm::vec4(x, y, z, radius+BlueOffset));
}

Shadows gShadows;

void Shadows::Add(float x, float y, float z, float radius, float limit) {
	if (fItems.size() < gOptions.fMaxShadows*limit) {
		fItems.push_back(glm::vec4(x, y, z, radius));
	}
}

Fogs gFogs;

void Fogs::Add(float x, float y, float z, int radius, float ambient) {
	if (fItems.size() < MAX_RENDER_FOGS) {
		// Add ambient fraction as decimals to the radius.
		if (ambient > 0.99f) ambient = 0.99f;
		fItems.push_back(glm::vec4(x, y, z, radius + ambient));
	}
}
std::vector<DrawObjectList> gDrawObjectList;
