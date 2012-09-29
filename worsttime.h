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

//
// A class to measure real worst execution time, and provide a report.
//

#pragma once

#include <string>
#include <GL/glfw.h>

#include "primitives.h"

class WorstTime {
public:
	WorstTime(const std::string &title) : fIndex(0) {
		if (!gDebugOpenGL || fLast == MAXSIZE)
			return; // Quietly ignore
		fIndex = fLast++; // Allocate a slot
		fTitles[fIndex] = title;
	}
	void Start() {
		if (!gDebugOpenGL)
			return;
		fStart = glfwGetTime();
	}
	void Stop(void) {
		if (!gDebugOpenGL)
			return;
		double delta = glfwGetTime() - fStart;
		if (delta > fResults[fIndex])
			fResults[fIndex] = delta;
	}

	static void Report(void);

	double Get(void) const { return fResults[fIndex]; }
private:
	enum { MAXSIZE = 20 };
	double fStart;
	static double fResults[MAXSIZE];
	static std::string fTitles[MAXSIZE];
	static unsigned fLast;

	int fIndex;
};
