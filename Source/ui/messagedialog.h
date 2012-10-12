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

// This class manages the in-game dialog messages.

#include "dialog.h"

class MessageDialog : public dialog {
public:
	// Initialize a dialog. Call the callback function (if any) when it is closed.
	MessageDialog(const std::string &title, const std::string &body, void (*callback)(void) = 0);

	// Draw the dialog.
	virtual void Draw(DrawTexture *drawTexture, float alpha);

	// The player clicked on something.
	virtual void Click(void);

	virtual void Escape(void);
private:
	DialogText fClose, fTitle, fBody;
	enum class ActiveElement { NONE, CLOSE };
	ActiveElement fActiveElement;
	void (*fCallback)(void);
};
