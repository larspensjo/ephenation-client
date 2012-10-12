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
#include "dialoginput.h"

class DrawTexture;

class Lighting : public dialog {
public:
	Lighting();

	// Draw this window.
	virtual void Draw(DrawTexture *drawTexture, float alpha);

	// The player clicked on something. Return false if the windows is closed.
	virtual void Click(void);

private:
	bool ModifiedValues(void) const;

	DialogText fTitle, fExposure, fWhitepoint, fAmbient;
	DialogText fPrevious, fCancel;
	enum class ActiveElement { NONE, PREVIOUS, EXPOSURE, WHITEPOINT, AMBIENT, CANCEL };
	ActiveElement fActiveElement;

	int fPrevAmbient;
	float fPrevExp, fPrevWhite;
};
