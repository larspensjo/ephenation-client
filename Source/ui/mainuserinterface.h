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
* The player health bar and other bars
* The player level and other stats
* The input field for /-commands
* FPS statistics

TODO: There are also other Rocket documents associated with the Context that will be drawn implicitly. These should
	be managed in a separate context.
*/

#pragma once

#include <string>

// Forward declaration
namespace Rocket {
	namespace Core {
		class Context;
		class Element;
		class ElementDocument;
	}
};

using std::string;

class MainUserInterface {
public:
	MainUserInterface();
	~MainUserInterface();
	void Resize(int w, int h);
	void Init(void);

	// Draw the complete context. The main UI, managed from this class, can be explicitely enabled with 'showGUI'.
	void Draw(bool showGUI);
	Rocket::Core::Context *GetRocketContext(void); // TODO: Ugly, I know.

	Rocket::Core::Element *GetElement(string);
private:
	Rocket::Core::Context *fRocketContext;
	Rocket::Core::ElementDocument *fDocument;
	bool fShowGUI;
};
