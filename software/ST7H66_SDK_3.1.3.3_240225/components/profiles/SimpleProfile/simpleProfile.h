/**************************************************************************************************
*******
**************************************************************************************************/

/**************************************************************************************************
    Filename:       simpleGATTprofile.h
    Revised:
    Revision:

    Description:    This file contains the Simple GATT profile definitions and
                  prototypes.

 **************************************************************************************************/

#ifndef SIMPLEPROFILE_H
#define SIMPLEPROFILE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
    INCLUDES
*/

/*********************************************************************
    CONSTANTS
*/

// Profile Parameters
//#define SIMPLEPROFILE_CHAR1                   0  // RW uint8 - Profile Characteristic 1 value
//#define SIMPLEPROFILE_CHAR2                   1  // RW uint8 - Profile Characteristic 2 value
//#define SIMPLEPROFILE_CHAR3                   2  // RW uint8 - Profile Characteristic 3 value
//#define SIMPLEPROFILE_CHAR4                   3  // RW uint8 - Profile Characteristic 4 value
//#define SIMPLEPROFILE_CHAR5                   4  // RW uint8 - Profile Characteristic 4 value
//#define SIMPLEPROFILE_CHAR6                   5  // RW uint8 - Profile Characteristic 4 value

// Simple Profile Service UUID
#define SIMPLEPROFILE_SERVICE_UUID            0xFD50

// Key Pressed UUID
#define SIMPLEPROFILE_DATA_IN_UUID            0xFD52
#define SIMPLEPROFILE_DATA_OUT_UUID           0xFD54

// Simple Keys Profile Services bit fields
#define SIMPLEPROFILE_SERVICE               0x00000001

// Length of Characteristic 5 in bytes
//#define SIMPLEPROFILE_CHAR5_LEN           5
//#define IBEACON_UUID_LEN                  16

/*********************************************************************
    TYPEDEFS
*/


/*********************************************************************
    MACROS
*/

/*********************************************************************
    Profile Callbacks
*/

// Callback when a characteristic value has changed
//typedef void (*simpleProfileChange_t)( uint8 paramID );
//
//typedef struct
//{
//    simpleProfileChange_t        pfnSimpleProfileChange;  // Called when characteristic value changes
//} simpleProfileCBs_t;

typedef uint16 (* simpleProfile_data_in_wt_cb)
(
    uint16 conn_hndl,
    uint16 offset,
    uint16 length,
    uint8* value
);
typedef uint16 (* simpleProfile_data_out_ccd_cb)
(
    uint16 conn_hndl,
    uint8 enabled
);
/**
    Mesh Proxy application callbacks
*/
typedef struct
{
    /** Proxy Data IN Callback */
    simpleProfile_data_out_ccd_cb             simpleProfile_data_out_ccd_cb;

    /** Proxy Data OUT notif Changed */
    simpleProfile_data_in_wt_cb            simpleProfile_data_in_cb;
} simpleProfile_CB;


/*********************************************************************
    API FUNCTIONS
*/


/*
    SimpleProfile_AddService- Initializes the Simple GATT Profile service by registering
            GATT attributes with the GATT server.

    @param   services - services to add. This is a bit map and can
                       contain more than one service.
*/

bStatus_t SimpleProfile_InitService( simpleProfile_CB* appCallbacks);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SIMPLEGATTPROFILE_H */
