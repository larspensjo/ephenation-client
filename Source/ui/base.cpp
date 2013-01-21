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
#include "factory.h"
#include "../gamedialog.h"
#include "../Splitter.h"
#include "../Options.h"
#include "../primitives.h"
#include "../connection.h"
#include "../SoundControl.h"
#include "../Inventory.h"
#include "../modes.h"

using std::string;
using std::stringstream;

std::deque<BaseDialog::DialogState> BaseDialog::fStack;

struct BaseDialog::DialogState {
	Rocket::Core::ElementDocument *fDocument;
	Rocket::Core::Element *fDefaultButton;
	Rocket::Core::Element *fCancelButton;
	BaseDialog *fBaseDialog;
	std::function<void()> fCallback; // Optional callback function to be called when dialog is closed.

	DialogState(Rocket::Core::ElementDocument *document, BaseDialog *theBaseDialog, std::function<void()> callback) {
		fDocument = document;
		fDefaultButton = 0;
		fCancelButton = 0;
		fBaseDialog = theBaseDialog;
		fCallback = callback;
	}
};

BaseDialog::~BaseDialog() {
	ASSERT(fStack.size() == 0);
}

// This event callback is generated automatically from libRocket.
// Notice that there may be "click" events for any part of the document.
void BaseDialog::ProcessEvent(Rocket::Core::Event& event) {
	Rocket::Core::Element *e = event.GetTargetElement(); // The element that generated the event.
	Rocket::Core::String tagname = e->GetTagName();
	Rocket::Core::String type = event.GetType();
	if (type == "submit") {
		// It was a "submit" event from a form element. The string in 'submit' is a feedback
		// that is currently not used.
		string submit = e->GetAttribute("onsubmit", Rocket::Core::String("")).CString();
		this->FormEvent(event, submit);
	} else if (type == "click") {
		string attr = e->GetAttribute("onclick", Rocket::Core::String("")).CString();
		// Use the attribute "onclick" to determine what to do.
		this->ClickEvent(event, attr);
	} else if (type == "focus") {
		if (tagname == "textarea") {
			fFocusOnTextarea = true;
		} else {
			fFocusOnTextarea = false;
		}
	}
}

bool BaseDialog::ClickEvent(Rocket::Core::Event& event, const string &action) {
	ASSERT(fStack.size() > 0);
	Splitter split(action, " ");
	if (action == "Close") {
		if (!this->Pop())
			gGameDialog.ClearInputRedirect();
		return true;
	} else if (split[0] == "Popup" && split.size() == 2) {
		gDialogFactory.Make(fStack.back().fDocument->GetContext(), split[1]);
		return true;
	} else if (split[0] == "Form" && split.size() == 2) {
		gDialogFactory.Make(fStack.back().fDocument->GetContext(), split[1]);
		return true;
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

void BaseDialog::Push(Rocket::Core::ElementDocument *doc, std::function<void()> callback) {
	if (fStack.size() > 0)
		fStack.back().fDocument->Hide();
	fStack.push_back(DialogState(doc, this, callback));
}

bool BaseDialog::Pop(void) {
	ASSERT (fStack.size() > 0);
	auto cb = fStack.back().fCallback;
	auto doc = fStack.back().fDocument;
	// Don't know why, but need to remove listeners or there will be spurious events even after document has been deallocated.
	doc->RemoveEventListener("click", this);
	doc->RemoveEventListener("submit", this);
	doc->Hide();
	doc->RemoveReference();
	fStack.pop_back();
	bool moreDialogs = false;
	if (fStack.size() > 0) {
		fStack.back().fDocument->Show();
		moreDialogs = true;
	}
	if (cb != nullptr)
		cb(); // There was a callback, call it.
	return moreDialogs;
}

void BaseDialog::Show() {
	fStack.back().fDocument->Show();
}

void BaseDialog::Treewalk(std::function<void(Rocket::Core::Element *)> func) {
	this->Treewalk(fStack.back().fDocument, func);
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
	ASSERT(fStack.size() > 0);
	Rocket::Core::String def = "false";
	auto enterkey = e->GetAttribute("enterkey", def);
	auto cancelkey = e->GetAttribute("cancelkey", def);
	if (enterkey == "true")
		fStack.back().fDefaultButton = e;
	if (cancelkey == "true")
		fStack.back().fCancelButton = e;
}

void BaseDialog::AddEventListener(const Rocket::Core::String& event, BaseDialog* listener) {
	fStack.back().fDocument->AddEventListener(event, listener);
}

void BaseDialog::DispatchCancel() {
	ASSERT(fStack.size() > 0);
	BaseDialog *d = fStack.back().fBaseDialog;
	d->CancelButton();
}

void BaseDialog::CancelButton() {
	if (!fStack.back().fCancelButton)
		return;
	Rocket::Core::Dictionary dic;
	fStack.back().fCancelButton->DispatchEvent("click", dic);
}

void BaseDialog::DispatchDefault() {
	ASSERT(fStack.size() > 0);
	BaseDialog *d = fStack.back().fBaseDialog;
	d->DefaultButton();
}

void BaseDialog::DefaultButton() {
	if (!fStack.back().fDefaultButton || fFocusOnTextarea)
		return;
	Rocket::Core::Dictionary dic;
	fStack.back().fDefaultButton->DispatchEvent("click", dic);
}
