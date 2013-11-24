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

#pragma once

#include "OVR.h"

namespace Controller {

class OculusRift
{
public:
	OculusRift();
	~OculusRift();
	void Create();
	float GetFieldOfView() const;
	float GetInterpupillaryDistance() const;
	float GetHorScreenSize() const;
	float GetLensSeparationDistance() const;

	void UseLeftEeye() { fLeftEyeSelected = true; }
	void UseRightEye() { fLeftEyeSelected = false; }
	static OculusRift sfOvr; // An instance of this class.
protected:
private:
	OVR::System fSystem;
	OVR::HMDInfo fInfo;

	bool fLeftEyeSelected = true;
};

};
