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

#include "maindialog.h"
#include "optionsdialog2.h"
#include "help2.h"
#include "../DrawTexture.h"
#include "../DrawText.h"

MainDialog::MainDialog() : fActiveElement(ActiveElement::NONE) {
	int h = fMenuFont->vsfl.height();
	fTitle.Init(fTitleFont, "Ephenation", 100, 0);
	fOptions.Init(fMenuFont, "Options...", 100, 100);
	fHelp.Init(fMenuFont, "Help...", 100, 100+h);
	fQuit.Init(fMenuFont, "Quit", 400, 350);
	fPlay.Init(fMenuFont, "Play", 0, 350);
}

void MainDialog::Draw(DrawTexture *drawTexture, float alpha) {
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
	if (fPlay.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::PLAY;
	}
	fPlay.Draw(color, alpha);

	color = black;
	if (fOptions.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::OPTIONS;
	}
	fOptions.Draw(color, alpha);

	color = black;
	if (fHelp.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::HELP;
	}
	fHelp.Draw(color, alpha);

	color = black;
	if (fQuit.Inside(x, y)) {
		color = white;
		fActiveElement = ActiveElement::QUIT;
	}
	fQuit.Draw(color, alpha);
}

void MainDialog::Click(void) {
	switch(fActiveElement) {
	case ActiveElement::NONE:
		break;
	case ActiveElement::OPTIONS:
		dialog::Push(new OptionsDialog2);
		break;
	case ActiveElement::HELP:
		this->Push(new HelpDialog2);
		break;
	case ActiveElement::PLAY:
		dialog::Pop();
		break;
	case ActiveElement::QUIT:
		glfwCloseWindow();
		break;
	}
}
