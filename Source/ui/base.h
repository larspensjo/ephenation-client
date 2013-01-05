// Copyright 2012, 2013 The Ephenation Authors
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
#include <functional>

// A common base class for libRocket dialogs.
// The actual dialog content is defined in external files in dialogs/*.rml (Rocket Markup Language),
// in a format similar to html.
#include <Rocket/Core.h>

using std::string;
class ChunkCoord;

// Inherit from Rocket::Core::EventListener to make it possible to register class for Rocket event callbacks.
class BaseDialog : public Rocket::Core::EventListener {
public:
	// Define a handler name to use for registering in the factory
	BaseDialog();
	~BaseDialog();

	// Define what Rocket context shall be used for the documents. TODO: Should use a private context, instead of a copy of a global one.
	void Init(Rocket::Core::Context *context);

	// Generate a click event on the default button
	void DefaultButton(void);

	// Generate a click on the Cancel or Close button
	void CancelButton(void);

	// Use the specified document.
	// The function has to be overrided to add proper event listeners.
	virtual void UseDocument(Rocket::Core::ElementDocument *) = 0;
protected:
	// When the document is a form, remember what input parameters are used
	// and what values they are mapped to.
	std::map<string, string> fFormResultValues;

	Rocket::Core::ElementDocument *fDocument; // The currently active document, if any

	// Process the submit event on a form. Override this function if event is expected, but call this function first.
	virtual void FormEvent(Rocket::Core::Event& event, const string &action);

	// Process the click event on a document.
	// Return true if the event was consumed.
	// Override this function as needed, but call this first.
	virtual bool ClickEvent(Rocket::Core::Event& event, const string &action);

	// Push the current dialog.
	void Push(void);

	// Pop back the previous dialog. Return true if there is still an active document
	bool Pop(void);

	// Walk through the tree and upate all nodes.
	void Treewalk(Rocket::Core::Element *, std::function<void(Rocket::Core::Element *)>);

	// Find default buttons, and save them to make it possible to
	// dispatch events.
	void DetectDefaultButton(Rocket::Core::Element *);

private:
	Rocket::Core::Context *fRocketContext; // Remember the Rocket context.
	Rocket::Core::Element *fCurrentDefaultButton, *fCurrentCloseButton;

	struct PushedDialog;
	std::deque<PushedDialog> fStack; // Need a stack to push multiple simultaneous dialogs.

	// Process all registered events from the current dialog.
	virtual void ProcessEvent(Rocket::Core::Event& event);

	// Close the current document and free resources.
	void CloseCurrentDocument(void);
};
