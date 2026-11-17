/* Overflow queue for TM frame segmentation, ported from NOS3 io_lib tm_sdlp.c:745-973.
 * The queue is a ring buffer between dataStart and dataEnd. */
#include "tm_sdlp_of.h"

/* Reads the packet length from a Space Packet header (length field + 7). */
static uint16 TM_SDLP_myReadSppTotalLength(const uint8 *pHdr)
{
    return (uint16)((((uint16)pHdr[4] << 8) | pHdr[5]) + TM_SDLP_MY_IDLE_MIN_LENGTH);
}

/* Appends data at dataEnd, wrapping at the buffer end (NOS3 CopyToOverflow).
 * isPartial is true when the start of the same packet is already in the frame. */
static int32 TM_SDLP_myPushToOverflow(TMTF_myOverflowInfo_t *pOverflow, const uint8 *pData, uint16 length,
                                      bool isPartial)
{
    uint16 lengthToEnd;

    if (length > pOverflow->buffSize)
    {
        return TM_SDLP_MY_INVALID_LENGTH;
    }

    /* A full queue drops the whole packet. */
    if (length > pOverflow->freeOctets)
    {
        return TM_SDLP_MY_OVERFLOW_FULL;
    }

    lengthToEnd = pOverflow->buffSize - (uint16)(pOverflow->dataEnd - pOverflow->buffer);

    if (length < lengthToEnd)
    {
        CFE_PSP_MemCpy(pOverflow->dataEnd, pData, length);
        pOverflow->dataEnd += length;
    }
    else
    {
        CFE_PSP_MemCpy(pOverflow->dataEnd, pData, lengthToEnd);
        CFE_PSP_MemCpy(pOverflow->buffer, pData + lengthToEnd, length - lengthToEnd);
        pOverflow->dataEnd = pOverflow->buffer + (length - lengthToEnd);
    }

    /* A packet that did not fit at all keeps partialOctets unchanged. */
    if (isPartial)
    {
        pOverflow->partialOctets = length;
    }

    pOverflow->freeOctets -= length;

    return TM_SDLP_MY_SUCCESS;
}

/* Moves the partial octets, or else one whole packet, from dataStart to the frame.
 * Only the free frame octets are moved; partialOctets keeps the rest (NOS3 CopyFromOverflow).
 * Returns the number of octets moved. */
static uint16 TM_SDLP_myPopOverflowToFrame(TM_SDLP_myFrameInfo_t *pFrameInfo)
{
    TMTF_myOverflowInfo_t *pOverflow = &pFrameInfo->overflowInfo;
    uint8                  msgHdr[TM_SDLP_MY_SPP_HDR_LENGTH];
    uint16                 lengthToCopy;
    uint16                 lengthToEnd;
    uint16                 freeOctets = pFrameInfo->freeOctets;
    bool                   setHeader;

    lengthToEnd = pOverflow->buffSize - (uint16)(pOverflow->dataStart - pOverflow->buffer);

    if (pOverflow->partialOctets > 0)
    {
        lengthToCopy = pOverflow->partialOctets;
        setHeader    = false;
    }
    else
    {
        /* The header itself may wrap, so it is joined in a local copy first. */
        if (lengthToEnd < TM_SDLP_MY_SPP_HDR_LENGTH)
        {
            CFE_PSP_MemCpy(msgHdr, pOverflow->dataStart, lengthToEnd);
            CFE_PSP_MemCpy(msgHdr + lengthToEnd, pOverflow->buffer, TM_SDLP_MY_SPP_HDR_LENGTH - lengthToEnd);
            lengthToCopy = TM_SDLP_myReadSppTotalLength(msgHdr);
        }
        else
        {
            lengthToCopy = TM_SDLP_myReadSppTotalLength(pOverflow->dataStart);
        }
        setHeader = true;
    }

    if (freeOctets < lengthToCopy)
    {
        if (setHeader)
        {
            pOverflow->partialOctets = lengthToCopy - freeOctets;
        }
        else
        {
            pOverflow->partialOctets -= freeOctets;
        }
        lengthToCopy = freeOctets;
    }
    else if (!setHeader)
    {
        pOverflow->partialOctets = 0;
    }

    /* The frame has room for lengthToCopy, so these calls never queue again. */
    if (lengthToEnd > lengthToCopy)
    {
        (void)TM_SDLP_myCopyToFrameOrOverflow(pFrameInfo, pOverflow->dataStart, lengthToCopy, setHeader);
        pOverflow->dataStart += lengthToCopy;
    }
    else
    {
        (void)TM_SDLP_myCopyToFrameOrOverflow(pFrameInfo, pOverflow->dataStart, lengthToEnd, setHeader);
        (void)TM_SDLP_myCopyToFrameOrOverflow(pFrameInfo, pOverflow->buffer, lengthToCopy - lengthToEnd, setHeader);
        pOverflow->dataStart = pOverflow->buffer + (lengthToCopy - lengthToEnd);
    }

    pOverflow->freeOctets += lengthToCopy;

    return lengthToCopy;
}

