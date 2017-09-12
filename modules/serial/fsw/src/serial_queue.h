#ifndef _serial_queue_h_
#define _serial_queue_h_

#include "cfe.h"

/* Function declarations */
int32 Serial_QueueGetMsg(uint32 queue, uint32 semId, uint8 *MsgBuf);
int32 Serial_QueueAddNode(uint32 queue, uint32 semId, uint8 *MsgBuf);
int32 Serial_QueueRemoveNode(uint32 queue);

#endif

