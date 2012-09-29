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
#include <stdio.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Teleport.h"
#include "DrawTexture.h"
#include "chunk.h"
#include "player.h"
#include "textures.h"
#include "primitives.h"
#include "SuperChunkManager.h"
#include "HealthBar.h"

#define MAXTELEPORT 15 // Numer of chunks possible to TP

void DrawTeleports(DrawTexture *dt, float angle, float renderViewAngle) {
	ChunkCoord player_cc;
	gPlayer.GetChunkCoord(&player_cc);
	glBindTexture(GL_TEXTURE_2D, GameTexture::RedColor);
	glm::mat4 proj = glm::perspective(renderViewAngle, gViewport[2]/gViewport[3], 1.0f, float(MAXTELEPORT)*CHUNK_SIZE);
	for (int dx = -MAXTELEPORT+1; dx < MAXTELEPORT; dx++) for (int dy = -MAXTELEPORT+1; dy < MAXTELEPORT; dy++) for (int dz = -MAXTELEPORT+1; dz < MAXTELEPORT; dz++) {
				unsigned char tx, ty, tz;
				ChunkCoord cc;
				cc.x = player_cc.x + dx;
				cc.y = player_cc.y + dy;
				cc.z = player_cc.z + dz;
				if (gSuperChunkManager.GetTeleport(&tx, &ty, &tz, &cc)) {
					float x = (float)tx + dx*CHUNK_SIZE + 0.5f;
					float y = (float)tz + dz*CHUNK_SIZE + 0.5f;
					float z = -(float)ty - dy*CHUNK_SIZE - 0.5f;
					glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
					modelMatrix = glm::rotate(modelMatrix, -angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Need to counter the rotation from the view matrix
					modelMatrix = glm::scale(modelMatrix, glm::vec3(4.0f, 4.0f, 4.0f));
					dt->Draw(proj, gViewMatrix * modelMatrix);
				}
			}
}


const ChunkCoord *TeleportClick(HealthBar *hb, float angle, float renderViewAngle, int x, int y, bool picking) {
	if (picking) {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Use black sky for picking
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	ChunkCoord player_cc;
	gPlayer.GetChunkCoord(&player_cc);
	glm::mat4 proj = glm::perspective(renderViewAngle, gViewport[2]/gViewport[3], 1.0f, float(MAXTELEPORT)*CHUNK_SIZE);
	glm::mat4 projview = proj * gViewMatrix;
	for (int dx = -MAXTELEPORT+1; dx < MAXTELEPORT; dx++) for (int dy = -MAXTELEPORT+1; dy < MAXTELEPORT; dy++) for (int dz = -MAXTELEPORT+1; dz < MAXTELEPORT; dz++) {
				if (dx == 0 && dy == 0 && dz == 0)
					continue; // Ignore TP at player chunk
				unsigned char tx, ty, tz;
				ChunkCoord cc;
				cc.x = player_cc.x + dx;
				cc.y = player_cc.y + dy;
				cc.z = player_cc.z + dz;
				if (gSuperChunkManager.GetTeleport(&tx, &ty, &tz, &cc)) {
					float x = (float)tx + dx*CHUNK_SIZE + 0.5f;
					float y = (float)tz + dz*CHUNK_SIZE + 0.5f;
					float z = -(float)ty - dy*CHUNK_SIZE - 0.5f;
					glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
					modelMatrix = glm::rotate(modelMatrix, -angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Need to counter the rotation from the view matrix
					modelMatrix = glm::scale(modelMatrix, glm::vec3(7.0f, 7.0f, 7.0f));
					float red = (dx+MAXTELEPORT)/256.0f;
					float green = (dy+MAXTELEPORT)/256.0f;
					float blue = (dz+MAXTELEPORT)/256.0f;
					float alpha = 1.0f;
					if (!picking) {
						red = 1.0f; blue = 1.0f; green = 0.0f; alpha = 0.4f;
					}
					hb->DrawSquare(projview * modelMatrix, red, green, blue, alpha);
				}
			}
	unsigned char pixel[3];
	glReadPixels(x, gViewport[3] - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
	static ChunkCoord ret;
	ret.x = pixel[0]-MAXTELEPORT+player_cc.x;
	ret.y = pixel[1]-MAXTELEPORT+player_cc.y;
	ret.z = pixel[2]-MAXTELEPORT+player_cc.z;
	// printf("%d,%d,%d\n", pixel[0]-MAXTELEPORT+player_cc.x, pixel[1]-MAXTELEPORT+player_cc.y, pixel[2]-MAXTELEPORT+player_cc.z);
	if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0)
		return 0;
	return &ret;
}
