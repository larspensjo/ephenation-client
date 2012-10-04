#pragma once

#include <Rocket/Core/SystemInterface.h>

class RocketSystemInterface: public Rocket::Core::SystemInterface
{
public:

   virtual float GetElapsedTime();

   virtual bool LogMessage(Rocket::Core::Log::Type type, const Rocket::Core::String& message);
};
