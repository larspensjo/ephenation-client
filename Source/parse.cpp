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

#ifdef WIN32
#include <Winsock2.h>
#else
#include <sys/time.h>
#endif

#include <GL/glew.h>
#include <GL/glfw.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <math.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

using std::string;

string gParseMessageAtLogin;

#include "Inventory.h"
#include <glm/glm.hpp>
#include "parse.h"
#include "client_prot.h"
#include "player.h"
#include "connection.h"
#include "msgwindow.h"
#include "otherplayers.h"
#include "modes.h"
#include "gamedialog.h"
#include "monsters.h"
#include "SoundControl.h"
#include "ui/Error.h"
#include "ChunkProcess.h"
#include "SuperChunkManager.h"
#include "Debug.h"

#define NELEM(x) (sizeof x / sizeof x[0])

Simple::Signal<void (float dmg, unsigned long id)> gMonsterHitByPlayerEvt;
Simple::Signal<void (float dmg)> gPlayerHitByMonsterEvt;
Simple::Signal<void (const char *msg)> gServerMessageEvt;

using namespace Controller;
using View::SoundControl;
using View::gSoundControl;

void DumpBytes(const unsigned char *b, int n) {
	char buff[1000];
	sprintf(buff, "DumpBytes %d: ", n);
	for (int i=0; i<n; i++) {
		char buff2[10];
		sprintf(buff2, "%d ", b[i]);
		strcat(buff, buff2);
	}
	View::gMsgWindow.Add(buff);
}

// Parse a 16-bit unsigned from two bytes. LSB first.
unsigned short Parseuint16(const unsigned char *b) {
	return b[0] + (b[1]<<8);
}

// Parse a 64-bit unsigned from 8 bytes. LSB first.
unsigned long long Parseuint64(const unsigned char *b) {
	unsigned long long l = 0;
	for (int i=0; i<8; i++) {
		l |= (unsigned long long)b[i] << (i*8);
	}
	return l;
}

// Encode a 16 bit unsigned number as 4 bytes, LSB first.
void EncodeUint16(unsigned char *b, unsigned short val) {
	for (int i=0; i<2; i++) {
		b[i] = (val >> (i*8)) & 0xFF;
	}
}

// Encode a 64 bit unsigned number as 5 bytes, LSB first.
void EncodeUint40(unsigned char *b, unsigned long long val) {
	for (int i=0; i<5; i++) {
		b[i] = (val >> (i*8)) & 0xFF;
	}
}

// Encode a 32 bit unsigned number as 4 bytes, LSB first.
void EncodeUint32(unsigned char *b, unsigned int val) {
	for (int i=0; i<4; i++) {
		b[i] = (val >> (i*8)) & 0xFF;
	}
}

unsigned int ParseUint32(const unsigned char *b) {
	unsigned long ret = 0;
	for (int i=0; i<4; i++)
		ret |= b[i] << (i*8);
	return ret;
}

unsigned long long ParseUint64(const unsigned char *b) {
	unsigned long long ret = 0;
	for (int i=0; i<8; i++)
		ret |= b[i] << (i*8);
	return ret;
}

// 'b' points to the third byte in the message, when comparing to the protocol.
// 'n' is the number of remaining bytes (total message length minus 3).
static void ParseChunk(const unsigned char *b, int n) {
	// DumpBytes(b, n);
	unique_ptr<Model::ChunkBlocks> nc(new Model::ChunkBlocks); // A temporary ChunkBlock is needed.
	// Even though the chunk coordinates are not unsigned, they can be parsed as such.
	ChunkCoord cc;
	cc.x = ParseUint32(b+12);
	cc.y = ParseUint32(b+16);
	cc.z = ParseUint32(b+20);
	// LPLOG("ParseChunk: Got chunk (%d,%d,%d)", cc.x, cc.y, cc.z);
	nc->fChunk = ChunkFind(&cc, true); // Server only sends chunk data on demand, which means there should always be a chunk found at this time.
	// ASSERT(nc->fChunk != 0);        // This assertion did fail, sometimes. Because of that 'true' is used as argument above.
	nc->flag = ParseUint32(b);
	nc->fChecksum = ParseUint32(b+4);
	nc->fChecksumTestNeeded = false;
	nc->fChecksumTimeout = 0.0;
	nc->fOwner = ParseUint32(b+8);
	nc->fCompressedChunk.reset(new unsigned char[n-24]);
	memcpy(nc->fCompressedChunk.get(), b+24, n-24);
	nc->compressSize = n-24;
	gChunkProcess.AddTaskNewChunk(std::move(nc));
}

