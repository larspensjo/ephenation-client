// Copyright 2013 The Ephenation Authors
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

#include <glm/gtc/matrix_transform.hpp>

#include "HudTransformation.h"
#include "primitives.h"
#include "OculusRift.h"

using namespace View;

HudTransformation View::gHudTransformation;

glm::mat4 HudTransformation::GetTransform() const {		// We want to draw the GUI as if it is placed out in the world.
	glm::vec4 p = gProjectionMatrix * glm::vec4(0.0f, 1.0f, -fGuiDistance, 1.0f);
	p /= p.w;
	float fact = 2.0f/p.y/gViewport[3];
	glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(fact, -fact, 1.0f));
	float yawPitchRoll[3] = { 0.0f, 0.0f, 0.0f };
	Controller::OculusRift::sfOvr.GetYawPitchRoll(yawPitchRoll);
	glm::mat4 view(1);
	view = glm::rotate(view, yawPitchRoll[2], glm::vec3(0.0f, 0.0f, -1.0f));
	view = glm::rotate(view, -yawPitchRoll[1], glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::rotate(view, -yawPitchRoll[0], glm::vec3(0.0f, 1.0f, 0.0f));
	view = glm::translate(view, glm::vec3(Controller::OculusRift::sfOvr.GetHorViewAdjustment(), 0.0f, 0.0f));
	glm::mat4 translate = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -fGuiDistance));
	return view * scale * translate;
}
