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

#pragma once

#include "base.h"

namespace Rocket {
	namespace Core {
		class Element; // Forward declaration
		class ElementDocument;
	}
};

/// Manage the options dialog
class OptionsDialog : public BaseDialog {
public:

protected:
	virtual void UseDocument(Rocket::Core::ElementDocument *, std::function<void()> callback);
	virtual void FormEvent(Rocket::Core::Event& event, const string &action);
private:
	/// Update inputs of a specific element (if there are any).
	void UpdateInput(Rocket::Core::Element *);
};
