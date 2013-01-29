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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "mainuserinterface.h"
#include "../primitives.h"
#include "../timemeasure.h"
#include "Error.h"
#include "../player.h"
#include "../monsters.h"
#include "../textures.h"
#include "../DrawTexture.h"
#include "../HealthBar.h"
#include "../otherplayers.h"

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
	if (fShowGUI) {
		DrawCompassRose();
		DrawPlayerStats();
	}
	tmr.Stop();
}

Rocket::Core::Context *MainUserInterface::GetRocketContext(void) {
	return fRocketContext;
}

Rocket::Core::Element *MainUserInterface::GetElement(string elem) {
	return fDocument->GetElementById(elem.c_str());
}


void MainUserInterface::DrawCompassRose(void) const {
	static const glm::mat4 id(1.0f); // No need to create a new one every time.

	// Move compass to lower left corner.
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.85f, -0.80f, 0.0f));

	// Scale it down, and remove dependency of screen width/height ratio
	float scale = 0.2f;
	float screenRatio = gViewport[2] / gViewport[3];
	model = glm::scale(model, glm::vec3(scale/screenRatio, scale, scale));

	// Rotate according to vertical viewing direction
	model = glm::rotate(model, Model::gPlayer.fAngleVert, glm::vec3(1.0f, 0.0f, 0.0f));

	// Rotate according to horisontal viewing direction
	model = glm::rotate(model, Model::gPlayer.fAngleHor, glm::vec3(0.0f, 0.0f, 1.0f));

	// Move the center of the compass, so rotation is done around the center, not the lower left corner of the bitmap.
	model = glm::translate(model, glm::vec3(-0.5f, -0.5f, 0.0f));

	glBindTexture(GL_TEXTURE_2D, GameTexture::CompassRose);
	auto fDrawTexture = DrawTexture::Make();
	fDrawTexture->Draw(id, model);
	// glBindTexture(GL_TEXTURE_2D, 0);  // Deprecated

	auto fHealthBar = HealthBar::Make(); // Singleton
	Model::gMonsters.RenderMinimap(model, fHealthBar);
	Model::gOtherPlayers.RenderMinimap(model, fHealthBar);
}


void MainUserInterface::DrawPlayerStats(void) const {
	auto fHealthBar = HealthBar::Make(); // Singleton
	// The coordinates and sizes used here are measured from the graphical UI. No view or projection matrix is used,
	// which means the screen is from -1.0 to +1.0 in both x and y.
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.69157f, -0.81982f, 0.0f));
	model = glm::scale(model, glm::vec3(0.43802f, 0.03804f, 1.0f));
	float dmg = Model::gPlayer.fPreviousHp - Model::gPlayer.fHp;
	static const glm::mat4 id(1.0f); // No need to create a new one every time.
	fHealthBar->DrawHealth(id, model, Model::gPlayer.fHp, dmg, false);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.69194f, -0.85735f, 0.0f));
	model = glm::scale(model, glm::vec3(0.38361f, 0.01788f, 1.0f));
	fHealthBar->DrawMana(model, Model::gPlayer.fMana);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(2.0f, 0.02856f, 1.0f));
	fHealthBar->DrawExp(model, Model::gPlayer.fExp);
}
