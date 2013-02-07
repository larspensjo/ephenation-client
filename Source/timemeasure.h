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

#include <map>
#include <string>

// Header file defines gDebugOpenGL that can be used to disable the timers.
#include "primitives.h"

/// Measure shader execution time, and provide a report.
class TimeMeasure {
public:
	/// Use this to create a static instance.
	/// @param title A string used in the report.
	TimeMeasure(const std::string &title) : fQuery(0), fFirst(true), fTitle(title) {}

	/// Start the timer.
	void Start() {
		if (!gDebugOpenGL)
			return;
		if (fFirst) {
			glGenQueries(1, &fQuery);
			fFirst = false;
			fTitles[fQuery] = fTitle;
			fResults[fQuery] = 0.0;
		} else {
			GLuint result;
			glGetQueryObjectuiv(fQuery, GL_QUERY_RESULT, &result);
			fResults[fQuery] = result * 0.000001;
		}
		glBeginQuery(GL_TIME_ELAPSED, fQuery);
	}

	/// Stop the timer.
	void Stop(void) {
		if (!gDebugOpenGL)
			return;
		glEndQuery(GL_TIME_ELAPSED);
	}

	/// Make a report of all timers, showing the last result.
	static void Report(void);
private:
	GLuint fQuery;
	bool fFirst;
	std::string fTitle;
	static std::map<GLuint, double> fResults;
	static std::map<GLuint, std::string> fTitles;
};
