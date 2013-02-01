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

#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "errormanager.h"

void assert_failed(const char *str, const char *file, int linenumber) {
	static bool recursive = false;
	if (recursive)
		_Exit(1); // Force immediate termination
	recursive = true;
	auto &ss = View::gErrorManager.GetStream(false, false); // This may fail for some cases.
	ss << "Assert failed: " << str << " " << file << " line " << linenumber;
	View::gErrorManager.Report(); // Force immediate report
	exit(1);
}
