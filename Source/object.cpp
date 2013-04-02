// Copyright 2013 The Ephenation Authors
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

#include "object.h"
#include "chunk.h"
#include "player.h"

using namespace Model;

glm::vec3 Model::GetRelativePosition(entityx::Entity entity) {
	auto posCmp = entity.component<PositionCmp>();
	assert(posCmp != nullptr);
	ChunkCoord cc;
	gPlayer.GetChunkCoord(&cc);
	float dx = (posCmp->x - (signed long long)cc.x*BLOCK_COORD_RES * CHUNK_SIZE)/float(BLOCK_COORD_RES);
	float dy = (posCmp->y - (signed long long)cc.y*BLOCK_COORD_RES * CHUNK_SIZE)/float(BLOCK_COORD_RES);
	float dz = (posCmp->z - (signed long long)cc.z*BLOCK_COORD_RES * CHUNK_SIZE)/float(BLOCK_COORD_RES);
	return glm::vec3(dx, dz, -dy);
}
