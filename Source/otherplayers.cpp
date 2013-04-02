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

#include <GL/glew.h>
#include <GL/glfw.h>
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
#include "manageanimation.h"

using std::stringstream;
using std::endl;

using namespace Model;

struct OneOtherPlayer : public entityx::Component<OneOtherPlayer>, public Object {
	unsigned long id;
	signed long long x;
	signed long long y;
	signed long long z;
	bool ingame;
	unsigned char hp;
	unsigned int level;
	float fDir; // The direction the player is facing, in degrees
	double fUpdateTime; // The last time when this player was updated from the server.
	double lastTimeMoved; // The time when the monster last moved
	std::string playerName;
	virtual unsigned long GetId() const { return this->id; }
	virtual int GetType() const { return ObjTypePlayer; }
	virtual int GetLevel() const { return this->level; }
	virtual glm::vec3 GetPosition() const;
	virtual glm::vec3 GetSelectionColor() const; // The color to draw on the ground when object selected
	virtual bool IsDead(void) const { return hp == 0; }
	virtual void RenderHealthBar(View::HealthBar *, float angle) const;
	virtual bool InGame(void) const { return ingame;}
};

// @todo This OtherPlayerEvtReceiver should be combined into the OtherPlayers system?
struct OtherPlayerEvtReceiver : public entityx::Receiver<OtherPlayerEvtReceiver> {
	void receive(const OtherPlayerUpdateEvt &evt);
	void receive(const OtherPlayerNameEvt &evt);

	/// Register self for events
	void Init(OtherPlayers *pOtherPlayers, entityx::EventManager &events) {
		fOtherPlayers = pOtherPlayers;
		events.subscribe<OtherPlayerUpdateEvt>(*this);
		events.subscribe<OtherPlayerNameEvt>(*this);
	}

	OtherPlayers *fOtherPlayers; // Save pointer to the system that will use the event

	OtherPlayerEvtReceiver() : fOtherPlayers(0) {}
};

static OtherPlayerEvtReceiver sEventReceiver;

void OtherPlayerEvtReceiver::receive(const OtherPlayerUpdateEvt &evt) {
	fOtherPlayers->SetPlayer(evt.id, evt.hp, evt.level, evt.x, evt.y, evt.z, evt.dir);
}

void OtherPlayerEvtReceiver::receive(const OtherPlayerNameEvt &evt) {
	fOtherPlayers->SetPlayerName(evt.id, evt.name, evt.adminLevel);
}

std::shared_ptr<OtherPlayers> Model::gOtherPlayers = std::make_shared<OtherPlayers>();

void OtherPlayers::update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) {
	this->Cleanup();
}

void OtherPlayers::configure(entityx::EventManager &events) {
	sEventReceiver.Init(this, events);
}

glm::vec3 OneOtherPlayer::GetSelectionColor() const {
	return glm::vec3(-0.2f, 0.2f, -0.2f);
}

