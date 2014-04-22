// Copyright 2012-2014 The Ephenation Authors
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

#include "../OpenglBuffer.h"

/// Class for creating a single quad for deferred shading
///
/// The result is two triangles. It has to be used with a
/// StageOneShader.
class QuadStage1 {
public:
	~QuadStage1();
	/// Create a list of triangles representing a QuadStage1. The size is a unit size.
	void Init(void);
	/// Draw a QuadStage1, on both front and back, using the currently bound texture0.
	void DrawDoubleSide(void) const;
	/// Draw a QuadStage1 single side, using the currently bound texture0.
	void DrawSingleSide(void) const;
	/// Only draw the edges.
	void DrawLines(void) const;
	/// Draw a QuadStage1, on front size, using the currently bound texture0.
	/// @param count the number of instances to draw
	void DrawSingleSideInstances(int count) const;
private:
	OpenglBuffer fBuffer;
	GLuint fVao = 0; // Vertex Attribute Object
};

extern QuadStage1 gQuadStage1;
