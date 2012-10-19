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
#include <GL/glew.h>

#include "messagedialog.h"
#include "../gamedialog.h"
#include "../Splitter.h"
#include "../Options.h"
#include "Error.h"

using std::string;

MessageDialog::MessageDialog() : fRocketContext(0), fDocument(0), fCallback(0) {
}

MessageDialog::~MessageDialog() {
	if (fDocument)
		fDocument->RemoveReference();
	if (fRocketContext)
		fRocketContext->RemoveReference();
}

void MessageDialog::Init(Rocket::Core::Context *context) {
	fRocketContext = context;
	fRocketContext->AddReference(); // This way, it will not be deallocated until we are done with it.
}

void MessageDialog::LoadDialog(const string &file) {
	if (fDocument != 0)
		this->Push();
	// Load and show the UI.
	fDocument = fRocketContext->LoadDocument(file.c_str());
	if (fDocument == 0)
		ErrorDialog("MessageDialog::LoadDialog: Failed to load %s", file.c_str());
	// Document is owned by the caller, which means reference count has already been incremented.
	fDocument->AddEventListener("click", this);
	fCallback = 0;
	fDocument->Show();
}

void MessageDialog::Set(const string &title, const string &body, void (*callback)(void)) {
	if (fDocument != 0)
		this->Push();
	// Load and show the UI.
	const char *fn = "dialogs/messagedialog.rml";
	fDocument = fRocketContext->LoadDocument(fn);
	if (fDocument == 0)
		ErrorDialog("MessageDialog::Init: Failed to load user interface '%s'", fn);
	// Document is owned by the caller, which means reference count has already been incremented.
	Rocket::Core::Element *header = fDocument->GetElementById("header");
	if (header)
		header->SetInnerRML(title.c_str());
	Rocket::Core::Element *content = fDocument->GetElementById("content");
	if (content)
		content->SetInnerRML(body.c_str());
	fDocument->AddEventListener("click", this);
	fCallback = callback;
	fDocument->Show();
}

// This event callback is generated automatically from libRocket.
// Notice that there may be "click" events for any part of the document.
void MessageDialog::ProcessEvent(Rocket::Core::Event& event) {
	Rocket::Core::Element *e = event.GetTargetElement(); // The element that generated the event.
	Rocket::Core::String type = event.GetType();
	string submit = e->GetAttribute("onsubmit", Rocket::Core::String("")).CString();
	if (submit != "") {
		// It was a "submit" event from a form element. The string in 'submit' is a feedback
		// that is currently not used.
		this->FormEvent(event, submit);
		return;
	}
	string attr = e->GetAttribute("onclick", Rocket::Core::String("")).CString();
	// Use the attribute "onclick" to determine what to do.
	this->ClickEvent(event, attr);
}

void MessageDialog::ClickEvent(Rocket::Core::Event& event, const string &action) {
	Splitter split(action, " ");
	if (action == "Close") {
		if (fCallback)
			(*fCallback)();
		if (!this->Pop())
			gGameDialog.ClearInputRedirect();
	} else if (split[0] == "Popup" && split.size() == 2) {
		this->Push();
		fDocument = fRocketContext->LoadDocument(("dialogs/" + split[1]).c_str());
		fDocument->AddEventListener("click", this);
		fCallback = 0;
		fDocument->Show();
	} else if (split[0] == "Form" && split.size() == 2) {
		// This is like "Popup", but a form with updated inputs will be shown.
		this->Push();
		fDocument = fRocketContext->LoadDocument(("dialogs/" + split[1]).c_str());
		fFormResultValues.clear(); // Restart with an empty list
		this->Treewalk(fDocument); // Fill default parameters in the document
		fDocument->AddEventListener("click", this);
		fDocument->AddEventListener("submit", this);
		fCallback = 0;
		fDocument->Show();
	} else if (action == "Quit") {
		if (fCallback)
			(*fCallback)();
		// Pop all saved documents, if any.
		while(this->Pop())
			continue;
		gGameDialog.ClearInputRedirect();
		gMode.Set(GameMode::ESC);
	} else if (action == "") {
		// No attribute given, ignore the event
	} else {
		ErrorDialog("MessageDialog::ProcessEvent Unknown 'onclick' attribute '%s' in %s", action.c_str(), fDocument->GetTitle().CString());
	}
}

