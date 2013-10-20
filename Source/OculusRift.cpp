#include "OVR.h"

#include "OculusRift.h"

using namespace OVR;

OculusRift::OculusRift()
{
	System::Init(Log::ConfigureDefaultLog(LogMask_All));
}

OculusRift::~OculusRift()
{
	//dtor
}
