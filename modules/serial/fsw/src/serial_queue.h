#ifndef _serial_queue_h_
#define _serial_queue_h_

#include "cfe.h"
#include "serial_sbn_if_struct.h"
#include "sbn_interfaces.h"

/* Function declarations */
int Serial_QueueGetMsg(uint32 queue, uint32 semId, NetDataUnion *MsgBuf);
int Serial_QueueAddNode(uint32 queue, uint32 semId, NetDataUnion *MsgBuf);
int Serial_QueueRemoveNode(uint32 queue);

#endif

