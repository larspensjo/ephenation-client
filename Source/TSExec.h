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

#include <pthread.h>
#include <entityx/Event.h>

#include "modes.h"

/// Instantiate a thread scheduler, which is itself a thread. It is a singleton class.
/// Init must be called once.
class TSExec : public entityx::Receiver<TSExec> {
public:
	void Init(entityx::EventManager &events);
	static TSExec gTSExec;

	/// Catch game mode change events.
	void receive(const GameMode::ChangeEvt &evt);
private:
	TSExec();
	~TSExec();
	pthread_attr_t fAttr;		// Used to setup the child thread
	pthread_t fThread;			// Identifies the child thread
	bool fShutdown;
	static void *Thread(void *p);	// This is the child thread
};
