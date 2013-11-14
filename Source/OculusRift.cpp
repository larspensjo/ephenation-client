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
	Ptr<SensorDevice> pSensor;
	SensorFusion *pFusionResult = new SensorFusion();
	// Get some default values
	fInfo.EyeToScreenDistance = 0.041f;
	fInfo.DistortionK[0] = 1.0f;
	fInfo.DistortionK[1] = 0.22f;
	fInfo.DistortionK[2] = 0.24f;
	fInfo.DistortionK[3] = 0.0f;
	fInfo.InterpupillaryDistance = 0.064f;
	fInfo.VScreenSize = 0.16f;
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

	cout << "----- Oculus Console -----" << endl;

	if (pHMD) {
		cout << " [x] HMD Found" << endl;
	} else {
		cout << " [ ] HMD Not Found" << endl;
	}

	if (pSensor) {
		cout << " [x] Sensor Found" << endl;
	} else {
		cout << " [ ] Sensor Not Found" << endl;
	}

	cout << "--------------------------" << endl;

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

	cout << endl << " Press ENTER to continue" << endl;

	cin.get();
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
}

float OculusRift::GetFieldOfView() const {
	return 2.0f * atanf(fInfo.VScreenSize / 2.0f / fInfo.EyeToScreenDistance) * 180.0f / M_PI;
}
