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

#include <stdio.h>

#include "ChunkProcess.h"
#include "chunk.h"
#include "ChunkObject.h"
#include "chunkcache.h"
#include "assert.h"
#include "primitives.h"
#include "player.h"
#include "render.h"

ChunkProcess gChunkProcess;

ChunkProcess::ChunkProcess() {
	fTerminate = false;
}

ChunkProcess::~ChunkProcess() {
	this->RequestTerminate();
}

void ChunkProcess::Init(int numThreads) {
	for (int i=0; i < numThreads; i++) {
		std::thread t(ChunkProcess::ThreadStatic, this);
		fThreads.push_back(std::move(t));
	}
}

// Get a chunk from the queue, but chose the one that is nearest to the player. This function is thread safe.
// Sorting can not help, as the player moves around so that the definition of "nearest" change by time.
static View::Chunk *getNextChunk(std::deque<View::Chunk*> &queue) {
	ChunkCoord cc;
	double nearestDist2 = 1e10; // Anything very big
	//  The player position may change and is not thread safe. Bu the worst that can happen is that wrong chunk is selected.
	gPlayer.GetChunkCoord(&cc);
	int x = int(maxRenderDistance / CHUNK_SIZE + 1);
	int max2 = x*x; // The player can't see more than this number of chunks.
restart:
	if (queue.empty())
		return 0;
	auto nearest = queue.begin();
	for (auto it = queue.begin(); it != queue.end(); it++) {
		View::Chunk *cp = *it;
		auto dx = cc.x - cp->cc.x;
		auto dy = cc.y - cp->cc.y;
		auto dz = cc.z - cp->cc.z;
		auto d2 = dx*dx + dy*dy + dz*dz;
		if (d2 > max2) {
			// printf("ChunkProcess getNextChunk: Discard %d,%d,%d\n", (*it)->cc.x, (*it)->cc.y, (*it)->cc.z);
			cp->fScheduledForComputation = false;
			cp->SetDirty(true); // Delay recomputation until next time chunk is needed.
			queue.erase(it); // Forget about this one for now.
			goto restart;     // Iterators are destroyed by erase.
		}
		if (d2 < nearestDist2) {
			nearestDist2 = d2;
			nearest = it;
		}
	}
	View::Chunk *cp = *nearest;  // Get a copy before the iterator is invalidated.
	queue.erase(nearest); // Remove the chunk from the queue.
	return cp;
}

// Get a ChunkBlocks from the queue, but chose the one that is nearest to the player. This function is thread safe.
static shared_ptr<ChunkBlocks> getNextChunkBlock(std::deque<shared_ptr<ChunkBlocks>> &queue) {
	ChunkCoord cc;
	double nearestDist2 = 1e10; // Anything very big
	//  The player position may change and is not thread safe. Bu the worst that can happen is that wrong chunk is selected.
	gPlayer.GetChunkCoord(&cc);
	auto nearest = queue.begin();
	for (auto it = queue.begin(); it != queue.end(); it++) {
		View::Chunk *cp = (*it)->fChunk;
		auto dx = cc.x - cp->cc.x;
		auto dy = cc.y - cp->cc.y;
		auto dz = cc.z - cp->cc.z;
		auto d2 = dx*dx + dy*dy + dz*dz;
		// Do not throw away chunks that are a long way from the player (as is done in getNextChunk), because these are
		// chunks that have been transferred from the server, and we do not want them to be transferred again.
		if (d2 < nearestDist2) {
			nearestDist2 = d2;
			nearest = it;
		}
	}
	auto cb = *nearest;  // Get a copy before the iterator is invalidated.
	queue.erase(nearest); // Remove the chunk from the queue.
	return cb;
}

void *ChunkProcess::ThreadStatic(ChunkProcess *cp) {
	cp->Task();
	return 0;
}

//
// Loop for ever, and wait for new jobs.
// The general principle used is repeating (starting with locked mutex):
//   wait(condition)
//   pop a job from the job queue
//   unlock(mutex)
//   Do the job.
//   lock(mutex)
//   put result in outgoing queue.
//
// This means that the actual job is done with no locked semaphores. Because of that, it is important that no data is
// manipulated that can also be accessed from the main process.
void ChunkProcess::Task(void) {
	std::unique_lock<std::mutex> lock(fMutex); // This will lock the mutex

	// The condition variable will not have any effect if not waiting for it. And that may happen
	// as the mutex is unlocked in the loop now and then.
	bool waitForCondition = true;

	// Stay in a loop forever, waiting for new messages to play
	while(1) {
		if (waitForCondition)
			fCondLock.wait(lock);
		waitForCondition = true; // Wait for condition next iteration

		if (fTerminate) {
			// Used to request that the child thread terminates.
			fMutex.unlock();
			break;
		}

		if (!fComputeObjectsInput.empty()) {
			// Take out the chunk from the fifo
			View::Chunk *ch = getNextChunk(fComputeObjectsInput);
			if (ch == 0)
				continue;
			if (ch->fScheduledForLoading) {
				ch->fScheduledForComputation = false;
				continue; // Ignore a recomputation, as the chunk will be loaded by new data anyway, later followed by a new computation
			}
			fMutex.unlock(); // Unlock the mutex while doing some heavy work
			// printf("ChunkProcess phase 1, size %lu, chunk %d,%d,%d\n", fComputeObjectsInput.size()+1, ch->cc.x, ch->cc.y, ch->cc.z);
			auto co = View::ChunkObject::Make(ch, false, 0, 0, 0);
			co->FindSpecialObjects(ch); // This will find light sources, and should be done before FindTriangles().
			fMutex.lock();
			ASSERT(ch->fScheduledForComputation);
			co->fChunk = ch;
			fComputedObjectsOutput.insert(std::move(co)); // Add it to the list of recomputed chunks
			waitForCondition = false; // Try another iteration
		}

		if (!fNewChunksInput.empty()) {
			auto nc = getNextChunkBlock(fNewChunksInput);
			View::Chunk *pc = nc->fChunk;
			// It may be that this chunk was also scheduled for recomputation. If so, the Uncompress() below that calls SetDirty() will not
			// schedule a new recomputation again.
			ASSERT(pc->fScheduledForLoading);
			fMutex.unlock(); // Unlock the mutex while doing some work
			// printf("ChunkProcess phase 2, size %lu\n", fNewChunksInput.size()+1);
			nc->Uncompress();

			// Save chunk in cache
			ChunkCache::cachunk chunkdata;
			chunkdata.cc = pc->cc;
			chunkdata.flag = nc->flag;
			chunkdata.fCheckSum = nc->fChecksum;
			chunkdata.fOwner = nc->fOwner;
			chunkdata.compressSize = nc->compressSize;
			chunkdata.compressedChunk = nc->fCompressedChunk.get();
			ChunkCache::fgChunkCache.SaveChunkInCache(&chunkdata); // TODO: Use a ChunkProcess::ChunkBlocks as argument instead
			fMutex.lock();
			ASSERT(nc->fChunk == pc);
			fNewChunksOutput.insert(nc); // Add it to the list of loaded chunks
			waitForCondition = false; // Try another iteration
		}
	}
}

