// Copyright 2012,2013 The Ephenation Authors
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
#include <sstream>
#include <iostream>

#include "activator.h"
#include "../client_prot.h"
#include "../connection.h"
#include "../SoundControl.h"
#include "../Inventory.h"
#include "../gamedialog.h"

#define NELEM(x) (sizeof(x) / sizeof (x[0]))

void ActivatorDialog::UseDocument(Rocket::Core::ElementDocument *doc, std::function<void()> callback) {
	this->Push(doc, callback); // Allways push the previous document first
	gGameDialog.GetActivator(fDx, fDy, fDz, fCC);
	fFormResultValues.clear(); // Restart with an empty list
	this->Treewalk([this](Rocket::Core::Element *e){ this->UpdateInput(e);} ); // Fill default parameters in the document
	this->Treewalk([this](Rocket::Core::Element *e) {this->DetectDefaultButton(e); }); // This can be done by the local callback
	this->AddEventListener("click", this);
	this->AddEventListener("submit", this);
	this->AddEventListener("focus", this); // Need this to track where the input focus is, to disable default button on textarea.
	this->Show();
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

void ActivatorDialog::FormEvent(Rocket::Core::Event& event, const string &action) {
	BaseDialog::FormEvent(event, action); // Needed to perform some common preparations
	if (action != "activator")
		return;

	// Interpret the result, constructing a string with all of it. Add the conditions first, and then the actions.
	std::stringstream ss;
	ss << "/activator add " << fCC.x << " " << fCC.y << " " << fCC.z << " " << fDx << " " << fDy << " " << fDz << " ";

	if (fFormResultValues["Activator.condkey"] == "1") {
		// A key condition is defined. This shall come before the broadcast flag, as the test is only applied to one player.
		// The rest of the line is used as a description, and a newline is needed.
		ss << "/keycond:" << fFormResultValues["Activator.condkey.key"] << "," << fFormResultValues["Activator.condkey.owner"] << " " << fFormResultValues["Activator.condkey.message"] << "\n";
	}

	string blocks = fFormResultValues["Activator.boradcastdist"];
	if (blocks == "")
		blocks = "10";
	if (atoi(blocks.c_str()) > 20)
		blocks = "20"; // Maximum distance limitation.
	if (fFormResultValues["Activator.broadcast"] != "") // Is the broadcast checkbox enabled?
		ss << "/broadcast:" << blocks << " ";

	string monsterLevel = fFormResultValues["Activator.spawnmonster"];
	if (monsterLevel != "")
		ss << "/monster:" << monsterLevel << " ";

	string inhibit = fFormResultValues["Activator.inhbit"];
	if (inhibit != "") {
		ss << "/inhibit:" << inhibit << " ";
	}

	string maxLevel = fFormResultValues["Activator.player.level.max"];
	if (maxLevel != "") {
		int v = atoi(maxLevel.c_str())+1;
		ss << "/level<" << v << " ";
	}

	string minLevel = fFormResultValues["Activator.player.level.min"];
	if (minLevel != "") {
		int v = atoi(minLevel.c_str())-1;
		ss << "/level>" << v << " ";
	}

	// Handle all actions
	string code = fFormResultValues["Activator.reward"];
	if (code != "")
		ss << "/invadd:" << code << " ";


	string jellyblock = fFormResultValues["Activator.jellyblock"];
	if (jellyblock != "") {
		ss << jellyblock << " ";
	}

	if (fFormResultValues["Activator.addkey"] == "1") {
		// The key is special, as it contains a key description until end of line. Because of that,
		// a newline is needed after the description.
		/// @todo The key picture is hard coded as 0.
		ss << "/addkey:" << fFormResultValues["Activator.addkey.id"] << ",0 " << fFormResultValues["Activator.addkey.text"] << "\n";
	}

	// The sound effect must come last, but before the other texts.
	string soundeffect = fFormResultValues["Activator.soundeffect"];
	if (soundeffect != "")
		ss << "#" << soundeffect << " ";

	// Add the message itself
	ss << fFormResultValues["Activator.message"];

	// Strip trailing spaces, if any
	string s = ss.str();
	size_t end = s.find_last_not_of(' ');
	if (end != string::npos)
		s = s.substr(0, end+1);

	SendString(s.c_str());
	// std::cout << s << std::endl;

	if (!this->Pop())
		gGameDialog.ClearInputRedirect(); // Normal case
}
