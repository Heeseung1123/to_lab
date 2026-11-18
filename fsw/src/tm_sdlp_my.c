/* Minimal TM-SDLP framing for one fixed channel, reduced from io_lib tm_sdlp.c. */
#include "tm_sdlp_my.h"
#include "to_lab_events.h"
#include "tm_sdlp_of.h"
#include "to_lab_dest_my.h"

/* Moved to tm_sdlp_my.h with the overflow fields, so tm_sdlp_of.c can use it. */
// typedef struct
// {
//     uint16            dataFieldLength;   /* Length of the data field */
//     uint16            dataFieldOffset;   /* Data field offset from the frame start */
//     uint16            freeOctets;        /* Free octets left in the data field */
//     uint16            currentDataOffset; /* Next free data octet from the frame start */
//     uint8             mcFrameCount;      /* Master Channel Frame Count */
//     TMTF_my_PriHdr_t *frame;             /* Transfer Frame buffer */
// } TM_SDLP_myFrameInfo_t;

static TM_SDLP_myFrameInfo_t TM_SDLP_myFrameInfo;
static uint8                 TM_SDLP_myFrameBuffer[TM_SDLP_MY_FRAME_SIZE];

/* Not used: packets are now segmented instead of being moved to the next frame. */
// static bool TM_SDLP_myFits(size_t packetSize, size_t freeOctets)
// {
//     return (packetSize == freeOctets) || (packetSize + TM_SDLP_MY_IDLE_MIN_LENGTH <= freeOctets);
// }

/* Fills the remaining data field with one idle packet. */
static void TM_SDLP_myFillFrameWithIdle(TM_SDLP_myFrameInfo_t *pFrameInfo)
{
    uint8 *pIdle;
    uint16 idleLengthField;

    if (pFrameInfo->freeOctets == 0)
    {
        return;
    }

    /* Fewer than 7 octets left: a 7 octet idle packet is split into the queue,
     * as NOS3 TM_SDLP_AddIdlePacket does (tm_sdlp.c:413-438). */
    if (pFrameInfo->freeOctets < TM_SDLP_MY_IDLE_MIN_LENGTH)
    {
        static const uint8 shortIdle[TM_SDLP_MY_IDLE_MIN_LENGTH] = {0x07, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00};

        if (TM_SDLP_myCopyToFrameOrOverflow(pFrameInfo, shortIdle, sizeof(shortIdle), true) < 0)
        {
            /* Queue full: zero the rest so no old data is sent. */
            CFE_PSP_MemSet((uint8 *)pFrameInfo->frame + pFrameInfo->currentDataOffset, 0, pFrameInfo->freeOctets);
            pFrameInfo->currentDataOffset += pFrameInfo->freeOctets;
            pFrameInfo->freeOctets = 0;
        }
        return;
    }

    pIdle           = (uint8 *)pFrameInfo->frame + pFrameInfo->currentDataOffset;
    idleLengthField = pFrameInfo->freeOctets - TM_SDLP_MY_IDLE_MIN_LENGTH;

    /* Zero the area first so data from the previous frame is not sent again. */
    CFE_PSP_MemSet(pIdle, 0, pFrameInfo->freeOctets);

    /* CCSDS header: idle APID 0x7FF, unsegmented sequence flags and data length. */
    pIdle[0] = 0x07;
    pIdle[1] = 0xFF;
    pIdle[2] = 0xC0;
    pIdle[4] = (uint8)(idleLengthField >> 8);
    pIdle[5] = (uint8)(idleLengthField & 0xFF);

    /* The idle packet also counts for the First Header Pointer. */
    if (!pFrameInfo->isFirstHdrPtrSet)
    {
        TMTF_MY_WR_FIRSTHDRPTR(*pFrameInfo->frame, pFrameInfo->currentDataOffset - pFrameInfo->dataFieldOffset);
        pFrameInfo->isFirstHdrPtrSet = true;
    }

    pFrameInfo->currentDataOffset += pFrameInfo->freeOctets;
    pFrameInfo->freeOctets = 0;
}