// Given only the LSB of the chunk coordinate, compute the full coordinate, relative to
// a given coordinate. A requirement is that the distance from the relative chunk is small.
ChunkCoord *UpdateLSB(ChunkCoord *cc, int x, int y, int z) {
	static ChunkCoord ret;

	ret.z = (cc->z & ~0xFF) | z; // Replace LSB
	ret.x = (cc->x & ~0xFF) | x; // Replace LSB
	ret.y = (cc->y & ~0xFF) | y; // Replace LSB

	// Check for wrap around, which can happen near byte boundary
	if (cc->x - ret.x > 127) { ret.x += 0x100; }
	if (cc->y - ret.y > 127) { ret.y += 0x100; }
	if (cc->z - ret.z > 127) { ret.z += 0x100; }
	if (ret.x - cc->x > 127) { ret.x -= 0x100; }
	if (ret.y - cc->y > 127) { ret.y -= 0x100; }
	if (ret.z - cc->z > 127) { ret.z -= 0x100; }
	return &ret;
}

static void ServerMessage(const char *msg) {
	if (msg[0] == '#') {
		// This is a special string to be used for sound effects.
		View::gSoundControl.RequestTrigSound(msg+1);
		if (strncmp(msg+1, "BOOM", 4) == 0)
			Controller::gGameDialog.RequestEffect(Controller::gameDialog::EFFECT_ZOOM2);
		if (strlen(msg) < 6)
			return; // That was all for now
		msg += 6; // Skip the sound string.
	}
	// A message that begins with '!!' is to be used for the dialog title.
	// A single '!' is used for the dialog content.
	if (msg[0] == '!') {
		if (msg[1] == '!') {
			sgPopupTitle = &msg[2];
			return;
		}
		sgPopup = sgPopup + &msg[1] + "\n";
		return;
	}
	gServerMessageEvt.emit(msg);
	LPLOG("Message '%s'", msg);
}

