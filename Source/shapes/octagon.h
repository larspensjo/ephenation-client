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

#include <GL/glew.h>

/// Drawing an octagon from -1 to 1 in x and y.
class Octagon {
public:
	Octagon();
	~Octagon();

	/// The draw routine
	void Draw();
private:
	void Init(); // Called automatically when needed.
	GLuint fIndexId, fBufferId, fVao;
};

extern Octagon gOctagon;