glm::vec3 OneOtherPlayer::GetPosition() const {
	ChunkCoord cc;
	gPlayer.GetChunkCoord(&cc);
	float dx = (this->x - (signed long long)cc.x*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dy = (this->y - (signed long long)cc.y*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	float dz = (this->z - (signed long long)cc.z*BLOCK_COORD_RES * CHUNK_SIZE)/(float)BLOCK_COORD_RES;
	// Add some corrections to get the coordinate of the feet.
	return glm::vec3(dx, dz-PLAYER_HEIGHT*2.0f, -dy);
}

void OtherPlayers::SetPlayer(unsigned long id, unsigned char hp, unsigned int level, signed long long x, signed long long y, signed long long z, float dir) {
	auto it = fEntities.find(id);
	if (it == fEntities.end()) {
		auto ent = fEntityManager->create();
		ent.assign<OneOtherPlayer>();
		fEntities[id] = ent;
	}
	auto ent = fEntities[id];
	std::shared_ptr<OneOtherPlayer> pl = ent.component<OneOtherPlayer>();

	if (pl->id == 0) {
		unsigned char b[7];
		b[0] = sizeof b;
		b[1] = 0;
		b[2] = CMD_REQ_PLAYER_INFO;
		EncodeUint32(b+3, id);
		SendMsg(b, sizeof b);
		pl->id = id;
	}
	// printf("Store info for player %d in slot %d (max: %d, fMaxIndex %d)\n", id, i, max, fMaxIndex);
	pl->ingame = true;
	if (pl->x != x || pl->y != y || pl->z != z) {
		if (!pl->IsDead()) {
			// TODO: There is a server bug that will make dead monsters move.
			pl->x = x;
			pl->y = y;
			pl->z = z;
			pl->lastTimeMoved = gCurrentFrameTime;
		}
	}
	pl->hp = hp;
	pl->level = level;
	pl->fDir = dir;
	pl->fUpdateTime = gCurrentFrameTime;
}

void OtherPlayers::SetPlayerName(unsigned long uid, const char *name, int adminLevel) {
	auto it = fEntities.find(uid);
	if (it == fEntities.end()) {
		return;
	}
	auto ent = it->second;
	auto pl = ent.component<OneOtherPlayer>();
	pl->playerName = name;
	// printf("Player %d got name %s and admin level %d\n", uid, it->second.playerName.c_str(), adminLevel);
}

void OtherPlayers::RenderPlayers(AnimationShader *animShader, bool selectionMode) const {
	float sun = 1.0f;
	if (gPlayer.BelowGround()) {
		// A gross simplification. If underground, disable all sun.
		sun = 0.0f;
	}
	glBindTexture(GL_TEXTURE_2D, GameTexture::RedColor);
	animShader->EnableProgram();
	std::shared_ptr<OneOtherPlayer> pl;
	for (auto entity : fEntityManager->entities_with_components(pl)) {
		if (!pl->ingame)
			continue;
		glm::vec3 pos = pl->GetPosition();

		glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
		model = glm::rotate(model, -pl->fDir, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(PLAYER_HEIGHT, PLAYER_HEIGHT, PLAYER_HEIGHT));

		if (pl->IsDead())
			View::gFrog.DrawAnimation(animShader, model, pl->lastTimeMoved, true, 0);
		else if (pl->lastTimeMoved + 0.2 > gCurrentFrameTime)
			View::gFrog.DrawAnimation(animShader, model, 0.0, false, 0);
		else
			View::gFrog.DrawAnimation(animShader, model, gCurrentFrameTime-0.22, false, 0); // Offset in time where model is not in a stride.

		if (!gOptions.fDynamicShadows || sun == 0)
			gShadows.Add(pos.x, pos.y, pos.z, 1.5f);
		(void)entity; // Unused
	}
	animShader->DisableProgram();
}

void OtherPlayers::RenderPlayerStats(View::HealthBar *hb, float angle) const {
	std::shared_ptr<OneOtherPlayer> pl;
	for (auto entity : fEntityManager->entities_with_components(pl)) {
		if (pl->ingame) {
			pl->RenderHealthBar(hb, angle);
		}
		(void)entity; // Unused
	}
}

void OtherPlayers::RenderMinimap(const glm::mat4 &miniMap,View:: HealthBar *hb) const {
	std::shared_ptr<OneOtherPlayer> pl;
	for (auto entity : fEntityManager->entities_with_components(pl)) {
		if (!pl->ingame || pl->IsDead())
			continue;
		auto pos = pl->GetPosition()/2.0f/float(CHUNK_SIZE);
		if (pos.x > 1.0f || pos.x < -1.0f || pos.y > 1.0f || pos.y < -1.0f || pos.z > 1.0f || pos.z < -1.0f)
			continue; // To far away on the radar
		// printf("OtherPlayers::RenderMinimap (%f, %f, %f)\n", pos.x, pos.y, pos.z);
		glm::mat4 model = glm::translate(miniMap, 0.5f-pos);
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		hb->DrawSquare(model, 0.0f, 1.0f, 1.0f, 1.0f);
		(void)entity; // Unused
	}
}

void OneOtherPlayer::RenderHealthBar(View::HealthBar *hb, float angle) const {
	auto pos = this->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
	model = glm::rotate(model, -angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Need to counter the rotation from the view matrix
	model = glm::scale(model, glm::vec3(2.0f, 0.2f, 1.0f));
	hb->DrawHealth(gProjectionMatrix, gViewMatrix * model, this->hp/255.0f, 0.0f, true);

	glm::vec4 v = glm::vec4(pos, 1.0f);
	v.y += 2.0f; // Just above
	glm::vec4 screen = gProjectionMatrix * gViewMatrix * v;
	screen = screen / screen.w;
	// printf("Selected: (%f,%f,%f,%f)\n", screen.x, screen.y, screen.z, screen.w);
	if (screen.z > 1)
		return; // Ignore display of Level information for monsters behind the camera
	gDrawFont.Enable();
	gDrawFont.UpdateProjection();
	gDrawFont.SetOffset(gViewport[2] * (screen.x+1)/2, gViewport[3] * (1-screen.y)/2);
	stringstream ss;
	if (this->playerName == "")
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
	std::shared_ptr<OneOtherPlayer> pl;
	for (auto entity : fEntityManager->entities_with_components(pl)) {
		if (!pl->ingame)
			continue;
		if (now - pl->fUpdateTime < 5.0)
			continue;
		// printf("Found stale player: index %d, id %ld\n", i, pl->id);
		pl->ingame = false;

		// Remove this players as a creature in SoundControl
		View::gSoundControl.RemoveCreatureSound(View::SoundControl::SOtherPlayer, pl->id);
		fEntities.erase(pl->id);
		entity.destroy();
	}
}
