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

namespace View {
	class HealthBar;
}

namespace Model {

/// This is a simple interface to make it easier to send monsters or players as the same pointer.
class Object {
public:
	virtual unsigned long GetId() const = 0;
	virtual int GetType() const = 0;
	virtual int GetLevel() const = 0;
	virtual glm::vec3 GetPosition() const = 0;
	virtual glm::vec3 GetSelectionColor() const = 0; // The color to draw on the ground when object selected
	virtual bool IsDead(void) const = 0;
	virtual void RenderHealthBar(View::HealthBar *, float angle) const = 0;
	virtual bool InGame(void) const = 0; // Return true if this object is still i the game.
	virtual ~Object() {} // Virtual destructor needed for inherited classes
private:
};

}
