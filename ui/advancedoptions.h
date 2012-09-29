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

// This class manages the in-game dialog for options. Eventually, it will completely
// replace OptionsDialog.

#include "dialog.h"

class AdvancedOptions : public dialog {
public:
	AdvancedOptions();
	virtual ~AdvancedOptions();

	// Draw the dialog background.
	virtual void Draw(DrawTexture *drawTexture, float alpha);

	// The player clicked on something.
	virtual void Click(void);
private:
	DialogText fAnisotropicFiltering, fAnisotropicFilteringMark, fPrevious, fTitle;
	DialogText fCancel, fQuit; // These conditional choices, not always available.
	DialogText fShadows, fStaticShadowsMark, fSmoothShadowsMark, fDynamicShadowsMark;
	DialogText fVSYNC, fVSYNCMark;
	DialogText fLighting;
	enum class ActiveElement { NONE, PREVIOUS, CANCEL, QUITANDAPPLY, ANISOTROPIC, SMOOTHSHADOWS, STATICSHADOWS, DYNAMICSHADOWS, VSYNC, LIGHTING };
	ActiveElement fActiveElement;

	bool RestartRequired(void);
	int fNewStaticShadows, fNewDynamicShadows, fNewAnisotropicFiltering;
};
