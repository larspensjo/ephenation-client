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

#include <string>
#include <GL/glfw.h>

#include "primitives.h"

/// @brief Measure real worst execution time, and provide a report.
/// Surround a section of code as follows:
/// @code{.cpp}
/// static WorstTime tm("name");
/// tm.Start();
/// SomeFunctionTakingTime();
/// tm.Stop();
/// @endcode
/// From the main loop, call WorstTime::Report() now and then.
///
class WorstTime {
public:
    /// @brief Declare a timer
    /// @param title A string that names the timer, and will be included in the final report
	WorstTime(const std::string &title) : fStart(0.0), fIndex(0) {
		if (!gDebugOpenGL || fLast == MAXSIZE)
			return; // Quietly ignore
		fIndex = fLast++; // Allocate a slot
		fTitles[fIndex] = title;
	}

	/// @brief Start the timer
	void Start() {
		if (!gDebugOpenGL)
			return;
		fStart = glfwGetTime();
	}

	/// @brief Stop the timer. The worst case since the last report is saved.
	void Stop(void) {
		if (!gDebugOpenGL)
			return;
		double delta = glfwGetTime() - fStart;
		if (delta > fResults[fIndex])
			fResults[fIndex] = delta;
	}

    /// @brief Report all timers and reset worst case.
	static void Report(void);

private:
	enum { MAXSIZE = 20 }; /// @todo Use std::vector instead!
	double fStart;
	static double fResults[MAXSIZE];
	static std::string fTitles[MAXSIZE];
	static unsigned fLast;

	int fIndex;
};
