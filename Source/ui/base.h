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

#include <Rocket/Core.h>

using std::string;

/**
 * @brief A common base class for libRocket dialogs
 *
 * The actual dialog content is defined in external files in dialogs/ *.rml (Rocket Markup Language),
 * in a format similar to html.
 * Inherit from Rocket::Core::EventListener to make it possible to register class for Rocket event callbacks.
 * There is a stack of documents, to able to have more than one active document at the same time. Though
 * only the document at the top of the stack will be shown. The stack has to be common between all
 * managers inheriting from this class, which is why it is declared as static.
 */
class BaseDialog : public Rocket::Core::EventListener {
public:
	BaseDialog() : fFocusOnTextarea(false) {}

	virtual ~BaseDialog();

	/**
	 * @brief Use the specified libRocket document
	 * The function has to be overrided to add proper event listeners.
	 */
	virtual void UseDocument(Rocket::Core::ElementDocument *, std::function<void()> callback) = 0;

	/// Dispatch a default action on the current dialog
	static void DispatchDefault();

	/// Dispatch a cancel action on the current dialog
	static void DispatchCancel();
protected:
	/// Generate a click event on the default button
	void DefaultButton();

	/// Generate a click on the Cancel or Close button
	void CancelButton();

	/**
	 * @brief When the document is a form, remember what input parameters are used and what values they are mapped to
	 */
	std::map<string, string> fFormResultValues;

	/**
	 * @brief Process the submit event on a form. Override this function if event is expected, but call this function first.
	 */
	virtual void FormEvent(Rocket::Core::Event& event, const string &action);

	/**
	 * @brief Process the click event on a document.
	 * @return true if the event was consumed.
	 * Override this function as needed, but call this first.
	 */
	virtual bool ClickEvent(Rocket::Core::Event& event, const string &action);

	/// Push a new dialog to the top of the stack.
	/// @param doc The new document.
	void Push(Rocket::Core::ElementDocument *doc, std::function<void()> callback);

	/**
	 * @brief Pop back the previous dialog.
	 * @return true if there is still an active document
	 */
	bool Pop();

	/**
	 * @brief Walk through the tree and upate all nodes.
	 */
	void Treewalk(Rocket::Core::Element *, std::function<void(Rocket::Core::Element *)>);

	// Start a treewalk on the current document.
	void Treewalk(std::function<void(Rocket::Core::Element *)>);

	/**
	 * @brief Find default buttons, and save them to make it possible to dispatch events.
	 */
	void DetectDefaultButton(Rocket::Core::Element *);

	/// Adds an event listener to this dialog manager.
	/// @param event Event to attach to.
	/// @param listener The listener object to be attached.
	void AddEventListener(const Rocket::Core::String& event, BaseDialog* listener);

	/// Show the current document
	void Show();

private:
	/// Flag if focus is on textarea
	bool fFocusOnTextarea;

	struct DialogState;
	/**
	 * @brief Need a stack to push multiple simultaneous dialogs.
	 * See further explanation in the class of the rational behind and use for this stack.
	 */
	static std::deque<DialogState> fStack;

	/**
	 * @brief Process all registered events from the current dialog.
	 */
	virtual void ProcessEvent(Rocket::Core::Event& event);
};
