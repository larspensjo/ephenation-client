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

#include <Rocket/Core.h>
#include <GL/glew.h> // Used by chunk.h in activator.h

#include "factory.h"
#include "base.h"
#include "activator.h"
#include "login.h"
#include "simple.h"
#include "options.h"
#include "../assert.h"

DialogFactory gDialogFactory;

static ActivatorDialog sActivatorDialog;
static LoginDialog sLoginDialog;
static SimpleDialog sSimpleDialog;
static OptionsDialog sOptionsDialog;

DialogFactory::DialogFactory() {
	fMap["activator"] = &sActivatorDialog;
	fMap["login"] = &sLoginDialog;
	fMap["simple"] = &sSimpleDialog;
	fMap["options"] = &sOptionsDialog;
}

void DialogFactory::Make(Rocket::Core::Context *context, const std::string &file, std::function<void()> callback) {
	Rocket::Core::ElementDocument *document = context->LoadDocument(("dialogs/" + file).c_str());
	ASSERT(document != 0);
	Rocket::Core::String def = "";
	auto tag = document->GetTagName();
	// auto type = document->GetAttribute("type", def);
	string handler = document->GetAttribute("handler", def).CString();
	ASSERT (handler != "");
	auto it = fMap.find(handler);
	ASSERT(it != fMap.end());
	it->second->UseDocument(document, callback);
}
