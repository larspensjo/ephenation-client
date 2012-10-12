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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "msgwindow.h"
#include "render.h"
#include "DrawText.h"
#include "modes.h"
#include "HealthBar.h"
#include "Options.h"
#include "primitives.h"
#include "ScrollingMessages.h"

#define ILL_SENT 0xFFFFFFFF

MsgWindow::MsgWindow(): fRocketElement(0), fActivateDropMessage(false) {
}

void MsgWindow::Init(Rocket::Core::Element *rocketElement) {
	fRocketElement = rocketElement;
}

void MsgWindow::Add(const char *fmt, ...) {
	va_list vl;
	va_start(vl, fmt);
	char buff[1000];
#ifdef WIN32
	_vsnprintf(buff, sizeof buff, fmt, vl);
#else
	vsnprintf(buff, sizeof buff, fmt, vl);
#endif

	fCompleteMessage = fCompleteMessage + "\n" + buff;

	if (fRocketElement) {
		fRocketElement->SetInnerRML(fCompleteMessage.c_str());
	}

	if (gMode.Get() != GameMode::GAME && gMode.Get() != GameMode::CONSTRUCT && gMode.Get() != GameMode::TELEPORT) {
		// Fail safe test if the graphics hasn't been initialized yet.
		printf("Message: %s\n", buff);
		return;
	}
	if (fActivateDropMessage)
		gScrollingMessages.AddMessage(fDropX, fDropY, buff);
}

void MsgWindow::SetAlternatePosition(int x, int y, bool enable) {
	fActivateDropMessage = enable;
	fDropX = x;
	fDropY = y;
}

MsgWindow gMsgWindow;
