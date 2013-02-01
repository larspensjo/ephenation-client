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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "Error.h"
#include "../errormanager.h"

void ErrorDialog(const char *fmt, ...) {
	va_list vl;
	va_start(vl, fmt);
	char buff[1000];
	#ifdef WIN32
	_vsnprintf(buff, sizeof buff, fmt, vl);
	#else
	vsnprintf(buff, sizeof buff, fmt, vl);
	#endif

	auto &ss = View::gErrorManager.GetStream(false, false);
	ss << buff;
	// The caller of ErrorDialog() expects the function not to return.
	View::gErrorManager.Report();
	_Exit(1);
}
