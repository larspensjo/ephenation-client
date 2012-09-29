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
// This is a class that will manage a multi line input field
//

#include <string>
#include <memory>

using std::string;
using std::shared_ptr;

class DrawFont;

class DialogInput {
public:
	// Set the offset in pixels, number of lines, and 'width' is counted in pixels.
	DialogInput(int xoffs, int yoffs, int lines, int width);
	void Init(shared_ptr<DrawFont> &font);
	void Set(const string &);
	void Render(float alpha, bool active) const;

	// Handle a key press on the keyboard
	void HandleKey(int ch);

	// Handle a printable character.
	void HandleCharacter(int ch);

	// Return true if the given coordinate is inside the input field.
	bool Inside(int x, int y) const;
private:
	void Insert(char ch); // Insert a character at cursor and move cursor
	void Delete(void); // Delete character before cursor
	void DrawCursor(int x, int y, float alpha, bool active) const;

	// The height of the current font.
	int EffectiveHeight(void) const;

	string fString; // This the complete string that is being shown.
	shared_ptr<DrawFont> fFont;
	int fWidth;
	int fXoffset, fYoffset; // Measured in pixels
	int fMaxLines;
	unsigned fCursorPos;
};
