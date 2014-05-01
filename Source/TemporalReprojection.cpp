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

#include <GL/glfw.h>

#include "TemporalReprojection.h"
#include "Debug.h"

void TemporalReprojection::Reset() {
	LPLOG("RESET count %d, Worst: %f '%s', shortest: %f '%s'", fCount, fWorst, fWorstDebug, fShortest, fShordebug);
	fTimer = glfwGetTime();
	fWorst = 0.0; fShortest = 1.0;
	fShordebug = fWorstDebug = "";
	fCount = 0;
}

void TemporalReprojection::Poll(const char *debug) {
	double now = glfwGetTime();
	double delta = now - fPreviousPoll;
	fCount++;
	LPLOG("%f (%f) '%s'", now - fTimer, delta, debug);
	fPreviousPoll = now;
	if (delta > fWorst) {
		fWorst = delta;
		fWorstDebug = debug;
	}
	if (delta < fShortest) {
		fShortest = delta;
		fShordebug = debug;
	}
}

TemporalReprojection TemporalReprojection::sgTemporalReprojection;
