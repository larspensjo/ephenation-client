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

#pragma once

#include <string>
#include <deque>
#include <utility>
#include <map>

// This class manages in-game dialog messages. It is based on the libRocket library.
// The actual dialog content is defined in external files in dialogs/*.rml (Rocket Markup Language),
// in a format very similar to html.
#include <Rocket/Core.h>

using std::string;

class MessageDialog : public Rocket::Core::EventListener {
public:
	MessageDialog();
	~MessageDialog();

	// Define what Rocket context shall be used for the documents.
	void Init(Rocket::Core::Context *context);

	// Load a special dialog that onlhy shows a title and a message.
	void Set(const string &title, const string &body, void (*callback)(void));

	// Load a dialog from a file and show it.
	void LoadDialog(const string &file);

	// Like LoadDialog, but a form that accepts input
	void LoadForm(const string &file);

	// Generate a click event on the default button
	void DefaultButton(void);

	// Generate a click on the Cancel or Close button
	void CancelButton(void);
private:
	// When the document is a form, remember what input parameters are used
	// and what values they are mapped to.
	std::map<string, string> fFormResultValues;

	Rocket::Core::Context *fRocketContext; // Remember the Rocket context.

	Rocket::Core::ElementDocument *fDocument; // The currently active document, if any
	Rocket::Core::Element *fCurrentDefaultButton, *fCurrentCloseButton;
	void (*fCallback)(void); // Call back for the current document, if any

	struct PushedDialog;
	std::deque<PushedDialog> fStack; // Need a stack to push multiple simultaneous dialogs.

	// Process all registered events from the current dialog.
	virtual void ProcessEvent(Rocket::Core::Event& event);

	// Process the submit event on a form.
	void FormEvent(Rocket::Core::Event& event, const string &action);

	// Process the click event on a document.
	void ClickEvent(Rocket::Core::Event& event, const string &action);

	// Close the current document and free resources.
	void CloseCurrentDocument(void);

	// Push the current dialog.
	void Push(void);

	// Pop back the previous dialog. Return true if there was something to pop
	bool Pop(void);

	// Walk through the tree and upate all nodes.
	void Treewalk(Rocket::Core::Element *, void (MessageDialog::*)(Rocket::Core::Element *));

	// Update inputs of this specific element (if there is any).
	void UpdateInput(Rocket::Core::Element *);

	// Find default buttons
	void DetectDefaultButton(Rocket::Core::Element *);
};
