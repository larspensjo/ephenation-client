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

//
// The main dialog is the oneyou get after logging in.
//

#include "dialog.h"

class DrawTexture;

class MainDialog : public dialog {
public:
	MainDialog();

	// Draw this window.
	virtual void Draw(DrawTexture *drawTexture, float alpha);

	// The player clicked on something. Return false if the windows is closed.
	virtual void Click(void);
private:
	DialogText fTitle, fHelp;
	DialogText fPlay, fOptions, fQuit;
	enum class ActiveElement { NONE, PLAY, OPTIONS, HELP, QUIT };
	ActiveElement fActiveElement;
};
