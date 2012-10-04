#include <GL/glfw.h>

#include "RocketSystemInterface.h"
#include "Error.h"

bool RocketSystemInterface::LogMessage(Rocket::Core::Log::Type error_type, const Rocket::Core::String& message)
{
   // Discard LT_INFO information, we don't really need it for now
	if (Rocket::Core::Log::LT_INFO == error_type)
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

	ErrorDialog("%s - %s\n", type.c_str(), message.CString());

	return true;
}

float RocketSystemInterface::GetElapsedTime()
{
   return (float)glfwGetTime();
}
