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

//
// This is a work-around, to support semaphores in MinGW.
// At the time of writing, it wasn't supported by default.
//

#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

class ChunkProcess;

namespace std {

class mutex {
public:
	pthread_mutex_t fMutex;
	mutex() { pthread_mutex_init(&fMutex, 0); }
	void lock(void) { pthread_mutex_lock(&fMutex); }
	void unlock(void) { pthread_mutex_unlock(&fMutex); }
	bool try_lock(void) { return pthread_mutex_trylock(&fMutex) == 0; }
};

template <class T> class unique_lock {
public:
	T *fLock;
	unique_lock(T &lock) : fLock(&lock) {
		pthread_mutex_lock(&fLock->fMutex);
	}
};

class condition_variable {
	pthread_cond_t fCondLock;
public:
	condition_variable() { pthread_cond_init(&fCondLock, 0); }
	void wait(unique_lock<mutex>& lock) { pthread_cond_wait(&fCondLock, &lock.fLock->fMutex); }
	void notify_all(void) { pthread_cond_broadcast(&fCondLock); }
	void notify_one(void) { pthread_cond_signal(&fCondLock); }
};

class thread {
	pthread_t fThread;
	bool valid;
public:
	thread() : valid(false) {}
	thread(void* (*func)(ChunkProcess *p), ChunkProcess *p) : valid(true) {
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&fThread, &attr, (void* (*)(void*))func, (void *)p);
		pthread_attr_destroy(&attr);
	}
	thread(thread &&other) { // Move constructor
		fThread = other.fThread; valid = other.valid; other.valid = false;
	}
	thread(const thread&) = delete; // Copy constructor, don't allow.
	void join(void) {
		assert(valid);
		pthread_join(fThread, NULL);
	}
	bool joinable(void) const { return valid; }
};

};
