#ifndef _UNDO_OP_
#define _UNDO_OP_

#include <stack>
#include "glm/glm.hpp"
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

#endif
