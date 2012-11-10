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
#include <sstream>
#include <Rocket/Controls.h>

#include "messagedialog.h"
#include "../gamedialog.h"
#include "../Splitter.h"
#include "../Options.h"
#include "../primitives.h"
#include "../connection.h"
#include "Error.h"

using std::string;
using std::stringstream;

struct MessageDialog::PushedDialog {
	Rocket::Core::ElementDocument *fDocument;
	void (*fCallback)(void);
	Rocket::Core::Element *fDefaultButton;
	Rocket::Core::Element *fCancelButton;

	PushedDialog(Rocket::Core::ElementDocument *document, void (*callback)(void), Rocket::Core::Element *enter, Rocket::Core::Element *close) {
		fDocument = document;
		fCallback = callback;
		fDefaultButton = enter;
		fCancelButton = close;
	}
};

MessageDialog::MessageDialog() : fRocketContext(0), fDocument(0), fCurrentDefaultButton(0), fCurrentCloseButton(0), fCallback(0) {
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
	this->Treewalk(fDocument, &MessageDialog::DetectDefaultButton);
	fDocument->Show();
}

void MessageDialog::LoadForm(const string &file) {
	// This is like "Popup", but a form with updated inputs will be shown.
	fDocument = fRocketContext->LoadDocument(("dialogs/" + file).c_str());
	fFormResultValues.clear(); // Restart with an empty list
	this->Treewalk(fDocument, &MessageDialog::UpdateInput); // Fill default parameters in the document
	this->Treewalk(fDocument, &MessageDialog::DetectDefaultButton);
	fDocument->AddEventListener("click", this);
	fDocument->AddEventListener("submit", this);
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
	this->Treewalk(fDocument, &MessageDialog::DetectDefaultButton);
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
		this->Treewalk(fDocument, &MessageDialog::DetectDefaultButton);
		fCallback = 0;
		fDocument->Show();
	} else if (split[0] == "Form" && split.size() == 2) {
		this->Push();
		this->LoadForm(split[1]);
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

	if (action == "login") {
		PerformLoginProcedure(fFormResultValues["Login.email"], fFormResultValues["Login.licensekey"], fFormResultValues["Login.password"], false);
		// printf("Login %s, %s, %s\n", fFormResultValues["Login.licensekey"].c_str(), fFormResultValues["Login.email"].c_str(), fFormResultValues["Login.password"].c_str());
	} else {
		// The resulting list in fFormResultValues now contains all the requested values.
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
	}

	if (!this->Pop())
		gGameDialog.ClearInputRedirect();
}

void MessageDialog::CloseCurrentDocument(void) {
	// Don't know why, but need to remove listeners or there will be spurious events even after document has been deallocated.
	fDocument->RemoveEventListener("click", this);
	fDocument->RemoveEventListener("submit", this);
	fDocument->Hide();
	fDocument->RemoveReference();
	fDocument = 0;
}

void MessageDialog::Push(void) {
	fDocument->Hide();
	fStack.push_back(PushedDialog(fDocument, fCallback, fCurrentDefaultButton, fCurrentCloseButton));
	fDocument = 0;
	fCallback = 0;
	fCurrentCloseButton = 0;
	fCurrentDefaultButton = 0;
}

bool MessageDialog::Pop(void) {
	this->CloseCurrentDocument();
	if (fStack.size() == 0)
		return false;
	fDocument = fStack.back().fDocument;
	fCallback = fStack.back().fCallback;
	fCurrentDefaultButton = fStack.back().fDefaultButton;
	fCurrentCloseButton = fStack.back().fCancelButton;
	fStack.pop_back();
	fDocument->Show();
	return true;
}

void MessageDialog::Treewalk(Rocket::Core::Element *e, void (MessageDialog::*func)(Rocket::Core::Element *)) {
	int numChildren = e->GetNumChildren();
	for (int i=0; i<numChildren; i++) {
		Rocket::Core::Element *child = e->GetChild(i);
		this->Treewalk(child, func);
		(this->*func)(child);
	}
}

void MessageDialog::UpdateInput(Rocket::Core::Element *e) {
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

		if (name == "Login.licensekey") {
			e->SetAttribute("value", Options::sfSave.fLicenseKey.c_str());
		}
		if (name == "Login.email") {
			e->SetAttribute("value", Options::sfSave.fEmail.c_str());
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
	} else if (tag == "p") {
		if (name == "client.availableversion") {
			stringstream ss;
			if (gClientAvailMajor != CLIENT_MAJOR_VERSION || gClientAvailMinor != CLIENT_MINOR_VERSION)
				ss << "<p style=\"color:red;\">New client version available: " << gClientAvailMajor << "." << gClientAvailMinor << "</p>";
			else
				ss << "Client version: " << gClientAvailMajor << "." << gClientAvailMinor;
			e->SetInnerRML(ss.str().c_str());
		}
	} else if (tag == "select") {
		auto *element= dynamic_cast<Rocket::Controls::ElementFormControlSelect*>(e);
		if (element && name == "Options.graphicalmode") {
			gOptions.ListGraphicModes(element);
		} else if (name == "Activator.soundeffect") {
		}
	}
}

void MessageDialog::DetectDefaultButton(Rocket::Core::Element *e) {
	Rocket::Core::String def = "false";
	auto enterkey = e->GetAttribute("enterkey", def);
	auto cancelkey = e->GetAttribute("cancelkey", def);
	if (enterkey == "true")
		fCurrentDefaultButton = e;
	if (cancelkey == "true")
		fCurrentCloseButton = e;
}

void MessageDialog::CancelButton(void) {
	if (!fCurrentCloseButton)
		return;
	Rocket::Core::Dictionary dic;
	fCurrentCloseButton->DispatchEvent("click", dic);
}

void MessageDialog::DefaultButton(void) {
	if (!fCurrentDefaultButton)
		return;
	Rocket::Core::Dictionary dic;
	fCurrentDefaultButton->DispatchEvent("click", dic);
}