void MessageDialog::FormEvent(Rocket::Core::Event& event, const string &action) {
	const Rocket::Core::Dictionary *dic = event.GetParameters();
	int size = dic->Size();
	// printf("Submit %s: size %d\n", action.c_str(), size);
	// Iterate through the list of arguments given to the event. This list only contains
	// elements that are used or filled with something. Cleared checkboxes are not included.
	for (int pos=0, i=0; i<size; i++) {
		Rocket::Core::String key;
		Rocket::Core::String value;
		dic->Iterate(pos, key, value);
		// printf("\tKey (%d): %s, value %s\n", pos, key.CString(), value.CString());
		fFormResultValues[key.CString()] = value.CString();
	}

	// printf("Results are:\n");
	// The resulting list in fFormResultValues now contains all the requested values.
	for (auto it = fFormResultValues.begin(); it != fFormResultValues.end(); it++) {
		// printf("\t%s:%s\n", it->first.c_str(), it->second.c_str());
		Options::fgOptions.ParseOneOption(it->first, it->second);
	}

	if (!this->Pop())
		gGameDialog.ClearInputRedirect();
}

void MessageDialog::CloseCurrentDocument(void) {
	fDocument->Hide();
	fDocument->RemoveReference();
	fDocument = 0;
}

void MessageDialog::Push(void) {
	fDocument->Hide();
	fStack.push_back(PushedDialog(fDocument, fCallback));
	fDocument = 0;
	fCallback = 0;
}

bool MessageDialog::Pop(void) {
	this->CloseCurrentDocument();
	if (fStack.size() == 0)
		return false;
	fDocument = fStack.back().first;
	fCallback = fStack.back().second;
	fStack.pop_back();
	fDocument->Show();
	return true;
}

void MessageDialog::Treewalk(Rocket::Core::Element *e) {
	int numChildren = e->GetNumChildren();
	for (int i=0; i<numChildren; i++) {
		Rocket::Core::Element *child = e->GetChild(i);
		this->Treewalk(child);
		this->UpdateInput(child);
	}
}

void MessageDialog::UpdateInput(Rocket::Core::Element *e) {
	Rocket::Core::String def = "";
	string tag = e->GetTagName().CString();
	string type = e->GetAttribute("type", def).CString();
	string name = e->GetAttribute("name", def).CString();
	string value = e->GetAttribute("value", def).CString();
	if (tag == "input") {
		fFormResultValues[name] = "";
		if (name == "Display.vsync" && Options::fgOptions.fVSYNC) {
			e->SetAttribute("checked", 1);
		}

		if (name == "Audio.musicvolume") {
			e->SetAttribute("value", Options::fgOptions.fMusicVolume);
		}
		if (name == "Audio.musicon" && Options::fgOptions.fMusicOn) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Audio.effectvolume") {
			e->SetAttribute("value", Options::fgOptions.fSoundFxVolume);
		}

		if (name == "Graphics.performance") {
			e->SetAttribute("value", Options::fgOptions.fPerformance);
		}
		if (name == "Graphics.fullscreen" && Options::fgOptions.fFullScreen) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Graphics.SmoothTerrain" && Options::fgOptions.fSmoothTerrain) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Graphics.MergeNormals" && Options::fgOptions.fMergeNormals) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Graphics.DynamicShadows" && Options::fgOptions.fDynamicShadows) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Graphics.StaticShadows" && Options::fgOptions.fStaticShadows) {
			e->SetAttribute("checked", 1);
		}
		if (name == "Graphics.DynamicShadows" && Options::fgOptions.fDynamicShadows) {
			e->SetAttribute("checked", 1);
		}
	} else if (tag == "option") {
		if (name == "Graphics.performance" && atoi(value.c_str()) == Options::fgOptions.fPerformance) {
			e->SetAttribute("selected", 1);
		}
	}
}
