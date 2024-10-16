/**************************************************************************************************
*******
**************************************************************************************************/

/**************************************************************************************************
    Filename:       prop_protocol.h
    Revised:
    Revision:

    Description:    
                  definitions and prototypes.


**************************************************************************************************/

#ifndef PROPPROTOCOL_H
#define PROPPROTOCOL_H

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

#define RFPHY_RX_SCAN_ALLWAYS_ON               (0xFFFFFFFF)

// PROP PROTOCOL Task Events
#define PPP_PERIODIC_TX_EVT         0x0001
#define PPP_PERIODIC_RX_EVT         0x0002
#define PPP_TX_DONE_EVT             0x0004
#define PPP_RX_DONE_EVT             0x0008
#define PPP_TRX_DONE_EVT            0x0010
#define PPP_RX_DATA_PROCESS_EVT     0x0020
#define PPP_TX_PENDING_PROCESS_EVT  0x0040
#define PPP_RX_PENDING_PROCESS_EVT  0x0080
#define PPP_RX_DBG_EVT              0x0100

#define PPP_RFPHY_TX_ONLY                   (0x00)
#define PPP_RFPHY_RX_ONLY                   (0x01)
#define PPP_RFPHY_TRX_ONLY                  (0x02)

#define PPP_CONFIG_TX                 (1)
#define PPP_CONFIG_RX                 (2)
#define PPP_CONFIG_TRX_ALL            (3)
#define PPP_MESH_DISABLE                    (0)
#define PPP_MESH_ENABLE                     (1)
#define PPP_AUTOACK_ENABLE                  (1)
#define PPP_AUTOACK_DISABLE                 (0)
#define PPP_NRF_DISABLE                     (0)
#define PPP_NRF_ENABLE                      (1)


#define PHY_DATA_CB                 (1)
#define PHY_OPCODE_CB               (2)
#define PPP_GET_NEEDACK_BIT(x)              (x & 0x04)
#define PPP_GET_ACK_BIT(x)                  (x & 0x08)
#define PPP_GET_PID(x)                      (x & 0x03)
#define PPP_GET_OPCODE(x)                   ((x & 0xF0)>>4)
#define PPP_GET_NETID_GROUPID(x)            (x >> 8)
#define PPP_GET_NETID_DEVICEID(x)           (x & 0x00FF)

#define PPP_ACK_DATA_MAX_NUM                32
#define PPP_SMART_NRF_TYPE                  0xFF
#define PPP_STX_DONE_TYPE                   0xFC


/*********************************************************************
    MACROS
*/
typedef struct
{
    uint8_t       type;
    uint8_t*      data;
    uint8_t       len;
    uint8_t       rssi;
} phy_comm_evt_t;

typedef uint8_t (*phy_comm_cb_t)(phy_comm_evt_t* pev);

/*********************************************************************
    FUNCTIONS
*/
uint8_t phy_rf_start_rx(uint32 rxTimeOut);
uint8_t phy_rf_stop_rx(void);
uint8_t phy_rf_get_current_status(void);
uint8_t phy_adv_data_update(uint8_t* din, uint8_t dLen);
uint8_t phy_rf_start_tx(uint8_t* din, uint8_t dLen, uint32_t txintv, uint16_t targetnetid);
uint8_t phy_rf_stop_tx(void);
uint8_t phy_cbfunc_regist(int8_t cbfunc_type, uint8_t (*phy_comm_cb_t)(phy_comm_evt_t* pev));
uint8_t phy_update_syncword(uint32_t syncword);
uint8_t phy_update_chmap(uint8_t chnum, uint8_t* chmap, uint8_t* wtmap);
void phy_adv_opcode_update(uint8_t opcode);
void phy_set_tx_maxtime(uint32_t txdura);

/*
    Task Initialization for the PROP PROTOCOL Application
*/
extern void PropProtocol_Init( uint8 task_id );

/*
    Task Event Processor for the PROP PROTOCOL Application
*/
extern uint16 PropProtocol_ProcessEvent( uint8 task_id, uint16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* PROPPROTOCOL_H */
