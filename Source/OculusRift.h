#pragma once

#include "OVR.h"

class OculusRift
{
public:
	OculusRift();
	~OculusRift();
	void Create();
protected:
private:
	OVR::System fSystem;
};
