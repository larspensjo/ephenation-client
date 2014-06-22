// Copyright 2014 The Ephenation Authors
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

namespace View {

class Nausea
{
public:
	void Init();
	float FieldOfViewOffset() const;
	float RollOffset() const;
	float YawOffset() const;
	float PitchOffset() const;
	float HeightOffset() const;
	static Nausea sgNausea;
private:
	float Noise(float offset) const;
	void Update();

	bool fEnabled = true;
	float fTimefactor = 0.1f;
	float fScale = 0.0f;
	double fStartTimer = 0.0;
};

}
