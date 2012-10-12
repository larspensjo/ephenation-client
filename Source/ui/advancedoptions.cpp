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

#include <GL/glew.h>
#include <GL/glfw.h>

#include "advancedoptions.h"
#include "lighting.h"
#include "../Options.h"

AdvancedOptions::AdvancedOptions() : fActiveElement(ActiveElement::NONE) {
	const int xalign = 40;
	int h = fBodyFont->vsfl.height();
	fTitle.Init(fTitleFont, "Advanced", 100, 0);
	fAnisotropicFiltering.Init(fBodyFont, "Anisotropic filtering", xalign, 100);
	fAnisotropicFilteringMark.Init(fBodyFont, "x", xalign-20, 100);

	fShadows.Init(fBodyFont, "Shadows: ", xalign, 100+h);
	fStaticShadowsMark.Init(fBodyFont, "static", xalign+100, 100+h);
	fSmoothShadowsMark.Init(fBodyFont, "smooth", xalign+160, 100+h);
	fDynamicShadowsMark.Init(fBodyFont, "dynamic", xalign+220, 100+h);

	fVSYNC.Init(fBodyFont, "Vertical synchronization", xalign, 100+h*2);
	fVSYNCMark.Init(fBodyFont, "x", xalign-20, 100+h*2);

	fPrevious.Init(fMenuFont, "Previous", 0, 350); fCancel.Init(fMenuFont, "Cancel", 0, 350);
	fQuit.Init(fMenuFont, "Save and quit", 250, 350);
	fLighting.Init(fMenuFont, "Lighting...", 250, 350);

	fNewStaticShadows = Options::fgOptions.fStaticShadows;
	fNewDynamicShadows = Options::fgOptions.fDynamicShadows;
	fNewAnisotropicFiltering = Options::fgOptions.fAnisotropicFiltering;
}

AdvancedOptions::~AdvancedOptions() {
}

void AdvancedOptions::Draw(DrawTexture *drawTexture, float alpha) {
	float x, y;
	this->GetMousePosition(x, y);
	glm::vec3 black(-1,-1,-1), white(0,0,0);

	// Draw the dialog background.
	dialog::Draw(drawTexture, alpha);

	fMenuFont->Enable();
	fMenuFont->UpdateProjection();
	glm::vec3 color;
	fActiveElement = ActiveElement::NONE;

	fTitle.Draw(black, alpha);
	fShadows.Draw(black, alpha);

	color = black;
	if (fAnisotropicFiltering.Inside(x, y) || fAnisotropicFilteringMark.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::ANISOTROPIC;
	}
	fAnisotropicFiltering.Draw(color, alpha);
	if (fNewAnisotropicFiltering)
		fAnisotropicFilteringMark.Draw(color, alpha);

	color = black;
	if (fNewStaticShadows == 0 && fNewDynamicShadows == 0) color = white;
	if (fStaticShadowsMark.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::STATICSHADOWS;
	}
	fStaticShadowsMark.Draw(color, alpha);

	color = black;
	if (fNewStaticShadows == 1 && fNewDynamicShadows == 0) color = white;
	if (fSmoothShadowsMark.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::SMOOTHSHADOWS;
	}
	fSmoothShadowsMark.Draw(color, alpha);

	color = black;
	if (fNewStaticShadows == 0 && fNewDynamicShadows == 1) color = white;
	if (fDynamicShadowsMark.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::DYNAMICSHADOWS;
	}
	fDynamicShadowsMark.Draw(color, alpha);

	color = black;
	if (fVSYNC.Inside(x, y) || fVSYNCMark.Inside(x,y)) {
		color = white;
		fActiveElement = ActiveElement::VSYNC;
	}
	fVSYNC.Draw(color, alpha);
	if (Options::fgOptions.fVSYNC)
		fVSYNCMark.Draw(color, alpha);

	if (this->RestartRequired()) {
		color = black;
		if (fCancel.Inside(x, y)) {
			color = white;
			fActiveElement = ActiveElement::CANCEL;
		}
		fCancel.Draw(color, alpha);

		color = black;
		if (fQuit.Inside(x, y)) {
			color = white;
			fActiveElement = ActiveElement::QUITANDAPPLY;
		}
		fQuit.Draw(color, alpha);
	} else {
		color = black;
		if (fPrevious.Inside(x, y)) {
			color = white;
			fActiveElement = ActiveElement::PREVIOUS;
		}
		fPrevious.Draw(color, alpha);

		color = black;
		if (fLighting.Inside(x, y)) {
			color = white;
			fActiveElement = ActiveElement::LIGHTING;
		}
		fLighting.Draw(color, alpha);
	}
}

void AdvancedOptions::Click(void) {
	switch(fActiveElement) {
	case ActiveElement::NONE:
		break;
	case ActiveElement::ANISOTROPIC:
		fNewAnisotropicFiltering = !fNewAnisotropicFiltering;
		break;
	case ActiveElement::STATICSHADOWS:
		fNewStaticShadows = 0;
		fNewDynamicShadows = 0;
		break;
	case ActiveElement::SMOOTHSHADOWS:
		fNewStaticShadows = 1;
		fNewDynamicShadows = 0;
		break;
	case ActiveElement::DYNAMICSHADOWS:
		fNewStaticShadows = 0;
		fNewDynamicShadows = 1;
		break;
	case ActiveElement::VSYNC:
		Options::fgOptions.fVSYNC = !Options::fgOptions.fVSYNC;
		glfwSwapInterval(Options::fgOptions.fVSYNC); // Immediately change behavior
		break;
	case ActiveElement::PREVIOUS:
		dialog::Pop();
		break;
	case ActiveElement::LIGHTING:
		this->Push(new Lighting);
		break;
	case ActiveElement::CANCEL:
		dialog::Pop();
		break;
	case ActiveElement::QUITANDAPPLY:
		Options::fgOptions.fStaticShadows = fNewStaticShadows;
		Options::fgOptions.fDynamicShadows = fNewDynamicShadows;
		Options::fgOptions.fAnisotropicFiltering = fNewAnisotropicFiltering;
		glfwCloseWindow(); // Force shutdown and saving of all options
		break;
	}
}

bool AdvancedOptions::RestartRequired(void) {
	return
	    fNewStaticShadows != Options::fgOptions.fStaticShadows ||
	    fNewDynamicShadows != Options::fgOptions.fDynamicShadows ||
	    fNewAnisotropicFiltering != Options::fgOptions.fAnisotropicFiltering;
}
