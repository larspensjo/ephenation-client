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

#pragma once
//
// This is a class that will manage a window with scrolling messages on the screen.
// It is also called the chat window, as defined by the Rocket main context.
//

#include <string>
#include <Rocket/Core.h>

namespace View {

class MsgWindow {
public:
	MsgWindow(); // Draw the window at x,y.
	void Add(const char *fmt, ...);
	void Init(Rocket::Core::Element *rocketElement);
	void SetAlternatePosition(int x, int y, bool enable=true);
private:
	std::string fCompleteMessage;
	Rocket::Core::Element *fRocketElement;

	// Some special messages shall be directed if inventory screen is shown
	bool fActivateDropMessage;
	int fDropX, fDropY; // Where the special messages shall be shown
};

extern MsgWindow gMsgWindow;

}
