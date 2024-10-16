#ifndef _LED_DRIVER_H 
#define _LED_DRIVER_H 
#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

typedef enum{

    led_init = 0,
    led_net_cfg,
    led_net_connect,
    led_net_con_suc,
    led_cloud_con_suc,
    led_wifi_low_power,

    led_max

}led_status_t;

void led_dirver_init(void);

void led_pro(uint16_t tick_ms);

void led_set_status(led_status_t status);

void led_set_enable(uint8_t en);

void led_hal_set_state(uint8_t state);

#ifdef __cplusplus
}
#endif
#endif	// _LED_DRIVER_H

