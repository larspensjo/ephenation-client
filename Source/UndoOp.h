// Copyright 2013-2014 The Ephenation Authors
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

#include <stack>
#include "glm/fwd.hpp"
#include "chunk.h"

namespace Controller {

typedef unsigned char OperationType; // block add/remove
typedef glm::vec3 BlockLocation;       // coordinate of block w.r.t chunk
typedef unsigned char BlockType;

/*!
 * @brief Represents the operation that needs to be undone or redone.
 *       Contains information about the block also. This is needed for
 *       performing the actual undo/redo.
 */
struct Operation {
    OperationType fOpType;
    BlockLocation fBlockCoord; // location of block in the chunk.
    ChunkCoord    fChunkBlock;
    BlockType     fBlkType;

    Operation(OperationType type, BlockLocation blkLoc, ChunkCoord chunkBlk, BlockType blkType)
    {
        fBlkType = blkType;
        fOpType  = type;
        fBlockCoord = blkLoc;
        fChunkBlock = chunkBlk;
    }
};

 /*!
  * @brief This class is used for providing an undo/redo provision while in the
  *       construction mode. Keyboard keys: U-Undo, R-Redo.
  *       So, for this, we maintain two stacks, an undo stack and a redo stack.
  *       When we need to undo, we push the topmost element from the undo stack to the
  *       redo stack, and just the opposite for redo.
  */
class UndoOp {
    private:
        std::stack<Operation*> fUndoOperationStack;
        std::stack<Operation*> fRedoOperationStack;

    public:
        UndoOp();
        ~UndoOp();

        void addOperation(Operation *op);
        void undoOperation();
        void redoOperation();

    private:
        /*!
        * @brief Server needs an update as to what needs to be undone.
        *       So, a message needs to be sent.
        */
        void updateServer(Operation*);

};

}