// Parse a command from the server. Notice that the length bytes are not included, which means
// 'n' is total message length - 2.
void Parse(const unsigned char *b, int n) {
	ChunkCoord cc;
	// ChunkOffsetCoord coc;
#if 0
	printf("Parse:");
	for (int i=0; i<n; i++)
		printf(" %d", b[i]);
	printf("\n");
#endif
	switch (b[0]) {
	case CMD_PLAYER_STATS: { // This message is generated on demand
		float hp = b[1] / 255.0f;
		unsigned long prevLevel = Model::gPlayer.fLevel;
		float prevExp = Model::gPlayer.fExp;
		Model::gPlayer.fExp = b[2] / 255.0f;
		if (Model::gPlayer.fExp > prevExp && Model::gPlayer.fStatsAvailable)
			View::gMsgWindow.Add("You gain %.1f%% experience points", (Model::gPlayer.fExp - prevExp)*100.0f);
		Model::gPlayer.fLevel = ParseUint32(b+3);
		Model::gPlayer.fFlags = ParseUint32(b+7);
		Model::gPlayer.fPreviousHp = Model::gPlayer.fHp;
		Model::gPlayer.fHp = hp;
		if (hp > Model::gPlayer.fPreviousHp) {
			// Special case: the previous hp is only used to compute damage, not healing
			Model::gPlayer.fPreviousHp = hp;
		}
		Model::gPlayer.fMana = b[11] / 255.0f;
		SoundControl::Sound sound = SoundControl::SNone;
		if (Model::gPlayer.fFlags & UserFlagHealed)
			sound |= SoundControl::SHealingSelf;
		if (prevLevel < Model::gPlayer.fLevel && Model::gPlayer.fStatsAvailable)
			sound |= SoundControl::SLevelUp;
		if (Model::gPlayer.fFlags & UserFlagJump)
			sound |= SoundControl::SPlayerLand;
		if (sound != SoundControl::SNone)
			gSoundControl.RequestSound(sound);
		Model::gPlayer.fStatsAvailable = true;
		// LPLOG("parse: New player stats hp %f, exp %f lvl %d, flags 0x%lx", Model::gPlayer.fHp, Model::gPlayer.fExp, Model::gPlayer.fLevel, Model::gPlayer.fFlags);
		break;
	}
	case CMD_REPORT_COORDINATE:
		Model::gPlayer.SetPosition((signed long long)Parseuint64(b+1), (signed long long)Parseuint64(b+9), (signed long long)Parseuint64(b+17));
		break;
	case CMD_CHUNK_ANSWER:
		ParseChunk(b+1, n-1);
		// LPLOG("Parse: Chunk answer %d,%d,%d", pc->cc.x, pc->cc.y, pc->cc.z);
		break;
	case CMD_OBJECT_LIST:
		// Report of various objects. Usually monsters or other players.
		for (int i=1; i<n; i += 18) {
			unsigned long id = ParseUint32(b+i);
			unsigned char state = b[i+4];
			unsigned char type = b[i+5];
			unsigned char hp = b[i+6];
			unsigned int level = ParseUint32(b+i+7);
			short dx = Parseuint16(b+i+11);
			short dy = Parseuint16(b+i+13);
			short dz = Parseuint16(b+i+15);
			float dir = b[17+i] / 256.0f * 365.0f; // Convert looking direction into degrees
			// printf("%d,", b[15+i]);
			if (type == 0 && state == 1) {
#if 0
				gDebugWindow.Add("Object id %d state %d type %d level %d hp %f moved to relative (%d,%d,%d) dir %f",
				                 id, state, type, level, (double)hp/255, dx, dy, dz, dir);
#endif
				Model::gOtherPlayers.SetPlayer(id, hp, level, Model::gPlayer.x+dx, Model::gPlayer.y+dy, Model::gPlayer.z+dz, dir);

				// Update this player as a creature in SoundControl
				gSoundControl.SetCreatureSound(SoundControl::SOtherPlayer,id,dx,dy,dz,hp==0,0.0f);
			} else {
#if 0
				gDebugWindow.Add("Object id %d state %d type %d level %d hp %f moved to relative (%d,%d,%d) dir %f",
				                 id, state, type, level, (double)hp/255, dx, dy, dz, dir);
#endif
				// Coordinates are relative to the player feet, while Model::gPlayer keeps track of the player head.
				Model::gMonsters.SetMonster(id, hp, level, Model::gPlayer.x+dx, Model::gPlayer.y+dy, Model::gPlayer.z+dz-(int)(PLAYER_HEIGHT*BLOCK_COORD_RES*2), dir);

				// Update this monster as a creature in SoundControl
				float size = Model::Monsters::Size(level)/5.0f; // Value is from 0.2 to 1.0
				gSoundControl.SetCreatureSound(SoundControl::SMonster,id,dx,dy,dz,hp==0,size);
			}
		}
		break;
	case CMD_BLOCK_UPDATE: {
		ChunkCoord cc;
		cc.x = ParseUint32(b+1);
		cc.y = ParseUint32(b+5);
		cc.z = ParseUint32(b+9);
		View::Chunk *cp = ChunkFind(&cc,false);
		if (cp == 0)
			break; // Chunk not found, ignore changes to it.
		auto cb = cp->fChunkBlocks;
		if (cb == 0)
			break;
		for (int i=13; i<n; i += 4) {
			int dx = b[i];
			int dy = b[i+1];
			int dz = b[i+2];
			int type = b[i+3];
			cb->CommandBlockUpdate(cc, dx, dy, dz, type);
			if (type == BT_Air)
				gSoundControl.RequestSound(SoundControl::SRemoveBlock);
			else
				gSoundControl.RequestSound(SoundControl::SBuildBlock);
			if (type == BT_Text) {
				Controller::gGameDialog.CreateActivatorMessage(dx, dy, dz, cc);
			}
		}
		cp->SetDirty(true);
		break;
	}
	case CMD_MESSAGE:
		ServerMessage((const char*)b+1);
		break;
	case CMD_REQ_PASSWORD:
		if (gMode.Get() == GameMode::PASSWORD) {
			gLoginChallenge.resize(n-1);
			memcpy(&gLoginChallenge[0], b+1, n-1); // Save the challenge bytes, to be used elsewhere
			gMode.Set(GameMode::REQ_PASSWD);
		} else if (gMode.Get() == GameMode::WAIT_ACK) {
			gMode.Set(GameMode::LOGIN_FAILED);
		}
		break;
	case CMD_LOGIN_ACK: {
		Model::gPlayer.fUid = ParseUint32(b+1);
		unsigned short angleHorRad = Parseuint16(b+5);
		signed short angleVertRad = Parseuint16(b+7);
		Model::gPlayer.fAngleHor = _angleHor = angleHorRad * (360.0f / 2 / M_PI / 100);
		Model::gPlayer.fAngleVert = _angleVert = angleVertRad * (360.0f / 2 / M_PI / 100);
		if (n >= 10)
			Model::gPlayer.fAdmin = b[9];
		else
			Model::gPlayer.fAdmin = 0;
		LPLOG("Player ID: %ld, admin %d", Model::gPlayer.GetId(), Model::gPlayer.fAdmin);
		gMode.Set(GameMode::GAME);
		break;
	}
	case CMD_LOGINFAILED: {
		gMode.Set(GameMode::LOGIN_FAILED);
		break;
	}
	case CMD_PROT_VERSION: {
		unsigned short minor = Parseuint16(b+1);
		unsigned short major = Parseuint16(b+3);
		if (major != PROT_VER_MAJOR) {
			// TODO: A better message is needed for Windows.
			ErrorDialog("Another version of the client is required (current %d.%d, required %d.%d\n", PROT_VER_MAJOR, PROT_VER_MINOR, major, minor);
		}
		if (minor != PROT_VER_MINOR) {
			std::stringstream ss;
			ss << "Using wrong communication prot version " << PROT_VER_MAJOR << "." << PROT_VER_MINOR <<
				"; but current is " << major << "." << minor << ".";
			gParseMessageAtLogin = ss.str();
			LPLOG("%s", gParseMessageAtLogin.c_str());
		}
		gClientAvailMinor = Parseuint16(b+5);
		gClientAvailMajor = Parseuint16(b+7);
		LPLOG("Current available client version is %d.%d", gClientAvailMajor, gClientAvailMinor);
		if (gMode.Get() == GameMode::INIT)
			gMode.Set(GameMode::LOGIN);
		break;
	}
	case CMD_RESP_PLAYER_HIT_BY_MONSTER:
		for (int i=1; i<n; i += 5) {
			// unsigned long id = ParseUint32(b+i);
			unsigned long dmg = b[i+4];
			gPlayerHitByMonsterEvt.emit(float(dmg)/255.0f);
		}
		break;
	case CMD_RESP_PLAYER_HIT_MONSTER: {
		for (int i=1; i<n; i += 5) {
			unsigned long id = ParseUint32(b+i);
			unsigned long dmg = b[i+4];
			gMonsterHitByPlayerEvt.emit(float(dmg)/255.0f, id);
		}
		break;
	}
	case CMD_RESP_AGGRO_FROM_MONSTER:
		for (int i=1; i<n; i += 4) {
			unsigned long id = ParseUint32(b+i);
			Controller::gGameDialog.AggroFrom(Model::gMonsters.Find(id));
		}
		break;
	case CMD_UPD_INV:
		for (int i=1; i<n; i += 9) {
			gInventory.SetAmount((const char *)(&b[i]), b[i+4], ParseUint32(&b[i+5]));
		}
		break;
	case CMD_EQUIPMENT: {
		// DumpBytes(b, n);
		unsigned long uid = ParseUint32(b+1);
		if (uid != Model::gPlayer.GetId())
			return; // TODO: save information about other players equipment
		// It is the complete list for this player. Iterate over every item in the list.
		for (int i=5; i<n; i += 9) {
			int slot = b[i];
			const char *equip = (const char *)&b[i+1];
			unsigned long lvl = ParseUint32(b+i+5);
			// gMsgWindow.Add("Player %d, equip slot %d with %4.4s", uid, slot, equip);
			switch (slot) {
			case 0: // Weapon slot
				Model::gPlayer.fWeaponLevel = lvl;
				Model::gPlayer.fWeaponType = equip[3]-'0'; // The equipment is a string WEPn, where 'n' is the type 1-4.
				// gMsgWindow.Add("Parse: Wep lvl %d, type %d (%4s)", Model::gPlayer.fWeaponLevel, Model::gPlayer.fWeaponType, equip);
				break;
			case 1: // Armor slot
				Model::gPlayer.fArmorLevel = lvl;
				Model::gPlayer.fArmorType = equip[3]-'0'; // The equipment is a string ARMn, where 'n' is the type 1-4.
				// gMsgWindow.Add("Parse: Arm lvl %d, type %d (%4s)", Model::gPlayer.fArmorLevel, Model::gPlayer.fArmorType, equip);
				break;
			case 2: // Helmet slot
				Model::gPlayer.fHelmetLevel = lvl;
				Model::gPlayer.fHelmetType = equip[3]-'0'; // The equipment is a string HLMn, where 'n' is the type 1-4.
				// gMsgWindow.Add("Parse: Helmet lvl %d, type %d (%4s)", Model::gPlayer.fHelmetLevel, Model::gPlayer.fHelmetType, equip);
				break;
			}
		}
		break;
	}
	case CMD_JELLY_BLOCKS: {
		// unsigned char flag = b[1]; // Currently unused
		unsigned char timeout = b[2];
		Model::gPlayer.GetChunkCoord(&cc);
		View::Chunk *pc = ChunkFind(UpdateLSB(&cc, b[3], b[4], b[5]), false);
		if (pc == 0)
			break; // Safety precaution
		auto cb = pc->fChunkBlocks;
		if (cb == 0)
			break;
		for (int i=6; i<n; i += 3) {
			unsigned char dx = b[i];
			unsigned char dy = b[i+1];
			unsigned char dz = b[i+2];
			cb->AddTimerJellyBlock(dx, dy, dz, double(timeout));
			cb->CommandBlockUpdate(cc, dx, dy, dz, BT_Air);
		}
		pc->SetDirty(true);
		break;
	}
	case CMD_PING: {
		if (b[1] == 0) {
			// It was a request, so we need to send a response
			unsigned char msg[4];
			msg[0] = sizeof msg;
			msg[1] = 0;
			msg[2] = CMD_PING;
			msg[3] = 1; // 0 means request, 1 means response
			SendMsg(msg, sizeof msg);
		} else {
			gCurrentPing = glfwGetTime() - gLastPing;
		}
		break;
	}
	case CMD_RESP_PLAYER_NAME: {
		unsigned long uid = ParseUint32(b+1);
		int adminLevel = b[5];
		Model::gOtherPlayers.SetPlayerName(uid, (const char *)b+6, n-6, adminLevel);
		break;
	}
	case CMD_SUPERCHUNK_ANSWER: {
		Model::gPlayer.GetChunkCoord(&cc);
		ChunkCoord *ret = UpdateLSB(&cc, b[1], b[2], b[3]);
		Model::gSuperChunkManager.Data(ret, b+4);
		break;
	}
	default:
		View::gMsgWindow.Add("Parse: Unknown server command %d, length %d", b[0], n);
		LPLOG("Unknown server command %d, length %d", b[0], n);
		DumpBytes(b, n);
		break;
	}
}
