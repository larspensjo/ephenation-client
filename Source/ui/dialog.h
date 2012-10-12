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
// This is a base class used for showing dialogs in the game window.
// The class itself has to be overrided.

#include <memory>
#include <utility>

#include "../DrawText.h"

class DrawTexture;
class DrawFont;

using std::unique_ptr;
using std::shared_ptr;

// This is a helper class to manage one string shown in a dialog.
class DialogText {
public:
	DialogText();
	void Init(shared_ptr<DrawFont> &font, const std::string &, int xoffs, int yoffs);
	~DialogText();
	void Draw(glm::vec3 color, float alpha) const;

	// Return true if the given coordinate is inside the text.
	bool Inside(int x, int y) const;
private:
	shared_ptr<DrawFont> fFont;
	unsigned int fIdx;
	int fWidth;
	int fXoffset, fYoffset;

	enum {
		LINEHEIGHT=50,
		HEIGHTCORRECTION=30 // For some reasons, there is a lot of space above the character
	};
};

class dialog {
public:
	// Define the dialog position in pixels. This is the drawable inside of a rectangle.
	// Some extra decoration will be added on the outside.
	dialog();

	// Draw the dialog. This specific will draw the background.
	virtual void Draw(DrawTexture *drawTexture, float alpha);

	// The player clicked on something.
	virtual void Click(void) = 0;

	// Give the dialog a chance to override this and clean up
	virtual void Escape(void) { dialog::Pop(); }

	// Give the dialogs a possibility to catch key press
	virtual void KeyPress(int ch) { return; }

	virtual void Character(int ch) { return; }

	// Get the offset to the upper left corner of the dialog. The first
	// position where text can be printed.
	static void GetOffset(float &x, float &y);

	// Return false if the windows is closed.
	static bool DispatchClick(void);

	// Draw a dialog. Optional, specify which one, otherwise it will be the main dialog.
	static void DispatchDraw(DrawTexture *drawTexture, float alpha, dialog *newDialog = 0);

	// The player pressed the ESC key. Return true if something is still shown.
	static bool DispatchEscape(void);

	static void DispatchKey(int ch);
	static void DispatchChar(int ch);
protected:
	shared_ptr<DrawFont> fBodyFont;
	shared_ptr<DrawFont> fMenuFont;
	shared_ptr<DrawFont> fTitleFont;

	// Get the mouse position relative the dialog oontent area (given by GetOffset)
	void GetMousePosition(float &x, float &y) const;

	static void Push(dialog*);
	static void Pop(void);    // This may destroy the current dialog, return immediately
private:
	static std::vector<unique_ptr<dialog> > fStack;
};
