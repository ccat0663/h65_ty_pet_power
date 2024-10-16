
#ifndef _DHT11_DRIVER_H 
#define _DHT11_DRIVER_H 
#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdio.h"


#define DHT_GPIO_MODE_OUT   1
#define DHT_GPIO_MODE_IN    0

#define DHT_GPIO_STATE_HIGHT   1
#define DHT_GPIO_STATE_LOW     0


typedef void (*dht_gpio_init_fun)(uint8_t mode);
typedef void (*dht_gpio_set_fun)(uint8_t state);
typedef uint8_t (*dht_gpio_get_fun)(void);

typedef struct
{
    dht_gpio_init_fun gpio_init;
    dht_gpio_set_fun gpio_set;
    dht_gpio_get_fun gpio_get;

    void (*delay_us)(uint32_t us);
    
}dht_driver_ops_t;

typedef struct
{
    uint16_t temp; // temp x 100

    uint16_t humi; // humi x 100
    
}dht_driver_data_t;

void dht_driver_init(dht_driver_ops_t *ops);

uint8_t dht_get_temp_humi_data(dht_driver_data_t *data);


#ifdef __cplusplus
}
#endif
#endif	// _DHT11_DRIVER_H

