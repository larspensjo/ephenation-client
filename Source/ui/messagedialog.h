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

// This class manages in-game dialog messages.
#include <Rocket/Core.h>

using std::string;

class MessageDialog : public Rocket::Core::EventListener {
public:
	MessageDialog();
	~MessageDialog();

	// Define what context shall be used for the documents.
	void Init(Rocket::Core::Context *context);

	// Load a special dialog that onlhy shows a title and a message.
	void Set(const string &title, const string &body, void (*callback)(void));

	// Load a dialog from a file and show it
	void LoadDialog(const string &file);
private:
	// When the document is a form, remember what input parameters are used
	// and what values they are mapped to.
	std::map<string, string> fFormResultValues;

	// Process the incoming event.
	virtual void ProcessEvent(Rocket::Core::Event& event);

	// Called on submit of a form
	void FormEvent(Rocket::Core::Event& event, const string &submit);

	// Close the current document and free resources.
	void CloseCurrentDocument(void);

	Rocket::Core::Context *fRocketContext;
	Rocket::Core::ElementDocument *fDocument;
	void (*fCallback)(void);

	typedef std::pair<Rocket::Core::ElementDocument*, void (*)(void)> PushedDialog;
	std::deque<PushedDialog> fStack; // Need a stack to push multiple dialogs.
	void Push(void);
	bool Pop(void); // Return true if there was something to pop

	// Walk through the tree and upate all input values.
	void Treewalk(Rocket::Core::Element *);

	// Update inputs of this element (if there is any)
	void UpdateInput(Rocket::Core::Element *);
};
