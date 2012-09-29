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

// This class will set up parallel processes (a thread pool) that will manage chunks.
// This is a singleton class, that manages all threads. The number of allocated threads
// depend on the number of cores of the PC.
//
// There are two types of tasks.
// 1. Load the binary data into a "struct chunk".
// 2. Compute the graphical objects from the binary data.
//
// The reason that these are not always combined into one single step is that it is not
// always the case that the graphical objects are needed. They are only needed if the
// chunk is visible.
// OpenGL data can not be managed from this process, as OpenGL is not thread safe.
//
// It is possible to extend with more types of tasks, not only chunk computations. The requirements
// for extending this mechanisms for other tasks are:
// * It requires a computation that is resource-intensitive
// * The computation can be done in parallel with no dependency on other data
//

#include <deque>
#include <set>
#include <memory>
#include <vector>

#ifdef WIN32
#include "mythread.h"
#else
#include <thread>
#include <mutex>
#include <condition_variable>
#endif

#include "ChunkObject.h"

using std::shared_ptr;
using std::unique_ptr;

class chunk;
class ChunkBlocks;
class ChunkObject;

// All functions in the public interface are thread safe. They will usually be called from the main process.
class ChunkProcess {
public:
	ChunkProcess();
	virtual ~ChunkProcess();
	void Init(int numThreads); // Create a pool of threads

	// Compute all graphical objects in a chunk. The result has to be polled from GetRecomputedObjects.
	void AddTaskComputeChunk(chunk *);
	// Get all new recomputed objects
	void GetRecomputedObjects(void);

	// Load new chunk data into a chunk, but do not recompute it. It will replace the previous one
	void AddTaskNewChunk(unique_ptr<ChunkBlocks>);

	// Request the processes to terminate
	void RequestTerminate(void);
private:
	std::condition_variable fCondLock;	// The conditional lock
	std::mutex fMutex;		// The mutex used for locking
	std::vector<std::thread> fThreads;			// Identifies the child threads
	// Callback function used to initialize threads.
	static void *ThreadStatic(ChunkProcess *p);
	// Every child thread executes this function, which will not normally return
	void Task(void);

	// ==============================================================================================================
	// The following variables must always be guarded by mutex 'fMutex' when they are updated. That is because they will be used by
	// both the main process as well as the local Task() process.
	//
	bool fTerminate;               // The processes has been requested to terminate
	std::deque<chunk*> fChunkFifo; // The list of chunk recomputation jobs
	std::deque<shared_ptr<ChunkBlocks>> fNewChunks; // List of "new chunk" jobs.
	// This is where the computed objects are saved. Use a set, to make sure every element is only ever once in it.
	std::set<shared_ptr<ChunkObject>> fComputedObjects;
	//
	// End of list of mutex protected variables.
	// ==============================================================================================================
};

// Create one global process
extern ChunkProcess gChunkProcess;
