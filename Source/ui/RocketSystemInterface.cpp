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

#include <GL/glfw.h>
#include <stdio.h>

#include "RocketSystemInterface.h"
#include "../primitives.h"

bool RocketSystemInterface::LogMessage(Rocket::Core::Log::Type error_type, const Rocket::Core::String& message)
{

   // Discard LT_INFO information, we don't really need it for now
	if (Rocket::Core::Log::LT_INFO == error_type && !gDebugOpenGL)
      return true;

   std::string type;

	switch(error_type)
	{
	case Rocket::Core::Log::LT_ALWAYS:
		type = "[Always]";
		break;
	case Rocket::Core::Log::LT_ERROR:
		type = "[Error]";
		break;
	case Rocket::Core::Log::LT_ASSERT:
		type = "[Assert]";
		break;
	case Rocket::Core::Log::LT_WARNING:
		type = "[Warning]";
		break;
	case Rocket::Core::Log::LT_INFO:
		type = "[Info]";
		break;
	case Rocket::Core::Log::LT_DEBUG:
		type = "[Debug]";
		break;
	case Rocket::Core::Log::LT_MAX:
		type = "[Max]";
		break;
	};

	printf("%s - %s\n", type.c_str(), message.CString());

	return true;
}

float RocketSystemInterface::GetElapsedTime()
{
   return (float)glfwGetTime();
}

#ifdef WIN32
/// @todo This is a kludge. Assert() couldn't be found when linking with libRocket on Win32.

#include <Rocket/Core.h>

namespace Rocket {
namespace Core {

bool Assert(const char* msg, const char* file, int line)
{
	Rocket::Core::String message(1024, "%s\n%s:%d", msg, file, line);
	return GetSystemInterface()->LogMessage(Log::LT_ASSERT, message);
}

}
}

#endif // WIN32
