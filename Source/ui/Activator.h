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
// This is data used by the activator dialog
//

#include "base.h"
#include "../chunk.h"

namespace Rocket {
	namespace Core {
		class Element;
	}
};

class ActivatorDialog : public BaseDialog {
public:
	ActivatorDialog();
	// Update inputs of a specific element (if there are any).
	virtual void UpdateInput(Rocket::Core::Element *);
protected:
	virtual void UseDocument(Rocket::Core::ElementDocument *);
private:
	int fDx, fDy, fDz;
	ChunkCoord fCC;
};

extern ActivatorDialog gActivatorDialog;
