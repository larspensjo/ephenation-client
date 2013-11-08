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
	HMDInfo Info;
	bool InfoLoaded = false;
	pManager = *DeviceManager::Create();
	pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();

	if (pHMD) {
		InfoLoaded = pHMD->GetDeviceInfo(&Info);
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
		cout << " DisplayDeviceName: " << Info.DisplayDeviceName << endl;
		cout << " ProductName: " << Info.ProductName << endl;
		cout << " Manufacturer: " << Info.Manufacturer << endl;
		cout << " Version: " << Info.Version << endl;
		cout << " HResolution: " << Info.HResolution<< endl;
		cout << " VResolution: " << Info.VResolution<< endl;
		cout << " HScreenSize: " << Info.HScreenSize<< endl;
		cout << " VScreenSize: " << Info.VScreenSize<< endl;
		cout << " VScreenCenter: " << Info.VScreenCenter<< endl;
		cout << " EyeToScreenDistance: " << Info.EyeToScreenDistance << endl;
		cout << " LensSeparationDistance: " << Info.LensSeparationDistance << endl;
		cout << " InterpupillaryDistance: " << Info.InterpupillaryDistance << endl;
		cout << " DistortionK[0]: " << Info.DistortionK[0] << endl;
		cout << " DistortionK[1]: " << Info.DistortionK[1] << endl;
		cout << " DistortionK[2]: " << Info.DistortionK[2] << endl;
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
