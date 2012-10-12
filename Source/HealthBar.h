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

class ColorShader;

// The original purpose of this class was to draw a health bar, but it is now also used for
// general simple colored areas.
class HealthBar {
public:
	static HealthBar *Make(void); // Make one single HealthBar, and always return the same.
	// Draw a health bar. 'hp' is the current hit points (0-1), 'dmg' is the last damage given (0-1).
	void DrawHealth(const glm::mat4 &projection, const glm::mat4 &model, float hp, float dmg, bool fillEnd) const;
	void DrawMana(const glm::mat4 &model, float mana) const;
	void DrawExp(const glm::mat4 &model, float exp) const;
	void DrawSquare(const glm::mat4 &model, float r, float g, float b, float a) const;
private:
	HealthBar(); // Constructor is private, to make it a singleton class.
	virtual ~HealthBar();
	void Init(void);
	static HealthBar fgSingleton;
	ColorShader *fShader;
	GLuint fBufferId, fVao;
};
