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

#include "client_prot.h"
#include <glm/glm.hpp>
#include "object.h"
;
struct ChunkCoord;
class StageOneShader;
class AnimationShader;
class AnimationModels;

class player : public Object {
public:
	// TODO: These should be private. It is the position of the head of the player.
	// Coordinate in the server system ('z' is height).
	signed long long x;
	signed long long y;
	signed long long z;

	float fAngleHor;
	float fAngleVert;
	bool loginOk;
	bool fStatsAvailable; // True when player stats are available.
	float fHp; // Hit points, going from 0-1
	float fPreviousHp; // The Previous HP, used to display delta.
	float fExp; // Experience points, going from 0-1
	float fMana;
	unsigned long fLevel;
	unsigned char fAdmin; // 0 to 10, where 0 means no admin rights.
	unsigned long fUid; // The unique server ID of the player.
	unsigned long fFlags; // From the server, see UserFlag* defined in client_prot.h
	unsigned char fWeaponType;
	unsigned long fWeaponLevel;
	unsigned char fArmorType;
	unsigned long fArmorLevel;
	unsigned char fHelmetType;
	unsigned long fHelmetLevel;

	void GetChunkCoord(ChunkCoord*) const;
	glm::vec3 GetOffsetToChunk(void) const; // This gives the head of the player
	bool InFight(void) const;
	void Draw(AnimationShader *animShader, StageOneShader *staticShader, bool torch, const AnimationModels *animationModels); // Draw self

	player() {
		fHp = 1.0f; fPreviousHp = 1.0f; fMana = 1.0f; fWeaponType = 0; fWeaponLevel = 0; fArmorType = 0; fArmorLevel = 0;
		fHelmetLevel = 0; fHelmetType = 0;
	}

	// Functions needed to fulfill the Object interface
	virtual unsigned long GetId() const { return fUid; }
	virtual int GetType() const { return ObjTypePlayer; }
	virtual int GetLevel() const { return this->fLevel; }
	virtual glm::vec3 GetPosition()	const;
	virtual glm::vec3 GetSelectionColor() const; // The color to draw on the ground when object selected
	virtual bool IsDead(void) const { return this->fHp <= 0.0f; }
	virtual void RenderHealthBar(HealthBar *, float angle) const {} // Ignore for now
	virtual bool InGame(void) const; // Return true if this object is still i the game.

	// Test if player is below ground level.
	bool BelowGround(void) const;

	// Update with a new position. The argument is the feet of the player.
	// Coordinates are given in the server system.
	void SetPosition(signed long long newx, signed long long newy, signed long long newz);

	// Extrapolate new coordinates, guess from time.
	void UpdatePositionSmooth(void);

	// Return true if the player position is known.
	bool KnownPosition() const { return fKnownPosition; }

	bool PlayerIsMoving(void) const;
private:
	glm::dvec3 fServerPosition; // Most recent position of feet as given from server
	glm::dvec3 fPrevServerPosition;
	double fPrevUpdate, fLastUpdate;

	bool fMoving;
	bool fKnownPosition; // False until the position of the player is known.
};

extern player gPlayer;
