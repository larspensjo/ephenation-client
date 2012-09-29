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

#include "optionsdialog2.h"
#include "maindialog.h"
#include "advancedoptions.h"
#include "../Options.h"
#include "../primitives.h"
#include "../SoundControl.h"

OptionsDialog2::OptionsDialog2() : fActiveElement(ActiveElement::NONE) {
	const int xalign = 120;
	int h = fBodyFont->vsfl.height();
	fTitle.Init(fTitleFont, "Options", 100, 0);
	fPing.Init(fBodyFont, "Ping (can slow down FPS)", xalign, 100); fPingMark.Init(fBodyFont, "x", xalign-20, 100);
	fMusic.Init(fBodyFont, "Music", xalign, 100+h); fMusicMark.Init(fBodyFont, "x", xalign-20, 100+h);

	fPerformance.Init(fBodyFont, "System performance (requires restart)", xalign, 100+2*h);
	fPerf1.Init(fBodyFont, "LI", xalign-120, 100+2*h);
	fPerf2.Init(fBodyFont, "L2", xalign-90, 100+2*h);
	fPerf3.Init(fBodyFont, "L3", xalign-60, 100+2*h);
	fPerf4.Init(fBodyFont, "L4", xalign-30, 100+2*h);

	fFullScreen.Init(fBodyFont, "Full screen", xalign, 100+3*h); fFullScreenMark.Init(fBodyFont, "x", xalign-20, 100+3*h);

	fPrevious.Init(fMenuFont, "Previous", 0, 350); fCancel.Init(fMenuFont, "Cancel", 0, 350);
	fAdvanced.Init(fMenuFont, "Advanced graphics...", 250, 350); fQuit.Init(fMenuFont, "Save and quit", 250, 350);

	fNewPerfLevel = Options::fgOptions.fPerformance;
	fNewFullScreen = Options::fgOptions.fFullScreen;
}

OptionsDialog2::~OptionsDialog2() {
}

void OptionsDialog2::Draw(DrawTexture *drawTexture, float alpha) {
	float x, y;
	this->GetMousePosition(x, y);
	glm::vec3 black(-1,-1,-1), white(0,0,0), red(0, -1, -1);

	// Draw the dialog background.
	dialog::Draw(drawTexture, alpha);

	fMenuFont->Enable();
	fMenuFont->UpdateProjection();
	glm::vec3 color;
	fActiveElement = ActiveElement::NONE;

	fTitle.Draw(black, alpha);

	color = black;
	if (fPing.Inside(x, y) || fPingMark.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::PING;
	}
	fPing.Draw(color, alpha);
	if (gShowPing)
		fPingMark.Draw(color, alpha);

	color = black;
	if (fMusic.Inside(x, y) || fMusicMark.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::MUSIC;
	}
	fMusic.Draw(color, alpha);
	if (Options::fgOptions.fMusicOn)
		fMusicMark.Draw(color, alpha);

	fPerformance.Draw(black, alpha);

	color = black;
	if (fFullScreen.Inside(x, y) || fFullScreenMark.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::FULLSCREEN;
	}
	fFullScreen.Draw(color, alpha);
	if (fNewFullScreen)
		fFullScreenMark.Draw(color, alpha);

	fPerformance.Draw(black, alpha);

	glm::vec3 c1 = black, c2 = black, c3 = black, c4 = black;
	switch (fNewPerfLevel) {
	case 1: c1 = red; break;
	case 2: c2 = red; break;
	case 3: c3 = red; break;
	case 4: c4 = red; break;
	}
	if (fPerf1.Inside(x,y)) { c1 = white; fActiveElement = ActiveElement::P1; }
	if (fPerf2.Inside(x,y)) { c2 = white; fActiveElement = ActiveElement::P2; }
	if (fPerf3.Inside(x,y)) { c3 = white; fActiveElement = ActiveElement::P3; }
	if (fPerf4.Inside(x,y)) { c4 = white; fActiveElement = ActiveElement::P4; }
	fPerf1.Draw(c1, alpha);
	fPerf2.Draw(c2, alpha);
	fPerf3.Draw(c3, alpha);
	fPerf4.Draw(c4, alpha);

	DialogText *close = &fPrevious;
	if (this->RestartRequired())
		close = &fCancel;

	color = black;
	if (close->Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::PREVIOUS;
	}
	close->Draw(color, alpha);

	if (this->RestartRequired()) {
		color = black;
		if (fQuit.Inside(x, y)) {
			color = white;
			fActiveElement = ActiveElement::SAVEANDQUIT;
		}
		fQuit.Draw(color, alpha);
	} else {
		color = black;
		if (fAdvanced.Inside(x, y)) {
			color = white;
			fActiveElement = ActiveElement::ADVANCED;
		}
		fAdvanced.Draw(color, alpha);
	}
}

void OptionsDialog2::Click(void) {
	switch(fActiveElement) {
	case ActiveElement::NONE:
		break;
	case ActiveElement::MUSIC:
		Options::fgOptions.fMusicOn = !Options::fgOptions.fMusicOn;
		gSoundControl.SwitchMusicStatus();
		break;
	case ActiveElement::PREVIOUS:
		dialog::Pop();
		break;
	case ActiveElement::ADVANCED:
		this->Push(new AdvancedOptions);
		break;
	case ActiveElement::SAVEANDQUIT:
		Options::fgOptions.fNewPerformance = fNewPerfLevel;
		Options::fgOptions.fFullScreen = fNewFullScreen;
		glfwCloseWindow();
		break;
	case ActiveElement::PING:
		gShowPing = !gShowPing;
		break;
	case ActiveElement::FULLSCREEN:
		fNewFullScreen = !fNewFullScreen;
		break;
	case ActiveElement::P1:
		fNewPerfLevel = 1; break;
	case ActiveElement::P2:
		fNewPerfLevel = 2; break;
	case ActiveElement::P3:
		fNewPerfLevel = 3; break;
	case ActiveElement::P4:
		fNewPerfLevel = 4; break;
	}
}

bool OptionsDialog2::RestartRequired(void) {
	return fNewPerfLevel != Options::fgOptions.fPerformance || fNewFullScreen != Options::fgOptions.fFullScreen;
}
