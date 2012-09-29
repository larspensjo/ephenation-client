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

#include <string>

#include "messagedialog.h"

using std::string;

MessageDialog::MessageDialog(const string &title, const string &body, void (*callback)(void)) : fCallback(callback) {
	fTitle.Init(fTitleFont, title, 100, 0);
	fBody.Init(fBodyFont, body, 0, 100);
	fClose.Init(fMenuFont, "Close", 0, 350);
}

void MessageDialog::Draw(DrawTexture *drawTexture, float alpha) {
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
	fBody.Draw(black, alpha);

	color = black;
	if (fClose.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::CLOSE;
	}
	fClose.Draw(color, alpha);
}

void MessageDialog::Click(void) {
	switch(fActiveElement) {
	case ActiveElement::NONE:
		break;
	case ActiveElement::CLOSE:
		this->Escape();
		break;
	}
}

void MessageDialog::Escape(void) {
	if (fCallback != 0)
		(*fCallback)();
	dialog::Pop(); // This will call the destructor
}
