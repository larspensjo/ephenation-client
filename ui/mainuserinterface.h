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

/*
Manage the main user interface when gaming. That includes:
* The chat window
* The player health and other bars
* The player level and other stats
* The input field for /-commands
*/

#pragma once

#include <string>
#include <Rocket/Core.h>

using std::string;

class MainUserInterface {
public:
	MainUserInterface();
	void Resize(int w, int h);
	void Init(void);
	void Draw(void);
	Rocket::Core::Context *GetRocketContext(void);

	Rocket::Core::Element *GetElement(string);
private:
	Rocket::Core::Context *fRocketContext;
	Rocket::Core::ElementDocument *fDocument;
};
