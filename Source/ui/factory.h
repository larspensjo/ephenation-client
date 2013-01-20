// Copyright 2012-2013 The Ephenation Authors
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
#include <map>
#include <functional>

#include "memory"

namespace Rocket {
	namespace Core {
		class Context;
	}
};

class BaseDialog;

/// A factory that loads a document and returns the proper dialog handler for it
class DialogFactory
{
public:
	/// The constructor will initiate the factory
	DialogFactory();

	/// Make a new dialog handler.
	/// @param context The Rocket context to load the document into.
	/// @param file The definition of the document, in rml format. It will be loaded from the dialog folder.
	/// @param callback A function that will be called when dialog is closed. 'ok' is true if player did not cancel it.
	/// The attribute "handler" from the body of the document is used to select the dialog
	/// handler from the factory.
	void Make(Rocket::Core::Context *context, const std::string &file, std::function<void()> callback = nullptr);
private:
	std::map<std::string, BaseDialog* > fMap;
};

/// A global instance of the factory
extern DialogFactory gDialogFactory;
