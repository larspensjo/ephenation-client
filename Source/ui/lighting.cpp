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

#include "lighting.h"
#include "../DrawTexture.h"
#include "../DrawText.h"
#include "../gamedialog.h"
#include "../Options.h"

Lighting::Lighting() : fActiveElement(ActiveElement::NONE) {
	int h = fMenuFont->vsfl.height();
	fTitle.Init(fTitleFont, "Lighting", 100, 0);
	fExposure.Init(fMenuFont, "Exposure...", 80, 100);
	fWhitepoint.Init(fMenuFont, "White point...", 80, 100+h);
	fAmbient.Init(fMenuFont, "Ambient light...", 80, 100+h*2);
	fPrevious.Init(fMenuFont, "Previous", 0, 350);
	fCancel.Init(fMenuFont, "Cancel", 400, 350);

	fPrevAmbient = Options::fgOptions.fAmbientLight;
	fPrevExp = Options::fgOptions.fExposure;
	fPrevWhite = Options::fgOptions.fWhitePoint;
}

bool Lighting::ModifiedValues() const {
	return fPrevAmbient != Options::fgOptions.fAmbientLight ||
	       fPrevExp != Options::fgOptions.fExposure ||
	       fPrevWhite != Options::fgOptions.fWhitePoint;
}

void Lighting::Draw(DrawTexture *drawTexture, float alpha) {
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

	color = black;
	if (fExposure.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::EXPOSURE;
	}
	fExposure.Draw(color, alpha);

	color = black;
	if (fWhitepoint.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::WHITEPOINT;
	}
	fWhitepoint.Draw(color, alpha);

	color = black;
	if (fAmbient.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::AMBIENT;
	}
	fAmbient.Draw(color, alpha);

	color = black;
	if (fPrevious.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::PREVIOUS;
	}
	fPrevious.Draw(color, alpha);

	color = black;
	if (fCancel.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::CANCEL;
	}
	fCancel.Draw(color, alpha);
}

void Lighting::Click(void) {
	switch(fActiveElement) {
	case ActiveElement::NONE:
		break;
	case ActiveElement::EXPOSURE:
		gGameDialog.CalibrateMode(gameDialog::CALIB_EXPOSURE);
		break;
	case ActiveElement::WHITEPOINT:
		gGameDialog.CalibrateMode(gameDialog::CALIB_WHITE_POINT);
		break;
	case ActiveElement::AMBIENT:
		gGameDialog.CalibrateMode(gameDialog::CALIB_AMBIENT);
		break;
	case ActiveElement::PREVIOUS:
		dialog::Pop();
		break;
	case ActiveElement::CANCEL:
		// Restore the saved values
		Options::fgOptions.fAmbientLight = fPrevAmbient;
		Options::fgOptions.fExposure = fPrevExp;
		Options::fgOptions.fWhitePoint = fPrevWhite;
		dialog::Pop();
		break;
	}
}