/* Writes the Master Channel Frame Count and rewinds the frame for the next packets. */
static void TM_SDLP_myCloseFrame(TM_SDLP_myFrameInfo_t *pFrameInfo)
{
    pFrameInfo->mcFrameCount++;
    TMTF_MY_WR_MCFRMCNT(*pFrameInfo->frame, pFrameInfo->mcFrameCount);

    /* The frame holds only the middle of a packet (NOS3 tm_sdlp.c:676-680). */
    if (!pFrameInfo->isFirstHdrPtrSet)
    {
        TMTF_MY_WR_FIRSTHDRPTR(*pFrameInfo->frame, TM_SDLP_MY_NO_FIRST_HDR_PTR);
    }

    pFrameInfo->freeOctets        = pFrameInfo->dataFieldLength;
    pFrameInfo->currentDataOffset = pFrameInfo->dataFieldOffset;
    pFrameInfo->isFirstHdrPtrSet  = false;
}

// int32 TM_SDLP_myInit(void)
int32 TM_SDLP_myInitFrameAndOverflow(uint8 *pOverflowBuffer, uint16 overflowSize)
{
    TM_SDLP_myFrameInfo_t *pFrameInfo = &TM_SDLP_myFrameInfo;
    int32                  dataFieldLength;
    uint16                 dataFieldOffset;
    uint8                  sdlsSecurityHeaderLength;
    uint8                  sdlsSecurityTrailerLength;
    // SecurityAssociation_t *sa_ptr = NULL;

    /* Encryption is disabled, so fixed lengths replace the SA lookup. */
    // iStatus = sa_if->sa_get_operational_sa_from_gvcid(0, TM_SDLP_MY_DUMMY_SCID, TM_SDLP_MY_DUMMY_VCID, 0, &sa_ptr);
    // if (iStatus != CRYPTO_LIB_SUCCESS)
    // {
    //     return iStatus;
    // }
    // sdlsSecurityHeaderLength  = Crypto_Get_Security_Header_Length(sa_ptr);
    // sdlsSecurityTrailerLength = Crypto_Get_Security_Trailer_Length(sa_ptr);
    sdlsSecurityHeaderLength  = TM_SDLP_MY_SDLS_HDR_LENGTH;
    sdlsSecurityTrailerLength = TM_SDLP_MY_SDLS_TRL_LENGTH;

    dataFieldOffset = TMTF_MY_PRIHDR_LENGTH + sdlsSecurityHeaderLength;
    dataFieldLength = (int32)TM_SDLP_MY_FRAME_SIZE - dataFieldOffset - sdlsSecurityTrailerLength;

    if (dataFieldLength < TM_SDLP_MY_IDLE_MIN_LENGTH)
    {
        CFE_EVS_SendEvent(TO_FRAMEINIT_ERR_EID, CFE_EVS_EventType_ERROR, "TO invalid TM frame data length %d",
                          (int)dataFieldLength);
        return TM_SDLP_MY_INVALID_LENGTH;
    }

    if ((pOverflowBuffer == NULL) || (overflowSize == 0))
    {
        CFE_EVS_SendEvent(TO_FRAMEINIT_ERR_EID, CFE_EVS_EventType_ERROR, "TO invalid TM overflow buffer");
        return TM_SDLP_MY_INVALID_LENGTH;
    }

    pFrameInfo->dataFieldLength   = (uint16)dataFieldLength;
    pFrameInfo->dataFieldOffset   = dataFieldOffset;
    pFrameInfo->freeOctets        = pFrameInfo->dataFieldLength;
    pFrameInfo->currentDataOffset = dataFieldOffset;
    pFrameInfo->mcFrameCount      = 0;
    pFrameInfo->isFirstHdrPtrSet  = false;
    pFrameInfo->frame             = (TMTF_my_PriHdr_t *)TM_SDLP_myFrameBuffer;

    CFE_PSP_MemSet(TM_SDLP_myFrameBuffer, 0, sizeof(TM_SDLP_myFrameBuffer));

    TMTF_MY_WR_TF_VERSION(*pFrameInfo->frame, TM_SDLP_MY_DUMMY_TFVN);
    TMTF_MY_WR_SCID(*pFrameInfo->frame, TM_SDLP_MY_DUMMY_SCID);
    TMTF_MY_WR_VCID(*pFrameInfo->frame, TM_SDLP_MY_DUMMY_VCID);
    TMTF_MY_WR_SEGLENGTHID(*pFrameInfo->frame, TMTF_MY_SEGLENGTHID_PKT);

    /* Packets are never segmented, so every sent frame starts with a packet. */
    // TMTF_MY_WR_FIRSTHDRPTR(*pFrameInfo->frame, 0);

    /* The First Header Pointer is now written per frame by AddData and CompleteFrame. */
    TM_SDLP_myInitOverflowQueue(&pFrameInfo->overflowInfo, pOverflowBuffer, overflowSize);

    return TM_SDLP_MY_SUCCESS;
}

