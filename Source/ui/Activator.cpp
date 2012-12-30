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

#include <string>
#include <Rocket/Controls.h>

#include "Activator.h"
#include "../client_prot.h"
#include "../connection.h"
#include "../SoundControl.h"
#include "../Inventory.h"

#define NELEM(x) (sizeof(x) / sizeof (x[0]))

ActivatorDialog::ActivatorDialog(int dx, int dy, int dz, const ChunkCoord &cc) :
	fDx(dx), fDy(dy), fDz(dz), fCC(cc)
{
}

static const struct {
	const char *descr;
	const char *code;
	int dx, dy, dz;
} jellyList[] = {
	{ "None", "" },
	{ "West", "/jelly:w",  -1,0,0 },
	{ "North", "/jelly:n", 0,1,0 },
	{ "East", "/jelly:e",  1,0,0 },
	{ "South", "/jelly:s", 0,-1,0 },
	{ "Up", "/jelly:u",    0,0,1 },
	{ "Down", "/jelly:d",  0,0,-1 },
};

// For the given chunk, coordinate and direction, determine if the specified direction
// is a valid jelly direction.
static bool validJellyDir(const chunk *cp, int dir, int dx, int dy, int dz) {
	dx += jellyList[dir].dx;
	dy += jellyList[dir].dy;
	dz += jellyList[dir].dz;
	// First make sure destination block is inside current chunk.
	if (dx < 0 || dz >= CHUNK_SIZE || dy < 0 || dy >= CHUNK_SIZE || dz < 0 || dz >= CHUNK_SIZE)
		return false;
	if (cp == 0)
		return true; // Safe guard, should not happen
	int bl = cp->GetBlock(dx, dy, dz);
	return bl != BT_Air && bl != BT_Trigger && bl != BT_Text && bl != BT_Link;
}

void ActivatorDialog::UpdateInput(Rocket::Core::Element *e) {
	Rocket::Core::String def = "";
	auto tag = e->GetTagName();
	auto type = e->GetAttribute("type", def);
	string name = e->GetAttribute("name", def).CString();
	auto value = e->GetAttribute("value", def);

	if (tag == "select") {
		auto *element= dynamic_cast<Rocket::Controls::ElementFormControlSelect*>(e);
		if (element == 0)
			return;
		if (name == "Activator.soundeffect") {
			element->RemoveAll();
			for (unsigned i=0; i < gSoundControl.fNumTrigSounds; i++) {
				const SoundControl::TrigSoundItem *s = &SoundControl::fTrigSoundList[i];
				if (s->description == 0)
					break;
				element->Add(s->description, s->id);
			}
		} else if (name == "Activator.reward") {
			element->RemoveAll();
			for (unsigned i=0; i < Inventory::fsObjectMapSize; i++) {
				if (Inventory::fsObjectMap[i].code == 0)
					continue; // There are NULL objects that shall be ignored.
				element->Add(Inventory::fsObjectMap[i].descr, Inventory::fsObjectMap[i].code);
			}
		} else if (name == "Activator.jellyblock") {
			element->RemoveAll();
			chunk *cp = ChunkFind(&fCC, false);
			for (unsigned i=0; i<NELEM(jellyList); i++) {
				if (!validJellyDir(cp, i, fDx, fDy, fDz))
					continue;
				element->Add(jellyList[i].descr, jellyList[i].code);
			}
		}
	}
}

// Send a string to the server
static void SendString(const std::string &str) {
	unsigned int len = str.size() + 3;
	unsigned char buff[3];
	buff[0] = len & 0xFF;
	buff[1] = len >> 8;
	buff[2] = CMD_DEBUG;
	SendMsg(buff, 3);
	SendMsg((const unsigned char *)str.c_str(), len-3);
}

