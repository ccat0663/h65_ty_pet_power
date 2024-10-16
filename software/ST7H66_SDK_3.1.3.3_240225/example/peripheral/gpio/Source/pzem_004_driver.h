#ifndef _PZEM_004_DRIVER_H 
#define _PZEM_004_DRIVER_H 
#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

#define PZEM_DEFAULT_ADDR   0xF8
#define PZEM_BAUD_RATE      9600

#define REG_VOLTAGE     0x0000
#define REG_CURRENT_L   0x0001
#define REG_CURRENT_H   0X0002
#define REG_POWER_L     0x0003
#define REG_POWER_H     0x0004
#define REG_ENERGY_L    0x0005
#define REG_ENERGY_H    0x0006
#define REG_FREQUENCY   0x0007
#define REG_PF          0x0008
#define REG_ALARM       0x0009

#define CMD_RHR         0x03
#define CMD_RIR         0X04
#define CMD_WSR         0x06
#define CMD_CAL         0x41
#define CMD_REST        0x42

#define WREG_ALARM_THR   0x0001
#define WREG_ADDR        0x0002

#define UPDATE_TIME     200

#define RESPONSE_ALL_DATA_SIZE 			25
#define RESPONSE_CELAN_ALL_DATA_SIZE 	4

#define READ_TIMEOUT 100

#define PZEM_OK     0
#define PZEM_FAIL   -1

typedef void (*pzem_uart_send_bytes)(uint8_t *data, uint8_t len);

typedef struct {

	uint32_t voltage;      // 电压x10 V 0.1
	uint32_t current;      // 电流x1000 A 0.001
	uint32_t power;        // 功率x10 W	0.1
	uint32_t energy;       // 电能 Wh	1
	uint32_t frequency;    // 频率x10 HZ	0.1
	uint32_t pf;           // 功率因数x100	0.01
	uint16_t alarms;       // 报警状态
	
} pzem_data_t;

typedef struct {

	uint8_t addr;

    pzem_uart_send_bytes uart_send;

} pzem_driver_t;

int pzem_get_all_data_async(pzem_driver_t *pzem);

int pzem_analysis_all_data(pzem_driver_t *pzem, uint8_t *data, uint8_t len, pzem_data_t *pzem_data);

int pzem_clean_all_data_async(pzem_driver_t *pzem);

int pzem_analysis_clean_all_data(pzem_driver_t *pzem, uint8_t *data, uint8_t len, pzem_data_t *pzem_data);

#ifdef __cplusplus
}
#endif
#endif	// _PZEM_004_DRIVER_H

