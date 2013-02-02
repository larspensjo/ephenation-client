#include "UndoOp.h"
#include "connection.h"
#include "parse.h"

UndoOp::UndoOp()
{


}

UndoOp::~UndoOp()
{
    while (!fUndoOperationStack.empty()) {
        delete fUndoOperationStack.top();
        fUndoOperationStack.pop();
    }

     while (!fRedoOperationStack.empty()) {
        delete fRedoOperationStack.top();
        fRedoOperationStack.pop();
    }

}

void UndoOp::addOperation(Operation *op)
{
    fUndoOperationStack.push(op);
}

void UndoOp::updateServer(Operation* op)
{
    // @todo Clean up below code.
    if (op->fOpType == CMD_BLOCK_UPDATE) {
        unsigned char b[18];
        b[0] = sizeof b;
        b[1] = 0;
        b[2] = CMD_HIT_BLOCK;
        EncodeUint32(b+3,  op->fChunkBlock.x);
        EncodeUint32(b+7,  op->fChunkBlock.y);
        EncodeUint32(b+11, op->fChunkBlock.z);
        b[15] = op->fBlockCoord.x;
        b[16] = op->fBlockCoord.y;
        b[17] = op->fBlockCoord.z;
        SendMsg(b, sizeof b);
    } else {
        unsigned char b[19];
        b[0] = sizeof b;
        b[1] = 0;
        b[2] = CMD_BLOCK_UPDATE;
        EncodeUint32(b+3,  op->fChunkBlock.x);
        EncodeUint32(b+7,  op->fChunkBlock.y);
        EncodeUint32(b+11, op->fChunkBlock.z);
        b[15] = op->fBlockCoord.x;
        b[16] = op->fBlockCoord.y;
        b[17] = op->fBlockCoord.z;
        b[18] = op->fBlkType;
        SendMsg(b, sizeof b);
    }
}

void UndoOp::undoOperation()
{
    // Obtain most recent operation.
    if (!fUndoOperationStack.empty()) {
        Operation *op = fUndoOperationStack.top();
        fRedoOperationStack.push(op);
        fUndoOperationStack.pop();
        updateServer(op);

        // change operation type when moved from undo/redo stack.
        op->fOpType = (op->fOpType == CMD_BLOCK_UPDATE) ? (CMD_HIT_BLOCK) : (CMD_BLOCK_UPDATE);
    }
    else {
        return; // @todo show GUI message?
    }

    // @todo Check this?
	/*if (fBuildingBlocks->CurrentBlockType() == BT_Text) {
		fRequestActivatorChunk = cc;
		fRequestActivatorX = x;
		fRequestActivatorY = y;
		fRequestActivatorZ = z;
	}*/
}

void UndoOp::redoOperation()
{
    if (!fRedoOperationStack.empty()) {
        Operation *op = fRedoOperationStack.top();
        fUndoOperationStack.push(op);
        fRedoOperationStack.pop();
        updateServer(op);

        // change operation type when moved from undo/redo stack.
        op->fOpType = (op->fOpType == CMD_BLOCK_UPDATE) ? (CMD_HIT_BLOCK) : (CMD_BLOCK_UPDATE);
    } else {
        return; // @todo show GUI message?
    }

}
