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

#include <string>
#include <Rocket/Controls.h>
#include <sstream>

#include "login.h"
#include "../Options.h"
#include "../connection.h"
#include "../gamedialog.h"
#include "../parse.h"

#define NELEM(x) (sizeof(x) / sizeof (x[0]))

void LoginDialog::UseDocument(Rocket::Core::ElementDocument *doc, std::function<void()> callback) {
	this->Push(doc, callback); // Allways push the previous document first
	fFormResultValues.clear(); // Restart with an empty list
	this->Treewalk([this](Rocket::Core::Element *e){ this->UpdateInput(e);} ); // Fill default parameters in the document
	this->Treewalk([this](Rocket::Core::Element *e) {this->DetectDefaultButton(e); }); // This can be done by the local callback
	this->AddEventListener("click", this);
	this->AddEventListener("submit", this);
	this->Show();
}

void LoginDialog::UpdateInput(Rocket::Core::Element *e) {
	Rocket::Core::String def = "";
	auto tag = e->GetTagName();
	auto type = e->GetAttribute("type", def);
	string name = e->GetAttribute("name", def).CString();
	auto value = e->GetAttribute("value", def);
	if (tag == "input") {
		if (name != "")
			fFormResultValues[name] = "";

		if (name == "Login.licensekey") {
			e->SetAttribute("value", Options::sfSave.fLicenseKey.c_str());
		}
		if (name == "Login.email") {
			e->SetAttribute("value", Options::sfSave.fEmail.c_str());
		}
	} else if (tag == "p") {
		// There are some empty special 'p' tags, to be filled
		if (name == "client.availableversion") {
			std::stringstream ss;
			if (gClientAvailMajor != CLIENT_MAJOR_VERSION || gClientAvailMinor != CLIENT_MINOR_VERSION)
				ss << "<p style=\"color:red;\">New client version available: " << gClientAvailMajor << "." << gClientAvailMinor << "</p>";
			else
				ss << "Client version: " << gClientAvailMajor << "." << gClientAvailMinor;
			if (gParseMessageAtLogin != "")
				ss << "<br /><p style=\"color:red;\">" << gParseMessageAtLogin << "</p>";
			e->SetInnerRML(ss.str().c_str());
		}
	}
}

void LoginDialog::FormEvent(Rocket::Core::Event& event, const string &action) {
	BaseDialog::FormEvent(event, action); // Needed to perform some common preparations

	if (action == "login") {
		Options::sfSave.fLicenseKey = fFormResultValues["Login.licensekey"];
		Options::sfSave.fEmail = fFormResultValues["Login.email"];
		PerformLoginProcedure(Options::sfSave.fEmail, Options::sfSave.fLicenseKey, fFormResultValues["Login.password"], false);
		// printf("Login %s, %s, %s\n", fFormResultValues["Login.licensekey"].c_str(), fFormResultValues["Login.email"].c_str(), fFormResultValues["Login.password"].c_str());

		if (!this->Pop())
			Controller::gGameDialog.ClearInputRedirect(); // Normal case
	}
}
