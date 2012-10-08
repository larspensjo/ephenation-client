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
#include <Rocket/Debugger.h>

#include "mainuserinterface.h"
#include "../primitives.h"
#include "../timemeasure.h"

MainUserInterface::MainUserInterface() : fRocketContext(0) {
}

void MainUserInterface::Init() {
	// Create the main Rocket context and set it on the shell's input layer.
	fRocketContext = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(gViewport[2], gViewport[3]));
	if (fRocketContext == NULL)
	{
		printf("Rocket::Core::CreateContext failed\n");
		exit(1);
	}

	Rocket::Debugger::Initialise(fRocketContext);

	// Load and show the UI.
	Rocket::Core::ElementDocument* document = fRocketContext->LoadDocument("dialogs/userinterface.rml");
	if (document != NULL)
	{
#if 1
		Rocket::Core::Element *e = document->GetElementById("chat");
		Rocket::Core::String s = e->GetInnerRML();
		e->SetInnerRML("Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat.");
#endif
		document->Show();
		document->RemoveReference();
	}
}

void MainUserInterface::Resize(int w, int h) {
	if (fRocketContext)
		fRocketContext->SetDimensions(Rocket::Core::Vector2i(w, h));
}

void MainUserInterface::Draw(void) {
	static TimeMeasure tmr("MainUI");
	tmr.Start();
	fRocketContext->Update();
	fRocketContext->Render();
	tmr.Stop();
}

Rocket::Core::Context *MainUserInterface::GetRocketContext(void) {
	return fRocketContext;
}
