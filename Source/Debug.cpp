// Copyright 2013 The Ephenation Authors
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

#include <cstdarg>
#include <cstdio>
#include <ctime>

#include "Debug.h"

void LPLog(const char *func, const char *file, int line, const char *fmt, ...) {
	std::time_t result = std::time(NULL);
	char buff[30];
	std::strftime(buff, sizeof buff, "%c", std::localtime(&result));
	std::printf("%s %s:%s:%d ", buff, file, func, line);
	std::va_list args;
	va_start(args, fmt);
	std::vprintf(fmt, args);
	std::printf("\n");
	std::fflush(stdout);
}
