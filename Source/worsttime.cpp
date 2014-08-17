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

#include "worsttime.h"

using std::string;

void WorstTime::Report(void) {
	static unsigned prevNumHeaders = 0;
	if (prevNumHeaders != fLast) {
		for (unsigned i=0; i < fLast; i++) {
			printf("%s ", fTitles[i].c_str());
		}
		printf("\n");
	}
	prevNumHeaders = fLast;
	for (unsigned i=0; i < fLast; i++) {
		printf("%*.2fms ", int(fTitles[i].length())-2, fResults[i]*1000.0);
		fResults[i] = 0.0;
	}
	printf("\n");
}

double WorstTime::fResults[MAXSIZE];
std::string WorstTime::fTitles[MAXSIZE];
unsigned WorstTime::fLast = 0;
