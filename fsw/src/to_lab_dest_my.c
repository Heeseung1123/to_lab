/* Extra TM destinations: the same frame is sent to each address in this list. */
#include <string.h>
#include "to_lab_dest_my.h"

typedef struct
{
    bool          Used;
    char          Ip[16];
    uint16        Port;
    OS_SockAddr_t Addr;
} TO_LAB_Dest_t;

static TO_LAB_Dest_t TO_LAB_Dests[TO_LAB_MAX_EXTRA_DESTS];

/* Returns the list index of ip:port, or -1 if it is not in the list. */
static int32 TO_LAB_FindDest(const char *ip, uint16 port)
{
    int32 i;

    for (i = 0; i < TO_LAB_MAX_EXTRA_DESTS; i++)
    {
        if (TO_LAB_Dests[i].Used && (TO_LAB_Dests[i].Port == port) && (strcmp(TO_LAB_Dests[i].Ip, ip) == 0))
        {
            return i;
        }
    }
    return -1;
}

/* Reads IP and port from the command; false if the command size is wrong. */
static bool TO_LAB_ReadDestCmd(const TO_LAB_DestCmd_t *pCmd, char *ip, uint16 *port)
{
    size_t size = 0;

    CFE_MSG_GetSize(&pCmd->CmdHeader.Msg, &size);
    if (size != sizeof(TO_LAB_DestCmd_t))
    {
        CFE_EVS_SendEvent(TO_DEST_ERR_EID, CFE_EVS_EventType_ERROR, "TO: destination command size %u, expected %u",
                          (unsigned int)size, (unsigned int)sizeof(TO_LAB_DestCmd_t));
        return false;
    }
    memcpy(ip, pCmd->DestIp, 16);
    ip[15] = '\0';
    *port  = (uint16)((pCmd->Port[0] << 8) | pCmd->Port[1]);
    return true;
}

/* ADD_DEST: puts ip:port in a free slot; a repeated ADD_DEST changes nothing. */
void TO_LAB_AddDest(const TO_LAB_DestCmd_t *pCmd)
{
    char   ip[16];
    uint16 port;
    int32  i;

    if (!TO_LAB_ReadDestCmd(pCmd, ip, &port) || (TO_LAB_FindDest(ip, port) >= 0))
    {
        return;
    }
    for (i = 0; i < TO_LAB_MAX_EXTRA_DESTS; i++)
    {
        if (!TO_LAB_Dests[i].Used)
        {
            OS_SocketAddrInit(&TO_LAB_Dests[i].Addr, OS_SocketDomain_INET);
            if (OS_SocketAddrFromString(&TO_LAB_Dests[i].Addr, ip) != OS_SUCCESS)
            {
                break;
            }
            OS_SocketAddrSetPort(&TO_LAB_Dests[i].Addr, port);
            memcpy(TO_LAB_Dests[i].Ip, ip, sizeof(ip));
            TO_LAB_Dests[i].Port = port;
            TO_LAB_Dests[i].Used = true;
            CFE_EVS_SendEvent(TO_DEST_INF_EID, CFE_EVS_EventType_INFORMATION, "TO: TM also goes to %s:%u", ip,
                              (unsigned int)port);
            return;
        }
    }
    CFE_EVS_SendEvent(TO_DEST_ERR_EID, CFE_EVS_EventType_ERROR, "TO: cannot add TM destination %s:%u", ip,
                      (unsigned int)port);
}

/* REMOVE_DEST: frees the slot of ip:port. */
void TO_LAB_RemoveDest(const TO_LAB_DestCmd_t *pCmd)
{
    char   ip[16];
    uint16 port;
    int32  i;

    if (!TO_LAB_ReadDestCmd(pCmd, ip, &port))
    {
        return;
    }
    i = TO_LAB_FindDest(ip, port);
    if (i >= 0)
    {
        TO_LAB_Dests[i].Used = false;
    }
    CFE_EVS_SendEvent(TO_DEST_INF_EID, CFE_EVS_EventType_INFORMATION, "TO: TM no longer goes to %s:%u", ip,
                      (unsigned int)port);
}

/* Main address first (its error is returned), then each extra one. */
int32 TO_LAB_SendToAllDests(osal_id_t sockId, const void *pFrame, size_t size, const OS_SockAddr_t *pMainAddr)
{
    int32 status = OS_SocketSendTo(sockId, pFrame, size, pMainAddr);
    int32 i;

    for (i = 0; i < TO_LAB_MAX_EXTRA_DESTS; i++)
    {
        if (TO_LAB_Dests[i].Used)
        {
            OS_SocketSendTo(sockId, pFrame, size, &TO_LAB_Dests[i].Addr);
        }
    }
    return status;
}
