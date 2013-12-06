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

#include <glm/glm.hpp>

namespace View {

/// Manage Heads Up Display positioning
class HudTransformation
{
public:
	/// Get the current GUI transformation
	const glm::mat4 &GetGUITransform() const { return fGUITransform; }

	/// Just get the view part of the transform
	const glm::mat4 &GetViewTransform() const { return fViewTransform; }

	/// Update the transformation.
	void Update();
private:
	float fGuiDistance = 10.0f; // A distance that feels good for the eyes, measured in meters.
	glm::mat4 fGUITransform;
	glm::mat4 fViewTransform;
};

extern HudTransformation gHudTransformation;

};
