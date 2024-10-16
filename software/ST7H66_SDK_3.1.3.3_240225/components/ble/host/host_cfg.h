/**************************************************************************************************
*******
**************************************************************************************************/

/**************************************************************************************************
**************************************************************************************************/

#ifndef HOST_CFG_H
#define HOST_CFG_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
    INCLUDES
*/
#include "sm.h"
#include "linkdb.h"
#include "l2cap_internal.h"
#include "sm_internal.h"
#include "gap_internal.h"
#include "att_internal.h"
#include "gatt_internal.h"

/*********************************************************************
    MACROS
*/

/*********************************************************************
    CONSTANTS
*/

/*********************************************************************
    TYPEDEFS
*/

/*********************************************************************
    GLOBAL VARIABLES
*/
// This is the link database, 1 record for each connection
static linkDBItem_t glinkDB[MAX_NUM_LL_CONN];

// Table of callbacks to make when a connection changes state
static pfnLinkDBCB_t glinkCBs[MAX_NUM_LL_CONN+LINKDB_STACK_CALLBACK_NUM];
static smPairingParams_t* smPairingParam[MAX_NUM_LL_CONN];
static uint16 gMTU_Size[MAX_NUM_LL_CONN];
gapAuthStateParams_t* gAuthenLink[MAX_NUM_LL_CONN];
l2capReassemblePkt_t l2capReassembleBuf[MAX_NUM_LL_CONN];
l2capSegmentBuff_t   l2capSegmentBuf[MAX_NUM_LL_CONN];
gattClientInfo_t    gattClientInfo[GATT_MAX_NUM_CONN];
gattServerInfo_t    gattServerInfo[GATT_MAX_NUM_CONN];

/*********************************************************************
    FUNCTIONS
*/
extern uint8 Host_InitContext(       uint8 max_link_num,
                                     linkDBItem_t* plinkDB,pfnLinkDBCB_t* plinkCBs,
                                     smPairingParams_t** param,
                                     uint16* mtu_size_buf,
                                     gapAuthStateParams_t** Authen_link,
                                     l2capReassemblePkt_t* ressembleBuf,l2capSegmentBuff_t* segmentBuf,
                                     gattClientInfo_t* clientInfo,
                                     gattServerInfo_t* serverInfo);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* HOST_CFG_H */
