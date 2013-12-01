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

#include "RocketSystemInterface.h"
#include "RocketRenderInterface.h"

class RocketGui
{
public:
	void Init(bool stereoView, float fieldOfView);

	void LoadFonts(const std::string& dir);

	~RocketGui();

	// Translate a GLFW key to a Rocket key.
	static Rocket::Core::Input::KeyIdentifier KeyMap(int key);
private:
	RocketSystemInterface fRocketSystemInterface;
	RocketRenderInterface fRocketRenderInterface;
};
