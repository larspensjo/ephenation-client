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

#include "Nausea.h"
#include "primitives.h"
#include "simplexnoise1234.h"
#include "MainEventSignals.h"
#include "Debug.h"

using namespace View;

Nausea Nausea::sgNausea;

void Nausea::Init() {
	gPreFrameUpdate.connect(Simple::slot (&sgNausea, &Nausea::Update));
	fStartTimer = gCurrentFrameTime;
}

void Nausea::Update() {
	if (!fEnabled)
		return;
	fScale = (gCurrentFrameTime - fStartTimer) / 20;
}

float Nausea::Noise(float offset) const {
	return snoise1(gCurrentFrameTime*fTimefactor + offset)*fScale;
}

float Nausea::FieldOfViewOffset() const {
	if (fEnabled)
		return Noise(0)*5;
	else
		return 0;
}

float Nausea::RollOffset() const {
	if (fEnabled)
		return Noise(1232.12312)*50;
	else
		return 0;
}

float Nausea::YawOffset() const {
	if (fEnabled)
		return Noise(93.12321)*50;
	else
		return 0;
}

float Nausea::PitchOffset() const {
	if (fEnabled)
		return Noise(23123.12312)*50;
	else
		return 0;
}

float Nausea::HeightOffset() const {
	if (fEnabled)
		return Noise(4201.23123);
	else
		return 0;
}
