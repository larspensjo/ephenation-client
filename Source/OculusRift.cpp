#include "OculusRift.h"

using namespace OVR;

OculusRift::OculusRift() : fSystem(Log::ConfigureDefaultLog(LogMask_All))
{
}

OculusRift::~OculusRift()
{
	//dtor
}

void OculusRift::Create() {
	Ptr<DeviceManager> pManager;
	Ptr<HMDDevice> pHMD;
	pManager = *DeviceManager::Create();
	pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
	HMDInfo hmd;
	if (pHMD != nullptr && pHMD->GetDeviceInfo(&hmd))
	{
		auto MonitorName = hmd.DisplayDeviceName;
		auto EyeDistance = hmd.InterpupillaryDistance;
		auto DistortionK0 = hmd.DistortionK[0];
		auto DistortionK1 = hmd.DistortionK[1];
		auto DistortionK2 = hmd.DistortionK[2];
		auto DistortionK3 = hmd.DistortionK[3];
	}
}
