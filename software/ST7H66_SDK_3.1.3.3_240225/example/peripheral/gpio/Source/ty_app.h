
#ifndef _TY_APP_H 
#define _TY_APP_H 
#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

#define TY_UART_REC_DATA 		0x0001
#define TY_KEY_LONG_EVENT       0x0002
#define TY_PZEM_PRO_EVENT       0x0004
#define TY_SEC_TICK_EVENT       0x0008

#define TY_TEMP_HUMI_UPDATA_EVENT      0x0010
#define TY_REAL_POWER_UPDATA_EVENT     0x0020
#define TY_DAY_ENERGY_UPDATA_EVENT     0x0040
#define TY_WARN_PUSH_UPDATA_EVENT      0x0080

#define TY_OTA_DONE_REBOOT_EVENT       0x0100
#define TY_SEND_RESET_DP_EVENT         0x0200
#define TY_CONNECT_CLOUD_UPDATA_ALL_DATA_EVENT         0x0400

void ty_app_init(uint8_t task_id);

uint16_t ty_app_pro_event(uint8_t task_id, uint16_t events);

void ty_uart_send_byte(uint8_t data);


#ifdef __cplusplus
}
#endif
#endif	// _TY_APP_H

