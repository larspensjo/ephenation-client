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

#include <GL/glfw.h>
#include <stdio.h>

#include "TSExec.h"
#include "SoundControl.h"
#include "modes.h"

TSExec::TSExec() {
	pthread_attr_init(&fAttr);
}

TSExec::~TSExec() {
	// pthread_join(fThread, NULL);					// Wait for child process to terminate. TODO: Need a mechanism to make thread terminate
	pthread_attr_destroy(&fAttr);
}

void TSExec::Init(void) {
	/* For portability, explicitly create threads in a joinable state */
	pthread_attr_setdetachstate(&fAttr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&fThread, &fAttr, TSExec::Thread, (void *)this); // This starts the child thread
}

// This is executed in the sound thread.
void *TSExec::Thread(void *p) {
	// TSExec *context = (TSExec *)p;
	while(1) {
		glfwSleep(0.1);
		if (gMode.Get() == GameMode::EXIT)
			pthread_exit(0);
		// printf("TSExec::Thread %d\n", context->fThread);
		gSoundControl.RequestSound(SoundControl::STSExec); // This will wakeup this process
	}
	return 0; // Not reached
}

TSExec TSExec::gTSExec;