int32 TM_SDLP_myFillAndSendFrame(osal_id_t sockId, const OS_SockAddr_t *pAddr)
{
    TM_SDLP_myFrameInfo_t *pFrameInfo = &TM_SDLP_myFrameInfo;
    int32                  status;

    if (pFrameInfo->freeOctets == pFrameInfo->dataFieldLength)
    {
        return TM_SDLP_MY_SUCCESS;
    }

    /* A long queue can fill the next frame at once, so send until one has room. */
    do
    {
        TM_SDLP_myFillFrameWithIdle(pFrameInfo);
        TM_SDLP_myCloseFrame(pFrameInfo);

        /* Encryption is disabled for now. */
        // iStatus = Crypto_TM_ApplySecurity((uint8_t *)pFrameInfo->frame, TM_SDLP_MY_FRAME_SIZE);
        // if (iStatus != CRYPTO_LIB_SUCCESS)
        // {
        //     return iStatus;
        // }

        // return OS_SocketSendTo(sockId, TM_SDLP_myFrameBuffer, sizeof(TM_SDLP_myFrameBuffer), pAddr);
        // status = OS_SocketSendTo(sockId, TM_SDLP_myFrameBuffer, sizeof(TM_SDLP_myFrameBuffer), pAddr);
        /* The same frame also goes to every added destination (to_lab_dest_my.c). */
        status = TO_LAB_SendToAllDests(sockId, TM_SDLP_myFrameBuffer, sizeof(TM_SDLP_myFrameBuffer), pAddr);
        if (status < 0)
        {
            return status;
        }

        /* The queue goes first into the next frame (NOS3 TM_SDLP_StartFrame). */
        TM_SDLP_myRefillFrameFromOverflow(pFrameInfo);
    } while (pFrameInfo->freeOctets == 0);

    return status;
}

int32 TM_SDLP_myAddPacketToFrame(const void *pPacket, size_t packetSize, osal_id_t sockId, const OS_SockAddr_t *pAddr)
{
    TM_SDLP_myFrameInfo_t *pFrameInfo = &TM_SDLP_myFrameInfo;
    int32                  status     = TM_SDLP_MY_SUCCESS;


    /* The SB message size fits the 16 bit lengths used by the frame and queue. */
    if (packetSize > 0xFFFF)
    {
        CFE_EVS_SendEvent(TO_PKTSIZE_ERR_EID, CFE_EVS_EventType_ERROR, "TO dropped packet of %u bytes",
                          (unsigned int)packetSize);
        return TM_SDLP_MY_SUCCESS;
    }

    /* Octets that do not fit go to the overflow queue for the next frame.
     * If the queue cannot hold them, the whole packet is dropped. */
    status = TM_SDLP_myCopyToFrameOrOverflow(pFrameInfo, (const uint8 *)pPacket, (uint16)packetSize, true);
    if (status < 0)
    {
        CFE_EVS_SendEvent(TO_PKTSIZE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "TO dropped packet of %u bytes, overflow queue full (%d)", (unsigned int)packetSize,
                          (int)status);
        return TM_SDLP_MY_SUCCESS;
    }
    status = TM_SDLP_MY_SUCCESS;

    if (pFrameInfo->freeOctets == 0)
    {
        status = TM_SDLP_myFillAndSendFrame(sockId, pAddr);
    }

    // if there is no message to foward in TLM pipe, then the mySendFrame() will be called 
    return status;
}
