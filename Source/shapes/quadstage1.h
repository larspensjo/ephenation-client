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

// Class for creating a single quad. The result is two triangles. It has to be used with a
// StageOneShader.
class QuadStage1 {
public:
	QuadStage1();
	~QuadStage1();
	// Create a list of triangles representing a QuadStage1. The size is a unit size.
	void Init(void);
	void DrawDoubleSide(void) const; // Draw a QuadStage1, using the currently bound texture0.
	void DrawSingleSide(void) const; // Draw a QuadStage1, using the currently bound texture0.
	void DrawLines(void) const; // Only draw the edges.
private:
	GLuint fBufferId;
	GLuint fVao; // Vertex Attribute Object
};

extern QuadStage1 gQuadStage1;
