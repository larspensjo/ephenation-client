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

// =====================================================
/**
 * @class Quad
 *
 * @brief
 *  A very minimal shape, drawing a quad from 0 to 1 in x and y.
 *  A simple quad drawn using VAO and VBO
 */
 // =====================================================
class Quad {
public:
	Quad();
	~Quad();

        // =====================================================
        /**
         * @brief The draw routine
         */
        // =====================================================
	void Draw(void);
private:

        // =====================================================
        /**
         * @brief Creation of the VAO and VBO
         * @param void
         */
        // =====================================================
	void Init();
	GLuint fBufferId, fVao;
};

extern Quad gQuad;
