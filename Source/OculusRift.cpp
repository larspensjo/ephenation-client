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

#include <iostream>
#undef __STRICT_ANSI__ // To get definition of M_PI
#include <math.h>

using namespace std;

#include "OculusRift.h"
#include "Debug.h"
#include "Nausea.h"

extern string sgPopup, sgPopupTitle; // Ugly, sorry

using namespace OVR;
using namespace Controller;

OculusRift::OculusRift() : fSystem(Log::ConfigureDefaultLog(LogMask_All))
{
}

void OculusRift::Create() {
	mFusionResult = new SensorFusion();
	// Get some default values
	fInfo.EyeToScreenDistance = 0.041f;
	fInfo.DistortionK[0] = 1.0f;
	fInfo.DistortionK[1] = 0.22f;
	fInfo.DistortionK[2] = 0.24f;
	fInfo.DistortionK[3] = 0.0f;
	fInfo.InterpupillaryDistance = 0.064f;
	fInfo.VScreenSize = 0.0935f;
	fInfo.HScreenSize = 0.14976f;
	fInfo.VScreenCenter = fInfo.VScreenSize/2.0f;
	fInfo.LensSeparationDistance = 0.064f;
	fInfo.HResolution = 1200;
	fInfo.VResolution = 800;
	bool InfoLoaded = false;
	pManager = *DeviceManager::Create();
	pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();

	if (pHMD) {
		InfoLoaded = pHMD->GetDeviceInfo(&fInfo);
		pSensor = *pHMD->GetSensor();
	} else {
	   pSensor = *pManager->EnumerateDevices<SensorDevice>().CreateDevice();
	}

	if (pSensor) {
	   mFusionResult->AttachToSensor(pSensor);
	}

	if (pHMD == nullptr) {
		sgPopupTitle = "Oculus Rift";
		sgPopup += "No display found.\n";
		LPLOG("No display found");
	}

	if (pSensor == nullptr) {
		sgPopupTitle = "Oculus Rift";
		const string msg = "No sensor found. /dev/hidraw/xx should be read and writeable. Did you run the ConfigurePermissionsAndPackages.sh?";
		sgPopup += msg + ".\n";
#ifdef _WIN32
		LPLOG("No sensor found");
#else
		LPLOG("%s", msg.c_str());
#endif // _WIN32
	}

	if (InfoLoaded) {
		LPLOG("Info loaded");
		LPLOG(" DisplayDeviceName: %s", fInfo.DisplayDeviceName);
		LPLOG(" ProductName: %s", fInfo.ProductName);
		LPLOG(" Manufacturer: %s", fInfo.Manufacturer);
		LPLOG(" Version: %d", fInfo.Version);
		LPLOG(" HResolution: %d", fInfo.HResolution);
		LPLOG(" VResolution: %d", fInfo.VResolution);
		LPLOG(" HScreenSize: %f", fInfo.HScreenSize);
		LPLOG(" VScreenSize: %f", fInfo.VScreenSize);
		LPLOG(" VScreenCenter: %f", fInfo.VScreenCenter);
		LPLOG(" EyeToScreenDistance: %f", fInfo.EyeToScreenDistance);
		LPLOG(" LensSeparationDistance: %f", fInfo.LensSeparationDistance);
		LPLOG(" InterpupillaryDistance: %f", fInfo.InterpupillaryDistance);
		LPLOG(" DistortionK[0]: %f", fInfo.DistortionK[0]);
		LPLOG(" DistortionK[1]: %f", fInfo.DistortionK[1]);
		LPLOG(" DistortionK[2]: %f", fInfo.DistortionK[2]);
		LPLOG(" DistortionK[3]: %f", fInfo.DistortionK[3]);
		LPLOG("--------------------------");
	}

#if 0
	while(pSensor) {
		Quatf quaternion = mFusionResult->GetOrientation();

		float yaw, pitch, roll;
		quaternion.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &pitch, &roll);

		cout << " Yaw: " << RadToDegree(yaw) <<
			", Pitch: " << RadToDegree(pitch) <<
			", Roll: " << RadToDegree(roll) << endl;

		Sleep(50);
	}
#endif
	using namespace OVR::Util::Render;

	LPLOG("FoV: %f", GetFieldOfView());
	LPLOG("IPD: %f", GetInterpupillaryDistance());
	LPLOG("HScreenSize: %f", GetHorScreenSize());
	LPLOG("LSD: %f", GetLensSeparationDistance());
	LPLOG("Left eye proj adj: %f", GetHorProjectionAdjustment());
	LPLOG("Left eye view adj: %f m", GetHorViewAdjustment());
}

const float *OculusRift::GetDistortionConstants() const {
	return fInfo.DistortionK;
}

void OculusRift::GetYawPitchRoll(float ret[3]) {
	Quatf quaternion = mFusionResult->GetOrientation();

	float yaw, pitch, roll;
	quaternion.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &pitch, &roll);
	// TODO: This should be done elsewhere.
	// Add an optional delay, for extra nauseous feeling.
	// TODO: The decay should depend on the time.
	static float sYaw, sPitch, sRoll;
	static bool first = true;
	if (first) {
		sYaw = yaw; sPitch = pitch; sRoll = roll;
		first = false;
	}
	float f = View::Nausea::sgNausea.Decay();
	sYaw = f * yaw + (1-f) * sYaw;
	sPitch = f * pitch + (1-f) * sPitch;
	sRoll = f * roll + (1-f) * sRoll;
	ret[0] = RadToDegree(sYaw) + View::Nausea::sgNausea.YawOffset();
	ret[1] = RadToDegree(sPitch) + View::Nausea::sgNausea.PitchOffset();
	ret[2] = RadToDegree(sRoll) + View::Nausea::sgNausea.RollOffset();
}

float OculusRift::GetFieldOfView() const {
	return 2.0f * atanf(fInfo.VScreenSize / 2.0f / fInfo.EyeToScreenDistance) * 180.0f / M_PI;
}

float OculusRift::GetInterpupillaryDistance() const {
	return fInfo.InterpupillaryDistance;
}

float OculusRift::GetHorScreenSize() const {
	return fInfo.HScreenSize;
}

float OculusRift::GetHorResolution() const {
	return fInfo.HResolution;
}

float OculusRift::GetVerResolution() const {
	return fInfo.VResolution;
}

float OculusRift::GetLensSeparationDistance() const {
	return fInfo.LensSeparationDistance;
}

float OculusRift::GetHorViewAdjustment() const {
	if (fLeftEyeSelected)
		return fInfo.LensSeparationDistance/2.0f;
	else
		return -fInfo.LensSeparationDistance/2.0f;
}

float OculusRift::GetHorProjectionAdjustment() const {
	// The screen is divided into left and right side, so we get the distance, in meter, to the center of the left side.
	// Post-projection viewport coordinates range from (-1.0, 1.0), with the
	// center of the left viewport falling at (1/4) of horizontal screen size.
	// We need to shift this projection center to match with the lens center.
	// We compute this shift in physical units (meters) to correct
	// for different screen sizes and then rescale to viewport coordinates.
	float viewCenter = fInfo.HScreenSize * 0.25f;
	float eyeProjectionShift = viewCenter - fInfo.LensSeparationDistance*0.5f;
	float projectionCenterOffset = 4.0f * eyeProjectionShift / fInfo.HScreenSize;
	if (fLeftEyeSelected)
		return projectionCenterOffset;
	else
		return -projectionCenterOffset;
}

OculusRift OculusRift::sfOvr; // An instance of this class.