#if 0
// Ask the user for an activator message
static void CreateActivatorMessage(int dx, int dy, int dz, const ChunkCoord *cc) {
	// Show a dialog where the player can configure the activator block.
	Fl_Text_Buffer buf;
	Activator act(&buf+0);
	for (size_t i=0; i < gSoundControl.fNumTrigSounds; i++) {
		const char *descr = gSoundControl.fTrigSoundList[i].description;
		// A null pointer indicates that the rest are "private" sounds that may not be used in construction mode
		if (descr == 0)
			break;
		act.fSoundeffect->add(descr, 0, 0);
	}
	for (size_t i=0; i < gInventory.fsObjectMapSize; i++) {
		act.fObject->add(gInventory.fsObjectMap[i].descr);
	}
	chunk *cp = ChunkFind(cc, false);
	for (size_t i = 0; i < 7; i++)
		act.fJellyBlock->add(jellyList[i].descr, 0, 0, 0, !validJellyDir(cp, i, dx, dy, dz));
	act.fJellyBlock->value(0);
	Fl::run();
	Fl::flush();
	if (!act.fOk)
		return;

	// Interpret the result, constructing a string with all of it. Add the conditions first, and then the actions.
	string s;
	if (act.fKeyCondition->value()) {
		// A key condition is defined. This shall come before the broadcast flag, is the test is only applied to one player.
		// As the rest of the line is used as a description, a newline is needed.
		s = s + "/keycond:" + act.fCondKeyId->value() + "," + act.fCondOwnerKeyId->value() + " " + act.fCondKeyDescr->value() + "\n";
	}
	const char *p = act.fBlocks->value();
	if (p[0] == 0)
		p = "10";
	if (atoi(p) > 20)
		p = "20"; // Maximum distance limitation.
	if (act.fBroadcast->value()) // Is the broadcast checkbox enabled?
		s = s + "/broadcast:" + p + " ";
	p = act.fMonsterLevel->text();
	if (p == 0)
		p = "0";
	if (act.fSpawn->value())
		s = s + "/monster:" + p + " ";
	p = act.fInhibit->value();
	if (p[0] != 0) {
		s = s + "/inhibit:" + p + " ";
	}
	// TODO: Should use stringstream all the way
	stringstream ss;
	p = act.fMaxLevel->value();
	if (p[0] != 0) {
		int v = atoi(p)+1;
		ss << "/level<" << v << " ";
	}
	p = act.fMinLevel->value();
	if (p[0] != 0) {
		int v = atoi(p)-1;
		ss << "/level>" << v << " ";
	}
	s += ss.str();

	// Handle all actions
	int v = act.fObject->value();
	if (v != -1 && gInventory.fsObjectMap[v].code != 0)
		s = s + "/invadd:" + gInventory.fsObjectMap[v].code + " ";
	v = act.fJellyBlock->value();
	if (v > 0) {
		s = s + jellyList[v].code + " ";
	}
	// The sound effect must come last, but before the other texts.
	v = act.fSoundeffect->value();
	if (v != -1)
		s = s + "#" + gSoundControl.fTrigSoundList[v].id + " ";
	if (act.fAddKey->value()) {
		// The key is special, as it contains a key description until end of line. Because of that,
		// a newline is needed after the description.
		char buff[10];
#ifdef WIN32
		_snprintf(buff, sizeof buff, "%d", act.fKeyPicture->value());
#else
		snprintf(buff, sizeof buff, "%d", act.fKeyPicture->value());
#endif
		s = s + "/addkey:" + act.fKeyId->value() + "," + buff + " " + act.fKeyDescr->value() + "\n";
	}
	// Finally, add the message itself
	s += buf.text();

	// Strip trailing spaces, if any
	size_t end = s.find_last_not_of(' ');
	if (end != string::npos)
		s = s.substr(0, end+1);
	size_t start = 0;
	while(start < s.length()) {
		end = s.find('\n', start);
		if (end == s.npos) {
			end = s.length();
		}
		char buff[200];
#ifdef WIN32
		_snprintf(buff, sizeof buff, "/activator add %d %d %d %d %d %d %s", cc->x, cc->y, cc->z, dx, dy, dz, s.substr(start, end-start).c_str());
#else
		snprintf(buff, sizeof buff, "/activator add %d %d %d %d %d %d %s", cc->x, cc->y, cc->z, dx, dy, dz, s.substr(start, end-start).c_str());
#endif
		SendString(buff);
		start = end+1; // Skip the newline
	}
}

#endif
