/**************************************************************************************************
*******
**************************************************************************************************/

/**************************************************************************************************
**************************************************************************************************/

#ifndef GATT_INTERNAL_H
#define GATT_INTERNAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
    INCLUDES
*/
#include "osal_cbTimer.h"

#include "att.h"
#include "gatt.h"

/*********************************************************************
    MACROS
*/
#define TIMER_VALID( id )                ( ( (id) != INVALID_TIMER_ID ) && \
                                           ( (id) != TIMEOUT_TIMER_ID ) )

#define TIMER_STATUS( id )               ( (id) == TIMEOUT_TIMER_ID ? bleTimeout : \
                                           (id) == INVALID_TIMER_ID ? SUCCESS : blePending )

/*********************************************************************
    CONSTANTS
*/

/*********************************************************************
    TYPEDEFS
*/
// Srtucture for Attribute Version Information attribute
typedef struct
{
    uint8 attVersion;        // Attribute Protocol Version
    uint8 gattVersion;       // Generic Attribute Profile Version
    uint16 manufacturerName; // Manufacturer Name
} gattVersionInfo_t;




// Function prototype to parse an attribute protocol request message
typedef bStatus_t (*gattParseReq_t)( uint8 sig, uint8 cmd, uint8* pParams, uint16 len, attMsg_t* pMsg );

// Function prototype to parse an attribute protocol response message
typedef bStatus_t (*gattParseRsp_t)( uint8* pParams, uint16 len, attMsg_t* pMsg );

// Function prototype to process an attribute protocol message
typedef bStatus_t (*gattProcessMsg_t)( uint16 connHandle,  attPacket_t* pPkt );

// Function prototype to process an attribute protocol request message
typedef bStatus_t (*gattProcessReq_t)( uint16 connHandle,  attMsg_t* pMsg );


// Structure to keep Client info
typedef struct
{
    // Info maintained for Client that expecting a response back
    uint16 connHandle;          // connection message was sent out
    uint8 method;               // type of response to be received
    gattParseRsp_t pfnParseRsp; // function to parse response to be received
    uint8 timerId;              // response timeout timer id
    uint8 taskId;               // task to be notified of response

    // GATT Request message
    gattMsg_t req;              // request message

    // Info maintained for GATT Response message
    uint8 numRsps;             // number of responses received
} gattClientInfo_t;

// Structure to keep Attribute Server info
typedef struct
{
    // Info maintained for Handle Value Confirmation message
    uint16 connHandle;    // connection message was sent on
    uint8 timerId;        // confirmation timeout timer id
    uint8 taskId;         // task to be notified of confirmation
} gattServerInfo_t;


/*********************************************************************
    VARIABLES
*/
extern uint8 gattTaskID;

/*********************************************************************
    FUNCTIONS
*/
extern void gattRegisterServer( gattProcessMsg_t pfnProcessMsg );

extern void gattRegisterClient( gattProcessMsg_t pfnProcessMsg );

extern bStatus_t gattNotifyEvent( uint8 taskId, uint16 connHandle, uint8 status,
                                  uint8 method, gattMsg_t* pMsg );

extern void gattStartTimer( pfnCbTimer_t pfnCbTimer, uint8* pData,
                            uint16 timeout, uint8* pTimerId );

extern void gattStopTimer( uint8* pTimerId );
extern uint8 gattClient_stackInitContent(uint8 max_gattClientNum,gattClientInfo_t* clientInfo);
extern uint8 gattServer_stackInitContent(uint8 max_gattServerNum,gattServerInfo_t* serverInfo);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* GATT_INTERNAL_H */