void ChunkProcess::RequestTerminate(void) {
	if (fTerminate)
		return; // Already done
	fMutex.lock();		// Lock the mutex, to get access to a message variable
	fTerminate = true;	  			// Update the message to the child thread
	fCondLock.notify_all();			// Signal all child threads that there is something
	fMutex.unlock();				// Unlock the mutex; this will wakeup the child threads
	for (auto it = fThreads.begin(); it != fThreads.end(); it++) {
		if (it->joinable()) {
			it->join();					// Wait for child process to terminate
		}
	}
}

void ChunkProcess::AddTaskComputeChunk(View::Chunk *ch) {
	// printf("ChunkProcess::AddTaskComputeChunk chunk %d,%d,%d\n", ch->cc.x, ch->cc.y, ch->cc.z);
	// Try to lock the mutex. If it fails, try another time instead. We don't want to delay the main thread.
	if (!fMutex.try_lock())
		return;
	// If the chunk is already being recomputed, wait until another time
	if (!ch->fScheduledForLoading && !ch->fScheduledForComputation) {
		this->fComputeObjectsInput.push_back(ch);
		ch->fScheduledForComputation = true;
		ch->SetDirty(false);
	}
	fCondLock.notify_one();			// Signal the child thread that there is something
	fMutex.unlock();				// Unlock the mutex; this will wakeup the child thread
}

// This is polled from the main thread, as we can't replace Chunk object pointers asynchronously. That way,
// the actual update with the result will be done only from the main process.
void ChunkProcess::Poll(void) {
	// Lock the mutex, to get access to the message variable
	if (!fMutex.try_lock())
		return; // Failed, skip it for this time.

	for (auto it=fComputedObjectsOutput.begin() ; it != fComputedObjectsOutput.end(); it++ ) {
		auto co = *it;
		View::Chunk *cp = co->fChunk;
		co->fChunk = 0;
		ASSERT(cp != 0 && cp->fScheduledForComputation); // Should not have been cleared elsewhere.
		cp->fChunkObject = co;
		cp->ReleaseOpenGLBuffers();
		cp->fScheduledForComputation = false;
		// if (gVerbose) printf("ChunkProcess::Poll %d,%d,%d\n", cp->cc.x, cp->cc.y, cp->cc.z);
	}
	fComputedObjectsOutput.clear();

	for (auto it=fNewChunksOutput.begin() ; it != fNewChunksOutput.end(); it++ ) {
		auto cb = *it;
		View::Chunk *cp = cb->fChunk;
		cp->fChunkBlocks = cb;
		cp->fScheduledForLoading = false;
		cp->SetDirty(true); // Need to be done after clearing loading flag
		cp->UpdateNeighborChunks(); // This will now use the new ChunkBlocks
		// if (gVerbose) printf("ChunkProcess::Poll %d,%d,%d\n", cp->cc.x, cp->cc.y, cp->cc.z);
	}
	fNewChunksOutput.clear();

	fMutex.unlock();				// Unlock the mutex; this will wakeup the child thread
}

void ChunkProcess::AddTaskNewChunk(unique_ptr<ChunkBlocks> cb) {
	// The chunk may also have been scheduled for computation, but we can't do anything to that here.
	fMutex.lock();				// Lock the mutex, to get access to the message variable
	View::Chunk *cp = cb->fChunk;
	// if (gVerbose) printf("ChunkProcess::AddTaskNewChunk chunk %d,%d,%d\n", cp->cc.x, cp->cc.y, cp->cc.z);
	if (!cp->fScheduledForLoading) {
		ASSERT(cb->fCompressedChunk != nullptr);       // There must be something to unpack.
		this->fNewChunksInput.push_back(std::move(cb));
		cp->fScheduledForLoading = true;
		fCondLock.notify_one();			// Signal the child thread that there is something
	}
	fMutex.unlock();				// Unlock the mutex; this will wakeup the child thread
}
