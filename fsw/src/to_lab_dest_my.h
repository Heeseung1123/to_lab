/* Extra TM destinations. Every TM frame goes to the OUTPUT_ENABLE address and,
 * in the same way, to each address added here, like one RF downlink that
 * several ground stations receive. */
#ifndef TO_LAB_DEST_MY_H
#define TO_LAB_DEST_MY_H

#include "cfe.h"

#define TO_ADD_DEST_CC    7 /* add a TM destination (IP and port) */
#define TO_REMOVE_DEST_CC 8 /* remove a TM destination */

#define TO_LAB_MAX_EXTRA_DESTS 4

#define TO_DEST_INF_EID 22
#define TO_DEST_ERR_EID 23

/* Payload of ADD_DEST and REMOVE_DEST: 16 B IP text, 2 B port (big-endian). */
typedef struct
{
    CFE_MSG_CommandHeader_t CmdHeader;
    char                    DestIp[16];
    uint8                   Port[2];
} TO_LAB_DestCmd_t;

/* ADD_DEST: adds the IP and port to the list (no change if already there). */
void TO_LAB_AddDest(const TO_LAB_DestCmd_t *pCmd);

/* REMOVE_DEST: removes the IP and port from the list. */
void TO_LAB_RemoveDest(const TO_LAB_DestCmd_t *pCmd);

/* Sends one frame to the main address, then to every extra one.
 * Only an error on the main address is returned. */
int32 TO_LAB_SendToAllDests(osal_id_t sockId, const void *pFrame, size_t size, const OS_SockAddr_t *pMainAddr);

#endif /* TO_LAB_DEST_MY_H */
