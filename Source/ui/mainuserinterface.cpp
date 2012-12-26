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
#include <Rocket/Core.h>

#include "mainuserinterface.h"
#include "../primitives.h"
#include "../timemeasure.h"
#include "Error.h"

MainUserInterface::MainUserInterface() : fRocketContext(0), fDocument(0), fShowGUI(false) {
}

MainUserInterface::~MainUserInterface() {
	if (fDocument)
		fDocument->RemoveReference();
	if (fRocketContext)
		fRocketContext->RemoveReference();
}

void MainUserInterface::Init() {
	// Create the main Rocket context and set it on the shell's input layer.
	fRocketContext = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(gViewport[2], gViewport[3]));
	if (fRocketContext == NULL)
	{
		printf("Rocket::Core::CreateContext failed\n");
		exit(1);
	}
	// The reference count is initialized to 1.

	if (gDebugOpenGL)
		Rocket::Debugger::Initialise(fRocketContext);

	// Load and show the UI.
	fDocument = fRocketContext->LoadDocument("dialogs/userinterface.rml");
	// LoadDocument will add one to reference already.
	if (fDocument == 0)
		ErrorDialog("MainUserInterface::Init: Failed to load user interface");
	if (fShowGUI)
		fDocument->Show();
}

void MainUserInterface::Resize(int w, int h) {
	if (fRocketContext)
		fRocketContext->SetDimensions(Rocket::Core::Vector2i(w, h));
}

void MainUserInterface::Draw(bool showGUI) {
	if (fShowGUI != showGUI) {
		// Remember the previous state, to avoid repeated calls
		if (showGUI)
			fDocument->Show();
		else
			fDocument->Hide();
		fShowGUI = showGUI;
	}
	static TimeMeasure tmr("MainUI");
	tmr.Start();
	fRocketContext->Update();
	fRocketContext->Render();
	tmr.Stop();
}

Rocket::Core::Context *MainUserInterface::GetRocketContext(void) {
	return fRocketContext;
}

Rocket::Core::Element *MainUserInterface::GetElement(string elem) {
	return fDocument->GetElementById(elem.c_str());
}
