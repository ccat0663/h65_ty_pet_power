/**************************************************************************************************
*******
**************************************************************************************************/

/**************************************************************************************************
    Filename:       ble_at.h
    Revised:
    Revision:

    Description:    This file contains the at ble sample application


**************************************************************************************************/

/*********************************************************************
    INCLUDES
*/
#ifndef __BLE_AT_H
#define __BLE_AT_H

#define AT_START_DEVICE_EVT                         0x0001
#define AT_PROCESS_UART_RX_CMD_EVT                  0x0040
#define AT_ENTER_ATUO_SLEEP_MODE_EVT                0x0080

extern uint8 bleAT_TaskID;   // Task ID for internal task/event processing

void bleAT_Init( uint8 task_id );
uint16 bleAT_ProcessEvent( uint8 task_id, uint16 events );
void ble_at_uart_init(void);


#endif

