// Copyright 2013 The Ephenation Authors
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

#include <Rocket/Controls.h>

#include "simple.h"
#include "../gamedialog.h"

void SimpleDialog::UseDocument(Rocket::Core::ElementDocument *doc, std::function<void()> callback) {
	this->Push(doc, callback); // Push this document on top of stack
	this->Treewalk([this](Rocket::Core::Element *e){ this->UpdateInput(e);} ); // Fill default parameters in the document
	this->Treewalk([this](Rocket::Core::Element *e) {this->DetectDefaultButton(e); }); // This can be done by the local callback
	this->AddEventListener("click", this);
	this->Show();
}

void SimpleDialog::UpdateInput(Rocket::Core::Element *e) {
	Rocket::Core::String def = "";
	auto tag = e->GetTagName();
	auto type = e->GetAttribute("type", def);
	string name = e->GetAttribute("name", def).CString();
	auto value = e->GetAttribute("value", def);
	if (tag == "div") {
		if (name == "header") {
			if (sgPopupTitle == "")
				sgPopupTitle = "Ephenation";
			e->SetInnerRML(sgPopupTitle.c_str());
		}
		if (name == "content") {
			e->SetInnerRML(sgPopup.c_str());
			sgPopup = "";
		}
	}
}
