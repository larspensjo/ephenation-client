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

#include <string>
#include <Rocket/Controls.h>
#include <sstream>

#include "options.h"
#include "../Options.h"
#include "../gamedialog.h"

void OptionsDialog::UseDocument(Rocket::Core::ElementDocument *doc, std::function<void()> callback) {
	this->Push(doc, callback); // Allways push the previous document first
	fFormResultValues.clear(); // Restart with an empty list
	this->Treewalk([this](Rocket::Core::Element *e){ this->UpdateInput(e);} ); // Fill default parameters in the document
	this->Treewalk([this](Rocket::Core::Element *e) {this->DetectDefaultButton(e); });
	this->AddEventListener("click", this);
	this->AddEventListener("submit", this);
	this->Show();
}

void OptionsDialog::UpdateInput(Rocket::Core::Element *e) {
	Rocket::Core::String def = "";
	auto tag = e->GetTagName();
	auto type = e->GetAttribute("type", def);
	string name = e->GetAttribute("name", def).CString();
	auto value = e->GetAttribute("value", def);
	if (tag == "input") {
		if (name != "")
			fFormResultValues[name] = "";

		if (name == "Display.vsync" && Options::sfSave.fVSYNC) {
			e->SetAttribute("checked", 1);
		}

		if (name == "Display.vsync" && Options::sfSave.fVSYNC) {
			e->SetAttribute("checked", 1);
		}

		if (name == "Audio.musicvolume") {
			e->SetAttribute("value", Options::sfSave.fMusicVolume);
		}
		if (name == "Audio.musicon" && Options::sfSave.fMusicOn) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Audio.effectvolume") {
			e->SetAttribute("value", Options::sfSave.fSoundFxVolume);
		}

		if (name == "Graphics.performance") {
			e->SetAttribute("value", Options::sfSave.fPerformance);
		}
		if (name == "Graphics.fullscreen" && Options::sfSave.fFullScreen) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Graphics.SmoothTerrain" && Options::sfSave.fSmoothTerrain) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Graphics.MergeNormals" && Options::sfSave.fMergeNormals) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Graphics.DynamicShadows" && Options::sfSave.fDynamicShadows) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Graphics.StaticShadows" && Options::sfSave.fStaticShadows) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Graphics.DynamicShadows" && Options::sfSave.fDynamicShadows) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Graphics.AddNoise" && Options::sfSave.fAddNoise) {
			e->SetAttribute("checked", 1);
		}
	} else if (tag == "option") {
		if (name == "Graphics.performance" && atoi(value.CString()) == Options::sfSave.fPerformance) {
			e->SetAttribute("selected", 1);
		}
		if (name == "Options.shadows") {
			// Map current configuration of shadows to the drop own list
			switch(atoi(value.CString())) {
			case 1:
				if (Options::sfSave.fStaticShadows == 0 && Options::sfSave.fDynamicShadows == 0)
					e->SetAttribute("selected", 1);
				break;
			case 2:
				if (Options::sfSave.fStaticShadows == 1 && Options::sfSave.fDynamicShadows == 0)
					e->SetAttribute("selected", 1);
				break;
			case 3:
				if (Options::sfSave.fStaticShadows == 0 && Options::sfSave.fDynamicShadows == 1)
					e->SetAttribute("selected", 1);
				break;
			}
		}
	} else if (tag == "select") {
		auto *element= dynamic_cast<Rocket::Controls::ElementFormControlSelect*>(e);
		if (element == 0)
			return;
		if (name == "Options.graphicalmode") {
			gOptions.ListGraphicModes(element);
		}
	}
}

void OptionsDialog::FormEvent(Rocket::Core::Event& event, const string &action) {
	BaseDialog::FormEvent(event, action); // Needed to perform some common preparations

	if (action == "options") {
		for (auto it = fFormResultValues.begin(); it != fFormResultValues.end(); it++) {
			// printf("\t%s:%s\n", it->first.c_str(), it->second.c_str());
			// First try the standard options and see if they match
			if (Options::sfSave.ParseOneOption(it->first, it->second)) {
				// Nothing just now
			} else if (it->first == "Options.shadows") {
				// Map the three alternativs to the two flags
				switch(atoi(it->second.c_str())) {
				case 1:
					Options::sfSave.fDynamicShadows = 0;
					Options::sfSave.fStaticShadows = 0;
					break;
				case 2:
					Options::sfSave.fDynamicShadows = 0;
					Options::sfSave.fStaticShadows = 1;
					break;
				case 3:
					Options::sfSave.fDynamicShadows = 1;
					Options::sfSave.fStaticShadows = 0;
					break;
				}
			} else if (it->first == "ping") {
				gShowPing = atoi(it->second.c_str());
			} else if (it->first == "Options.graphicalmode") {
				int mode = atoi(it->second.c_str());
				Options::sfSave.SetGraphicsMode(mode);
			}
		}

		if (!this->Pop())
			Controller::gGameDialog.ClearInputRedirect(); // Normal case
	}
}
