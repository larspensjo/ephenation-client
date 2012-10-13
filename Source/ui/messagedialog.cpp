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

#include "messagedialog.h"
#include "Error.h"
#include "../gamedialog.h"

using std::string;

MessageDialog::MessageDialog() : fRocketContext(0), fDocument(0), fHeader(0), fContent(0), fCallback(0) {
}

MessageDialog::~MessageDialog() {
	fDocument->RemoveReference();
	if (fHeader)
		fHeader->RemoveReference();
	if (fContent)
		fContent->RemoveReference();
}

void MessageDialog::Init(Rocket::Core::Context *context) {
	fRocketContext = context;

	// Load and show the UI.
	fDocument = fRocketContext->LoadDocument("dialogs/messagedialog.rml");
	if (fDocument == 0)
		ErrorDialog("MessageDialog::Init: Failed to load user interface");
	fDocument->AddReference();
	fHeader = fDocument->GetElementById("header");
	if (fHeader)
		fHeader->AddReference();
	fContent = fDocument->GetElementById("content");
	if (fContent)
		fContent->AddReference();
	Rocket::Core::Element *closeButton = fDocument->GetElementById("closebutton");
	if (closeButton)
		closeButton->AddEventListener("click", this);
}

void MessageDialog::Set(const string &title, const string &body, void (*callback)(void)) {
	if (fHeader)
		fHeader->SetInnerRML(title.c_str());
	if (fContent)
		fContent->SetInnerRML(body.c_str());
	fCallback = callback;
	fDocument->Show();
}

void MessageDialog::Draw() {
	fRocketContext->Update();
	fRocketContext->Render();
}

void MessageDialog::ProcessEvent(Rocket::Core::Event& event) {
	fDocument->Hide();
	if (fCallback)
		(*fCallback)();
	gGameDialog.ClearInputRedirect();
}
