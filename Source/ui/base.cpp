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
#include <GL/glew.h>
#include <sstream>
#include <Rocket/Controls.h>

#include "base.h"
#include "Error.h"
#include "factory.h"
#include "../gamedialog.h"
#include "../Splitter.h"
#include "../Options.h"
#include "../primitives.h"
#include "../connection.h"
#include "../SoundControl.h"
#include "../Inventory.h"

using std::string;
using std::stringstream;

std::deque<BaseDialog::DialogState> BaseDialog::fStack;

struct BaseDialog::DialogState {
	Rocket::Core::ElementDocument *fDocument;
	Rocket::Core::Element *fDefaultButton;
	Rocket::Core::Element *fCancelButton;
	BaseDialog *fBaseDialog;
	Rocket::Core::Context *fContext;

	DialogState(Rocket::Core::ElementDocument *document, Rocket::Core::Element *enter, Rocket::Core::Element *close, BaseDialog *theBaseDialog, Rocket::Core::Context *theContext) {
		fDocument = document;
		fDefaultButton = enter;
		fCancelButton = close;
		fBaseDialog = theBaseDialog;
		fContext = theContext;
	}
};

BaseDialog::BaseDialog() : fDocument(0), fRocketContext(0), fCurrentDefaultButton(0),
	fCurrentCloseButton(0)
{
}

BaseDialog::~BaseDialog() {
	if (fDocument)
		fDocument->RemoveReference();
	if (fRocketContext)
		fRocketContext->RemoveReference();
}

void BaseDialog::Init(Rocket::Core::Context *context) {
	fRocketContext = context;
	fRocketContext->AddReference(); // This way, it will not be deallocated until we are done with it.
}

// This event callback is generated automatically from libRocket.
// Notice that there may be "click" events for any part of the document.
void BaseDialog::ProcessEvent(Rocket::Core::Event& event) {
	Rocket::Core::Element *e = event.GetTargetElement(); // The element that generated the event.
	Rocket::Core::String type = event.GetType();
	string submit = e->GetAttribute("onsubmit", Rocket::Core::String("")).CString();
	if (submit != "") {
		// It was a "submit" event from a form element. The string in 'submit' is a feedback
		// that is currently not used.
		this->FormEvent(event, submit);
	} else {
		string attr = e->GetAttribute("onclick", Rocket::Core::String("")).CString();
		// Use the attribute "onclick" to determine what to do.
		this->ClickEvent(event, attr);
	}
}

bool BaseDialog::ClickEvent(Rocket::Core::Event& event, const string &action) {
	Splitter split(action, " ");
	if (action == "Close") {
		if (!this->Pop())
			gGameDialog.ClearInputRedirect();
		return true;
	} else if (split[0] == "Popup" && split.size() == 2) {
		gDialogFactory.Make(fRocketContext, split[1]);
		this->Push();
		fDocument = fRocketContext->LoadDocument(("dialogs/" + split[1]).c_str());
		fDocument->AddEventListener("click", this);
		this->Treewalk(fDocument, [this](Rocket::Core::Element *e) {this->DetectDefaultButton(e); });
		fDocument->Show();
	} else if (split[0] == "Form" && split.size() == 2) {
		this->Push();
		// this->LoadForm(split[1]);
	} else if (action == "Quit") {
		// Pop all saved documents, if any.
		while(this->Pop())
			continue;
		gGameDialog.ClearInputRedirect();
		gMode.Set(GameMode::ESC);
		return true;
	} else if (action == "") {
		// No attribute given, ignore the event
		return true;
	}
	return false;
}

void BaseDialog::FormEvent(Rocket::Core::Event& event, const string &action) {
	// Some common logic needed by all forms.
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
	// The resulting list in fFormResultValues now contains all the requested values.
}

void BaseDialog::CloseCurrentDocument(void) {
	// Don't know why, but need to remove listeners or there will be spurious events even after document has been deallocated.
	fDocument->RemoveEventListener("click", this);
	fDocument->RemoveEventListener("submit", this);
	fDocument->Hide();
	fDocument->RemoveReference();
	fDocument = 0;
}

void BaseDialog::Push() {
	if (fDocument)
		fDocument->Hide();
	fStack.push_back(DialogState(fDocument, fCurrentDefaultButton, fCurrentCloseButton, this, fRocketContext));
	fDocument = 0;
	fCurrentCloseButton = 0;
	fCurrentDefaultButton = 0;
}

bool BaseDialog::Pop(void) {
	this->CloseCurrentDocument();
	ASSERT (fStack.size() > 0);
	fDocument = fStack.back().fDocument;
	fCurrentDefaultButton = fStack.back().fDefaultButton;
	fCurrentCloseButton = fStack.back().fCancelButton;
	fStack.pop_back();
	if (fDocument) {
		fDocument->Show();
		return true;
	}
	return false;
}

void BaseDialog::Treewalk(Rocket::Core::Element *e, std::function<void(Rocket::Core::Element *)> func) {
	int numChildren = e->GetNumChildren();
	for (int i=0; i<numChildren; i++) {
		Rocket::Core::Element *child = e->GetChild(i);
		this->Treewalk(child, func);
		func(child);
	}
}

void BaseDialog::DetectDefaultButton(Rocket::Core::Element *e) {
	Rocket::Core::String def = "false";
	auto enterkey = e->GetAttribute("enterkey", def);
	auto cancelkey = e->GetAttribute("cancelkey", def);
	if (enterkey == "true")
		fCurrentDefaultButton = e;
	if (cancelkey == "true")
		fCurrentCloseButton = e;
}

void BaseDialog::CancelButton(void) {
	if (!fCurrentCloseButton)
		return;
	Rocket::Core::Dictionary dic;
	fCurrentCloseButton->DispatchEvent("click", dic);
}

void BaseDialog::DefaultButton(void) {
	if (!fCurrentDefaultButton)
		return;
	Rocket::Core::Dictionary dic;
	fCurrentDefaultButton->DispatchEvent("click", dic);
}
