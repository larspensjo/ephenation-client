// Copyright 2012 The Ephenation Authors
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

#include <stdio.h>

#include "modes.h"
#include "entitycomponentsystem.h"

GameMode gMode;

void GameMode::Set(Mode m) {
	fMode = m;
	Controller::gEntityComponentSystem.fEventManager.emit<ChangeEvt>(m);
}

GameMode::Mode GameMode::Get(void) const {
	return fMode;
}

GameMode::GameMode() : fMode(INIT) {
}
