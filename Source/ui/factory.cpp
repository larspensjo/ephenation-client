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

#include <Rocket/Core.h>

#include "factory.h"
#include "base.h"
#include "Error.h"

DialogFactory gDialogFactory;

BaseDialog *DialogFactory::Make(Rocket::Core::Context *context, const std::string &file) {
	Rocket::Core::ElementDocument *document = context->LoadDocument(("dialogs/" + file).c_str());
	Rocket::Core::String def = "";
	auto tag = document->GetTagName();
	auto type = document->GetAttribute("type", def);
	string handler = document->GetAttribute("handler", def).CString();
	if (handler == "") {
		ErrorDialog("DialogFactory::Make: No handler defined for body of %s", file.c_str());
	}
	BaseDialog *base = fMap[handler];
	if (base == 0) {
		ErrorDialog("DialogFactory::Make: No handler registered for %s", handler.c_str());
	}
	base->UseDocument(document);
	return base;
}

void DialogFactory::Register(const std::string &name, BaseDialog *base) {
	fMap[name] = base;
}
