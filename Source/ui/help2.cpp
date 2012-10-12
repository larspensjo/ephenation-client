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

#include "help2.h"

const std::string h =
    "Adventure, kill monsters, go up in level. Monsters get progressively harder\n"
    "from the starting point.\n"
    "Find and use better equipments. Find a free area, allocate a territory of\n"
    "your own and redesign it as you want.\n"
    "Make an exciting adventure for others, or a place to invite friends to.\n"
    "Look at the high scores of popular areas, to find places to go adventure.\n"
    "To kill a monster, use TAB to select it and initiate attack with 'I'.";


HelpDialog2::HelpDialog2() {
	// int h = fBodyFont->vsfl.height();
	fTitle.Init(fTitleFont, "Help", 100, 0);
	fBody.Init(fBodyFont, h, 0, 100);

	fPrevious.Init(fMenuFont, "Previous", 0, 350);
}

void HelpDialog2::Draw(DrawTexture *drawTexture, float alpha) {
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
	fBody.Draw(black, alpha);

	color = black;
	if (fPrevious.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::PREVIOUS;
	}
	fPrevious.Draw(color, alpha);
}


void HelpDialog2::Click(void) {
	switch(fActiveElement) {
	case ActiveElement::NONE:
		break;
	case ActiveElement::PREVIOUS:
		dialog::Pop();
		break;
	}
}
