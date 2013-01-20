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

#include "base.h"
#include "../chunk.h"

// Forward declarations
namespace Rocket {
	namespace Core {
		class Element;
	}
	namespace Controls {
		class ElementFormControlSelect;
	}
};

/// Manage data used by the activator block dialog
class ActivatorDialog : public BaseDialog {
public:
	ActivatorDialog() : fSoundSelector(0) {}

protected:
	virtual void UseDocument(Rocket::Core::ElementDocument *, std::function<void()> callback);
	virtual void FormEvent(Rocket::Core::Event& event, const string &action);
private:
	/// Override this as we want to catch click on special buttons
	virtual bool ClickEvent(Rocket::Core::Event& event, const string &action);
	// Update inputs of a specific element (if there are any).
	void UpdateInput(Rocket::Core::Element *);

	int fDx, fDy, fDz;
	ChunkCoord fCC;
	/// Remember the sound selector
	Rocket::Controls::ElementFormControlSelect *fSoundSelector;
};
