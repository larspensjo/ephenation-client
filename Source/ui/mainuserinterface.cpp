// Copyright 2012-2013 The Ephenation Authors
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
#include "../HudTransformation.h"

using namespace View;

MainUserInterface::~MainUserInterface() {
	if (fDocument)
		fDocument->RemoveReference();
	if (fRocketContext)
		fRocketContext->RemoveReference();
}

void MainUserInterface::Init(bool stereoView) {
	fStereoView = stereoView;
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
	if (fStereoView)
		fDocument = fRocketContext->LoadDocument("dialogs/userinterface-ovr.rml");
	else
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
	glm::mat4 model(1);
	glm::mat4 proj(1);
	if (fStereoView) {
		proj = gProjectionMatrix;
		glm::mat4 center = glm::translate(glm::mat4(1), glm::vec3(-0.5, -0.5f, 0.0f));
		glm::mat4 scaleToMeter = glm::scale(glm::mat4(1), glm::vec3(0.4f, 0.4f, 0.4f));
		glm::mat4 pointToNorth = glm::rotate(glm::mat4(1), Model::gPlayer.fAngleHor, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 makeHorizontal = glm::rotate(glm::mat4(1), -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 transl = glm::translate(glm::mat4(1), glm::vec3(-2.0f, -3.5f, -4.0f));
		model = View::gHudTransformation.GetViewTransform() * (transl * makeHorizontal * scaleToMeter * pointToNorth * center);
	} else {
		// Move compass to lower left corner.
		model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.85f, -0.80f, 0.0f));

		// Scale it down, and remove dependency of screen width/height ratio
		float scale = 0.2f;
		float screenRatio = gViewport[2] / gViewport[3];
		model = glm::scale(model, glm::vec3(scale/screenRatio, scale, scale));

		// Rotate according to vertical viewing direction
		model = glm::rotate(model, Model::gPlayer.fAngleVert, glm::vec3(1.0f, 0.0f, 0.0f));

		// Rotate according to horizontal viewing direction
		model = glm::rotate(model, Model::gPlayer.fAngleHor, glm::vec3(0.0f, 0.0f, 1.0f));

		// Move the center of the compass, so rotation is done around the center, not the lower left corner of the bitmap.
		model = glm::translate(model, glm::vec3(-0.5f, -0.5f, 0.0f));
	}

	glBindTexture(GL_TEXTURE_2D, GameTexture::CompassRose);
	auto fDrawTexture = DrawTexture::Make();
	fDrawTexture->Draw(proj, model);

	auto fHealthBar = HealthBar::Make(); // Singleton
	Model::gMonsters.RenderMinimap(proj * model, fHealthBar);
	Model::gOtherPlayers.RenderMinimap(proj * model, fHealthBar);
}

void MainUserInterface::DrawPlayerStats(void) const {
	auto fHealthBar = HealthBar::Make(); // Singleton
	glm::mat4 model(1);
	glm::mat4 proj(1);

	// The bars have x and y going from 0 to 1.
	glm::mat4 center = glm::translate(glm::mat4(1), glm::vec3(-0.5, 0.5f, 0.0f)); // TODO: y should be -0.5f???

	if (fStereoView) {
		// We want to draw the GUI as if it is placed out in the world.
		proj = gProjectionMatrix;
		// Scale to OVR screen pixels
		glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(300.0f, 5.0f, 1.0f));
		// Make the bar standing up
		glm::mat4 rot = glm::rotate(glm::mat4(1), 90.0f, glm::vec3(0.0f, 0.0f, -1.0f));
		// Move the bar to the left side of the window
		glm::mat4 leftSide = glm::translate(glm::mat4(1), glm::vec3(-200.0f, 0.0f, 0.0f));
		model = View::gHudTransformation.GetGUITransform() * (leftSide * rot * scale * center);
	} else {
		// The coordinates and sizes used here are measured from the graphical UI. No view or projection matrix is used,
		// which means the screen is from -1.0 to +1.0 in both x and y.
		model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.69157f, -0.81982f, 0.0f));
		model = glm::scale(model, glm::vec3(0.43802f, 0.03804f, 1.0f));
	}
	float dmg = Model::gPlayer.fPreviousHp - Model::gPlayer.fHp;
	fHealthBar->DrawHealth(proj, model, Model::gPlayer.fHp, dmg, false);

	if (fStereoView) {
		// Scale to OVR screen pixels
		glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(300.0f, 5.0f, 1.0f));
		// Make the bar standing up
		glm::mat4 rot = glm::rotate(glm::mat4(1), 90.0f, glm::vec3(0.0f, 0.0f, -1.0f));
		// Move the bar to the left side of the window
		glm::mat4 leftSide = glm::translate(glm::mat4(1), glm::vec3(200.0f, 0.0f, 0.0f));
		model = View::gHudTransformation.GetGUITransform() * (leftSide * rot * scale * center);
	} else {
		model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.69194f, -0.85735f, 0.0f));
		model = glm::scale(model, glm::vec3(0.38361f, 0.01788f, 1.0f));
	}
	fHealthBar->DrawMana(model, Model::gPlayer.fMana);

	if (fStereoView) {
		// Scale to OVR screen pixels
		glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(250.0f, 5.0f, 1.0f));
		// Move the bar to the left side of the window
		glm::mat4 bottom = glm::translate(glm::mat4(1), glm::vec3(0.0f, 200.0f, 0.0f));
		model = View::gHudTransformation.GetGUITransform() * (bottom * scale * center);
	} else {
		model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(2.0f, 0.02856f, 1.0f));
	}
	fHealthBar->DrawExp(model, Model::gPlayer.fExp);
}
