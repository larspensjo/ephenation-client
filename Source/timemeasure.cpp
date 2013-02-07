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

#include <GL/glew.h>
#include <string>

#include "timemeasure.h"

using std::string;

void TimeMeasure::Report(void) {
	static unsigned prevSize = 0;
	if (prevSize != fTitles.size()) {
		for (auto it = fTitles.begin(); it != fTitles.end(); it++) {
			printf("%s ", it->second.c_str());
		}
		printf("\n");
	}
	prevSize = fTitles.size();
	for (auto it = fTitles.begin(); it != fTitles.end(); it++) {
		auto key = it->first;
		auto title = it->second;
		printf("%-*.2f ", int(title.length()), fResults[key]);
	}
	printf("\n");
}

std::map<GLuint, double> TimeMeasure::fResults;
std::map<GLuint, string> TimeMeasure::fTitles;
