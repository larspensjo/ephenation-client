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

#pragma once
//
// This is a class that will manage a window with scrolling messages on the screen.
//

class DrawFont;
class HealthBar;

class MsgWindow {
public:
	MsgWindow(int lines, int width, int x, int y); // Draw the window at x,y.
	void Add(const char *fmt, ...);
	~MsgWindow();
	void Render(void) const;
	void Init(void);
	void SetAlternatePosition(int x, int y, bool enable=true);
private:
	unsigned int *fLines; // Pointer to sentences from VSFLFont
	int fNumLines;
	int fX, fY; // Coordinate on screen
	float fWidth; // Number of pixels
	float fMsgWinTransparency;
	HealthBar *fBackground; // The HealthBar class provides a mechanism for drawing filled rectangles.

	// Some special messages shall be directed if inventory screen is shown
	bool fActivateDropMessage;
	int fDropX, fDropY; // Where the special messages shall be shown
};

extern MsgWindow gMsgWindow;
extern bool gShowMsgWindow;
