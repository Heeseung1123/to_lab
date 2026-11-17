/* Minimal TM-SDLP framing for one fixed channel, reduced from io_lib tm_sdlp.h.
 * Packets that do not fit are segmented through the overflow queue (tm_sdlp_of.c). */
#ifndef _tm_sdlp_my_h_
#define _tm_sdlp_my_h_

#include "cfe.h"
#include "tmtf_my.h"
// #include "crypto.h"

#define TM_SDLP_MY_SUCCESS         (0)
#define TM_SDLP_MY_INVALID_LENGTH  (-3)

/* Fixed dummy channel IDs written into the premade frame header. */
#define TM_SDLP_MY_DUMMY_TFVN 0
#define TM_SDLP_MY_DUMMY_SCID 0x003
#define TM_SDLP_MY_DUMMY_VCID 1

#define TM_SDLP_MY_FRAME_SIZE 1786

/* Fixed SDLS layout: SPI 2 + IV 12 bytes and no trailer.
 * The area stays reserved and zeroed while encryption is disabled. */
#define TM_SDLP_MY_SDLS_HDR_LENGTH 14
#define TM_SDLP_MY_SDLS_TRL_LENGTH 0

/* An idle packet needs a 6 byte header and at least 1 data byte. */
#define TM_SDLP_MY_IDLE_MIN_LENGTH 7

/* Frame working data, as NOS3 TM_SDLP_FrameInfo_t; moved here from tm_sdlp_my.c. */
typedef struct
{
    uint16                dataFieldLength;   /* Length of the data field */
    uint16                dataFieldOffset;   /* Data field offset from the frame start */
    uint16                freeOctets;        /* Free octets left in the data field */
    uint16                currentDataOffset; /* Next free data octet from the frame start */
    uint8                 mcFrameCount;      /* Master Channel Frame Count */
    bool                  isFirstHdrPtrSet;  /* A packet already starts in this frame */
    TMTF_myOverflowInfo_t overflowInfo;      /* Octets waiting for the next frame */
    TMTF_my_PriHdr_t     *frame;             /* Transfer Frame buffer */
} TM_SDLP_myFrameInfo_t;

/* Builds the premade frame. Call once during application init. */
// int32 TM_SDLP_myInit(void);

/* Builds the premade frame and uses the TO_LAB buffer as the overflow queue.
 * Call once during application init. */
int32 TM_SDLP_myInitFrameAndOverflow(uint8 *pOverflowBuffer, uint16 overflowSize);

/* Adds one packet to the frame and sends every frame that gets completed.
 * Returns a negative value on a send error; a packet the queue cannot hold is dropped. */
int32 TM_SDLP_myAddPacketToFrame(const void *pPacket, size_t packetSize, osal_id_t sockId, const OS_SockAddr_t *pAddr);

/* Fills the current frame with idle data and sends it. An empty frame is not sent. */
int32 TM_SDLP_myFillAndSendFrame(osal_id_t sockId, const OS_SockAddr_t *pAddr);

#endif /* _tm_sdlp_my_h_ */
