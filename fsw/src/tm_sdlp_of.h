/* Overflow queue for TM frame segmentation, ported from NOS3 io_lib tm_sdlp.c.
 * Octets of a packet that do not fit in the frame wait in the queue and are
 * copied first into the next frame by TM_SDLP_myRefillFrameFromOverflow. */
#ifndef _tm_sdlp_of_h_
#define _tm_sdlp_of_h_

#include "tm_sdlp_my.h"

/* First Header Pointer when no packet starts in the frame (NOS3 tmtf.h:61). */
#define TM_SDLP_MY_NO_FIRST_HDR_PTR 0x7FF

/* Space Packet primary header length. */
#define TM_SDLP_MY_SPP_HDR_LENGTH 6

/* Same value as NOS3 tm_sdlp.h:52. */
#define TM_SDLP_MY_OVERFLOW_FULL (-6)

/* Points the empty queue at a buffer owned by the caller (TO_LAB). */
void TM_SDLP_myInitOverflowQueue(TMTF_myOverflowInfo_t *pOverflow, uint8 *pBuffer, uint16 buffSize);

/* Copies data at the next free frame octet; octets that do not fit go to the queue.
 * isPacket sets the First Header Pointer if no packet started in this frame yet.
 * Returns the free octets left in the frame, or a negative error (nothing copied). */
int32 TM_SDLP_myCopyToFrameOrOverflow(TM_SDLP_myFrameInfo_t *pFrameInfo, const uint8 *pData, uint16 dataLength, bool isPacket);

/* Fills an empty frame from the queue first, as NOS3 TM_SDLP_StartFrame does. */
void TM_SDLP_myRefillFrameFromOverflow(TM_SDLP_myFrameInfo_t *pFrameInfo);

#endif /* _tm_sdlp_of_h_ */
