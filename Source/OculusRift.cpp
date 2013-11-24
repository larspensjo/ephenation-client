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

using namespace OVR;
using namespace Controller;

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
	Ptr<SensorDevice> pSensor;
	SensorFusion *pFusionResult = new SensorFusion();
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
	   pFusionResult->AttachToSensor(pSensor);
	}

	if (pHMD) {
		LPLOG(" [x] HMD Found");
	} else {
		LPLOG(" [ ] HMD Not Found");
	}

	if (pSensor) {
		LPLOG(" [x] Sensor Found");
	} else {
		LPLOG(" [ ] Sensor not found");
	}

	if (InfoLoaded) {
		cout << " DisplayDeviceName: " << fInfo.DisplayDeviceName << endl;
		cout << " ProductName: " << fInfo.ProductName << endl;
		cout << " Manufacturer: " << fInfo.Manufacturer << endl;
		cout << " Version: " << fInfo.Version << endl;
		cout << " HResolution: " << fInfo.HResolution<< endl;
		cout << " VResolution: " << fInfo.VResolution<< endl;
		cout << " HScreenSize: " << fInfo.HScreenSize<< endl;
		cout << " VScreenSize: " << fInfo.VScreenSize<< endl;
		cout << " VScreenCenter: " << fInfo.VScreenCenter<< endl;
		cout << " EyeToScreenDistance: " << fInfo.EyeToScreenDistance << endl;
		cout << " LensSeparationDistance: " << fInfo.LensSeparationDistance << endl;
		cout << " InterpupillaryDistance: " << fInfo.InterpupillaryDistance << endl;
		cout << " DistortionK[0]: " << fInfo.DistortionK[0] << endl;
		cout << " DistortionK[1]: " << fInfo.DistortionK[1] << endl;
		cout << " DistortionK[2]: " << fInfo.DistortionK[2] << endl;
		cout << "--------------------------" << endl;
	}

#if 0
	while(pSensor) {
		Quatf quaternion = pFusionResult->GetOrientation();

		float yaw, pitch, roll;
		quaternion.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &pitch, &roll);

		cout << " Yaw: " << RadToDegree(yaw) <<
			", Pitch: " << RadToDegree(pitch) <<
			", Roll: " << RadToDegree(roll) << endl;

		Sleep(50);

		if (_kbhit()) exit(0);
	}
#endif
	using namespace OVR::Util::Render;
	StereoConfig stereo;
	float renderScale;
	int Width = 1200, Height = 800;
	// Obtain setup data from the HMD and initialize StereoConfig for stereo rendering.
	stereo.SetFullViewport(Viewport(0,0, Width, Height));
	stereo.SetStereoMode(Stereo_LeftRight_Multipass);
	stereo.SetDistortionFitPointVP(-1.0f, 0.0f);
	renderScale = stereo.GetDistortionScale();

	StereoEyeParams leftEye = stereo.GetEyeRenderParams(StereoEye_Left);
	StereoEyeParams rightEye = stereo.GetEyeRenderParams(StereoEye_Right);
	// Left eye rendering parameters
	Viewport leftVP = leftEye.VP;
	Matrix4f leftProjection = leftEye.Projection;
	Matrix4f leftViewAdjust = leftEye.ViewAdjust;

	LPLOG("FoV: %f", GetFieldOfView());
	LPLOG("IPD: %f", GetInterpupillaryDistance());
	LPLOG("HScreenSize: %f", GetHorScreenSize());
	LPLOG("LSD: %f", GetLensSeparationDistance());
	LPLOG("Left eye proj adj: %f", GetHorProjectionAdjustment());
	LPLOG("Left eye view adj: %f m", GetHorViewAdjustment());
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
