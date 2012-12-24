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

#include <GL/glew.h>
#include <GL/glfw.h>
#include <string.h>
#include <string>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "object.h"
#include "client_prot.h"
#include "otherplayers.h"
#include "render.h"
#include "chunk.h"
#include "player.h"
#include "primitives.h"
#include "textures.h"
#include "SoundControl.h"
#include "HealthBar.h"
#include "parse.h"
#include "connection.h"
#include "DrawText.h"
#include "Options.h"
#include "shaders/AnimationShader.h"
#include "BlenderModel.h"

using std::string;
using std::stringstream;
using std::endl;

OtherPlayers gOtherPlayers;

OtherPlayers::OtherPlayers() : fMaxIndex(0) {
	for (int i=0; i<MAXPLAYERS; i++) {
		fPlayers[i].slotFree = true;
	}
}

glm::vec3 OtherPlayers::OneOtherPlayer::GetSelectionColor() const {
	return glm::vec3(-0.2f, 0.2f, -0.2f);
}

glm::vec3 OtherPlayers::OneOtherPlayer::GetPosition() const {
	ChunkCoord cc;
	gPlayer.GetChunkCoord(&cc);
	float dx = (this->x - (signed long long)cc.x*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dy = (this->y - (signed long long)cc.y*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dz = (this->z - (signed long long)cc.z*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	// Add some corrections to get the coordinate of the feet.
	return glm::vec3(dx, dz-4.0f, -dy);
}

OtherPlayers::OneOtherPlayer *OtherPlayers::GetSelection(unsigned char R, unsigned char G, unsigned char B) {
	return &fPlayers[G*256+B];
}

int OtherPlayers::FindPlayer(unsigned long uid) const {
	for (int i=0; i < fMaxIndex; i++) {
		if (!fPlayers[i].slotFree && fPlayers[i].id == uid) {
			return i;
		}
	}
	return -1;
}

void OtherPlayers::SetPlayer(unsigned long id, unsigned char hp, unsigned int level, signed long long x, signed long long y, signed long long z, float dir) {
	// Find the player, or an empty slot
	int i;
	int firstFree = -1;
	int max = fMaxIndex+1;
	if (max > MAXPLAYERS)
		max = MAXPLAYERS;
	for (i=0; i < max; i++) {
		if (fPlayers[i].slotFree) {
			if (firstFree == -1)
				firstFree = i;
		} else if (fPlayers[i].id == id) {
			break;
		}
	}

	// Either use the slot found, or the first free slot

	if (i == max) { // Id was not already stored
		i = firstFree;
		fPlayers[i].slotFree = true;
	}

	if (fPlayers[i].slotFree) {
		unsigned char b[7];
		b[0] = sizeof b;
		b[1] = 0;
		b[2] = CMD_REQ_PLAYER_INFO;
		EncodeUint32(b+3, id);
		SendMsg(b, sizeof b);
		fPlayers[i].playerName = 0;
		fPlayers[i].id = id;
		fPlayers[i].slotFree = false;
		fPlayers[i].ingame = true;
	}

	if (i != MAXPLAYERS) {
		// printf("Store info for player %d in slot %d (max: %d, fMaxIndex %d)\n", id, i, max, fMaxIndex);
		fPlayers[i].x = x;
		fPlayers[i].y = y;
		fPlayers[i].z = z;
		fPlayers[i].hp = hp;
		fPlayers[i].level = level;
		fPlayers[i].fDir = dir;
		fPlayers[i].fUpdateTime = gCurrentFrameTime;

		if (i == fMaxIndex)
			fMaxIndex = i+1;
	}
}

void OtherPlayers::SetPlayerName(unsigned long uid, const char *name, int n, int adminLevel) {
	int ind = this->FindPlayer(uid);
	if (ind == -1)
		return; // Give it up. Should not happen
	fPlayers[ind].playerName = new char[n+1];
	strncpy(fPlayers[ind].playerName, name, n);
	fPlayers[ind].playerName[n] = 0; // null-terminate string
	// printf("Player %d got name %s and admin level %d\n", uid, fPlayers[ind].playerName, adminLevel);
}

void OtherPlayers::RenderPlayers(AnimationShader *animShader, bool selectionMode) const {
	ChunkCoord cc;
	gPlayer.GetChunkCoord(&cc);
	float sun = 1.0f;
	if (gPlayer.BelowGround()) {
		// A gross simplification. If underground, disable all sun.
		sun = 0.0f;
	}
	glBindTexture(GL_TEXTURE_2D, GameTexture::RedColor);
	animShader->EnableProgram();
	for (int i=0; i<fMaxIndex; i++) {
		if (fPlayers[i].ingame) {
			float dx = (fPlayers[i].x - (signed long long)cc.x*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
			float dy = (fPlayers[i].y - (signed long long)cc.y*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
			float dz = (fPlayers[i].z - (signed long long)cc.z*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;

			glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(dx, dz-PLAYER_HEIGHT*2.0f, -dy));
			model = glm::rotate(model, -fPlayers[i].fDir, glm::vec3(0.0f, 1.0f, 0.0f));

			double tm = gCurrentFrameTime-0.22; // Offset in time where model is not in a stride.
#if 0
			if (fMoving)
				tm = 0.0;
#endif
			gFrog.DrawAnimation(animShader, model, tm, false, 0);
			if (!gOptions.fDynamicShadows || sun == 0)
				gShadows.Add(dx, dz-PLAYER_HEIGHT*2.0f, -dy, 1.5f);
		}
	}
	animShader->DisableProgram();
}

void OtherPlayers::RenderPlayerStats(HealthBar *hb, float angle) const {
	for (int i=0; i<fMaxIndex; i++) {
		if (fPlayers[i].ingame) {
			fPlayers[i].RenderHealthBar(hb, angle);
		}
	}
}

void OtherPlayers::RenderMinimap(const glm::mat4 &miniMap, HealthBar *hb) const {
	for (int i=0; i<fMaxIndex; i++) {
		if (fPlayers[i].ingame && !fPlayers[i].IsDead()) {
			float dx = float(gPlayer.x - fPlayers[i].x)/CHUNK_SIZE/BLOCK_COORD_RES/2;
			float dy = float(gPlayer.y - fPlayers[i].y)/CHUNK_SIZE/BLOCK_COORD_RES/2;
			float dz = float(gPlayer.z - fPlayers[i].z)/CHUNK_SIZE/BLOCK_COORD_RES/2;
			if (dx > 1.0f || dx < -1.0f || dy > 1.0f || dy < -1.0f || dz > 1.0f || dz < -1.0f)
				continue; // To far away on the radar
			// printf("OtherPlayers::RenderMinimap (%f, %f, %f)\n", dx, dy, dz);
			glm::mat4 model = glm::translate(miniMap, glm::vec3(0.5f-dx, 0.5f-dy, 0.0f));
			model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
			// model = glm::rotate(model, 180.0f-fMonsters[i].fDir, glm::vec3(0.0f, 1.0f, 0.0f));
			hb->DrawSquare(model, 0.0f, 1.0f, 1.0f, 1.0f);
		}
	}
}

void OtherPlayers::OneOtherPlayer::RenderHealthBar(HealthBar *hb, float angle) const {
	ChunkCoord cc;
	gPlayer.GetChunkCoord(&cc);
	float dx = (this->x - (signed long long)cc.x*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dy = (this->y - (signed long long)cc.y*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dz = (this->z - (signed long long)cc.z*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(dx, dz+1.0f, -dy));
	model = glm::rotate(model, -angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Need to counter the rotation from the view matrix
	model = glm::scale(model, glm::vec3(2.0f, 0.2f, 1.0f));
	hb->DrawHealth(gProjectionMatrix, gViewMatrix * model, this->hp/255.0f, 0.0f, true);

	glm::vec4 v = glm::vec4(dx, dz+2.0f, -dy, 1.0f);
	glm::vec4 screen = gProjectionMatrix * gViewMatrix * v;
	screen = screen / screen.w;
	// printf("Selected: (%f,%f,%f,%f)\n", screen.x, screen.y, screen.z, screen.w);
	if (screen.z > 1)
		return; // Ignore display of Level information for monsters behind the camera
	gDrawFont.Enable();
	gDrawFont.UpdateProjection();
	gDrawFont.SetOffset(gViewport[2] * (screen.x+1)/2, gViewport[3] * (1-screen.y)/2);
	stringstream ss;
	if (this->playerName)
		ss << this->playerName << " ";
	ss << "level " << this->level << endl;
	glm::vec3 offs(0.0f, -0.8f, -0.8f);
	if (this->level+3 < gPlayer.fLevel)
		offs = glm::vec3(-0.2f, -0.2f, -0.2f); // Gray
	else if (this->level < gPlayer.fLevel+3)
		offs = glm::vec3(-0.8f, -0.0f, -0.8f); // Green
	gDrawFont.vsfl.renderAndDiscard(ss.str(), offs);
}

// The server doesn't explicitly tell the client when another player shall be removed. The reason for this is
// that the client only gets information about changed players that are near enough. The way to go is to
// measure the time that the player was last updated, and discard the player after 5s. The server will normally
// only update player information every time there is a change, but there will also be a complete update at
// least every 4s. So players that have not had any update in 5s are stale, and should be removed.
void OtherPlayers::Cleanup(void) {
	double now = gCurrentFrameTime;
	for (int i=0; i<fMaxIndex; i++) {
		if (!fPlayers[i].ingame)
			continue;
		if (now - fPlayers[i].fUpdateTime < 5.0)
			continue;
		// printf("Found stale player: index %d, id %ld\n", i, fPlayers[i].id);
		fPlayers[i].ingame = false;
		fPlayers[i].slotFree = true;
		delete []fPlayers[i].playerName;
		fPlayers[i].playerName = 0;

		// Remove this players as a creature in SoundControl
		gSoundControl.RemoveCreatureSound(SoundControl::SOtherPlayer,fPlayers[i].id);
	}
}
