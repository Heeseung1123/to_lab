/************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2002 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
** File: to_lab_sub_table.c
**
** Purpose:
**  Define TO Lab CPU specific subscription table
**
** Notes:
**
*************************************************************************/

#include "cfe_tbl_filedef.h" /* Required to obtain the CFE_TBL_FILEDEF macro definition */

#include "to_lab_sub_table.h"
#include "to_lab_app.h"

/*
** Add the proper include file for the message IDs below
*/

/*
** Common CFS app includes below are commented out
*/
#include "to_lab_msgids.h"
#include "ci_lab_msgids.h"

#include "sample_app_msgids.h"
#include "blinky_msgids.h"
// #include "cf_msgids.h"
#include "file_dl_msgids.h"
#include "msn_app_msgids.h"
// #include "relay_app_msgids.h" /* relay_app removed; file_dl relays files */

#if 0
#include "hs_msgids.h"
#include "fm_msgids.h"
#include "sc_msgids.h"
#include "ds_msgids.h"
#include "lc_msgids.h"
#endif

TO_LAB_Subs_t TO_LAB_Subs = {.Subs = {/* CFS App Subscriptions */
                                      {CFE_SB_MSGID_WRAP_VALUE(TO_LAB_HK_TLM_MID), {0, 0}, 4},
                                      /* Sent only on command; not needed. */
                                      // {CFE_SB_MSGID_WRAP_VALUE(TO_LAB_DATA_TYPES_MID), {0, 0}, 4},
                                      {CFE_SB_MSGID_WRAP_VALUE(CI_LAB_HK_TLM_MID), {0, 0}, 4},
                                      {CFE_SB_MSGID_WRAP_VALUE(CI_LAB_PQC_EP_TLM_MID), {0, 0}, 4},
                                      /* Their HK is no longer requested (sch_lab_table.c). */
                                      // {CFE_SB_MSGID_WRAP_VALUE(SAMPLE_APP_HK_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(BLINKY_HK_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(CF_HK_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(CF_EOT_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(0x0FFD), {0, 0}, 32},
                                      /* file_dl segments: one per 100 ms, so 32 leaves room for RESEND bursts. */
                                      {CFE_SB_MSGID_WRAP_VALUE(FILE_DL_SEG_TLM_MID), {0, 0}, 32},
                                      {CFE_SB_MSGID_WRAP_VALUE(MSN_APP_HK_TLM_MID), {0, 0}, 4},
                                      {CFE_SB_MSGID_WRAP_VALUE(MSN_APP_TEMP_TLM_MID), {0, 0}, 4},
                                      {CFE_SB_MSGID_WRAP_VALUE(MSN_APP_DIAG_TLM_MID), {0, 0}, 4},
                                      {CFE_SB_MSGID_WRAP_VALUE(MSN_APP_APP_STATUS_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(RELAY_APP_HK_TLM_MID), {0, 0}, 4},
                                      /* file_dl upload status: gaps to send again, and the verify result. */
                                      // {CFE_SB_MSGID_WRAP_VALUE(FILE_DL_UL_STATUS_TLM_MID), {0, 0}, 4},
                                      {CFE_SB_MSGID_WRAP_VALUE(FILE_UL_STATUS_TLM_MID), {0, 0}, 4},
                                      /* Relay: its own status and downlink segment MIDs. */
                                      {CFE_SB_MSGID_WRAP_VALUE(FILE_RELAY_STATUS_TLM_MID), {0, 0}, 4},
                                      {CFE_SB_MSGID_WRAP_VALUE(FILE_RELAY_SEG_TLM_MID), {0, 0}, 32},

#if 0
        /* Add these if needed */
        {CFE_SB_MSGID_WRAP_VALUE(HS_HK_TLM_MID), {0,0}, 4},
        {CFE_SB_MSGID_WRAP_VALUE(FM_HK_TLM_MID), {0,0}, 4},
        {CFE_SB_MSGID_WRAP_VALUE(SC_HK_TLM_MID), {0,0}, 4},
        {CFE_SB_MSGID_WRAP_VALUE(DS_HK_TLM_MID), {0,0}, 4},
        {CFE_SB_MSGID_WRAP_VALUE(LC_HK_TLM_MID), {0,0}, 4},
#endif

                                      /* cFE Core subscriptions */
                                      /* cFE core HK is no longer requested; the rest is sent only on command. */
                                      // {CFE_SB_MSGID_WRAP_VALUE(CFE_ES_HK_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(CFE_EVS_HK_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(CFE_SB_HK_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(CFE_TBL_HK_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(CFE_TIME_HK_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(CFE_TIME_DIAG_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(CFE_SB_STATS_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(CFE_TBL_REG_TLM_MID), {0, 0}, 4},
                                      /* Events are not downlinked; they still go to the console log. */
                                      // {CFE_SB_MSGID_WRAP_VALUE(CFE_EVS_LONG_EVENT_MSG_MID), {0, 0}, 32},

                                      /* Sent only on command; not needed. */
                                      // {CFE_SB_MSGID_WRAP_VALUE(CFE_ES_APP_TLM_MID), {0, 0}, 4},
                                      // {CFE_SB_MSGID_WRAP_VALUE(CFE_ES_MEMSTATS_TLM_MID), {0, 0}, 4},

                                      /* TO_UNUSED entry to mark the end of valid MsgIds */
                                      {TO_UNUSED, {0, 0}, 0}}};

CFE_TBL_FILEDEF(TO_LAB_Subs, TO_LAB_APP.TO_LAB_Subs, TO Lab Sub Tbl, to_lab_sub.tbl)