void TM_SDLP_myInitOverflowQueue(TMTF_myOverflowInfo_t *pOverflow, uint8 *pBuffer, uint16 buffSize)
{
    pOverflow->buffSize      = buffSize;
    pOverflow->freeOctets    = buffSize;
    pOverflow->partialOctets = 0;
    pOverflow->dataStart     = pBuffer;
    pOverflow->dataEnd       = pBuffer;
    pOverflow->buffer        = pBuffer;

    CFE_PSP_MemSet(pBuffer, 0, buffSize);
}

int32 TM_SDLP_myCopyToFrameOrOverflow(TM_SDLP_myFrameInfo_t *pFrameInfo, const uint8 *pData, uint16 dataLength, bool isPacket)
{
    uint16 lengthToCopy = dataLength;
    int32  status;

    /* Queue the octets that do not fit before anything is copied to the frame. */
    if (pFrameInfo->freeOctets < lengthToCopy)
    {
        lengthToCopy = pFrameInfo->freeOctets;

        status = TM_SDLP_myPushToOverflow(&pFrameInfo->overflowInfo, pData + lengthToCopy, dataLength - lengthToCopy,
                                          (lengthToCopy > 0));
        if (status < 0)
        {
            return status;
        }
    }

    CFE_PSP_MemCpy((uint8 *)pFrameInfo->frame + pFrameInfo->currentDataOffset, pData, lengthToCopy);
    pFrameInfo->freeOctets -= lengthToCopy;

    /* The First Header Pointer marks the first packet that starts in this frame. */
    if (isPacket && (lengthToCopy > 0) && !pFrameInfo->isFirstHdrPtrSet)
    {
        TMTF_MY_WR_FIRSTHDRPTR(*pFrameInfo->frame, pFrameInfo->currentDataOffset - pFrameInfo->dataFieldOffset);
        pFrameInfo->isFirstHdrPtrSet = true;
    }
    pFrameInfo->currentDataOffset += lengthToCopy;

    return (int32)pFrameInfo->freeOctets;
}

void TM_SDLP_myRefillFrameFromOverflow(TM_SDLP_myFrameInfo_t *pFrameInfo)
{
    TMTF_myOverflowInfo_t *pOverflow    = &pFrameInfo->overflowInfo;
    uint16                 lengthToCopy = pOverflow->buffSize - pOverflow->freeOctets;

    /* Only as much as the data field can take is moved now. */
    if (lengthToCopy > pFrameInfo->dataFieldLength)
    {
        lengthToCopy = pFrameInfo->dataFieldLength;
    }

    while ((lengthToCopy > 0) && (pFrameInfo->freeOctets > 0))
    {
        lengthToCopy -= TM_SDLP_myPopOverflowToFrame(pFrameInfo);
    }
}
