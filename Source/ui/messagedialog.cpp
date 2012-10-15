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
#include "Error.h"
#include "../gamedialog.h"
#include "../Splitter.h"

using std::string;

MessageDialog::MessageDialog() : fRocketContext(0), fDocument(0), fCallback(0) {
}

MessageDialog::~MessageDialog() {
	if (fDocument)
		fDocument->RemoveReference();
}

void MessageDialog::Init(Rocket::Core::Context *context) {
	fRocketContext = context;
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
	fDocument = fRocketContext->LoadDocument("dialogs/messagedialog.rml");
	if (fDocument == 0)
		ErrorDialog("MessageDialog::Init: Failed to load user interface");
	// Document is owned by the caller, which means reference count has already been incremented.
	Rocket::Core::Element *header = fDocument->GetElementById("header");
	Rocket::Core::Element *content = fDocument->GetElementById("content");
	if (header)
		header->SetInnerRML(title.c_str());
	if (content)
		content->SetInnerRML(body.c_str());
	fDocument->AddEventListener("click", this);
	fCallback = callback;
	fDocument->Show();
}

void MessageDialog::ProcessEvent(Rocket::Core::Event& event) {
	Rocket::Core::Element *e = event.GetTargetElement();
	string attr = e->GetAttribute("onclick", Rocket::Core::String("")).CString();
	// Use the argument to "onclick" to determine what to do.
	Splitter split(attr, " ");
	if (split[0] == "Close") {
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
	} else if (split[0] == "Quit") {
		if (fCallback)
			(*fCallback)();
		// Pop all saved documents, if any.
		while(this->Pop())
			continue;
		gGameDialog.ClearInputRedirect();
		gMode.Set(GameMode::ESC);
	} else if (attr == "") {
		// No attribute given, ignore the event
	} else {
		ErrorDialog("MessageDialog::ProcessEvent Unknown 'onclick' attribute '%s' in %s", attr.c_str(), fDocument->GetTitle().CString());
	}
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
