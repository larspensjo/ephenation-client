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

#pragma once

#include <entityx/Event.h>

/// Manage the game mode
class GameMode {
public:
	enum Mode {
		INIT,		// Starting mode
		LOGIN,		// Login dialog, wait for player to click Ok.
		REQ_PASSWD, // There has been a request from the server
		PASSWORD,	// Login name has been sent, waiting for password request
		WAIT_ACK,   // Waiting for login ack from the server (or reject).
		LOGIN_FAILED,
		GAME,		// Full game running.
		CONSTRUCT,	// Like game mode, but change the surroundings.
		TELEPORT,   // Similar to game mode, but enable teleporting
		ESC,		// Player pressed Esc or was disconnected, option-dialog active.
		EXIT,		// The client is exiting. All processes shall close down.
	};

	void Set(Mode);
	Mode Get(void) const;
	GameMode();
	bool CommunicationAllowed(void) const {
		return fMode == GAME || fMode == CONSTRUCT || fMode == TELEPORT;
	}

	/// Event generated everytime the game mode changes
	struct ChangeEvt : public entityx::Event<ChangeEvt> {
		ChangeEvt(Mode mode) : mode(mode) {}

		Mode mode; /// The new game mode
	};
private:
	Mode fMode;
};

extern GameMode gMode;
