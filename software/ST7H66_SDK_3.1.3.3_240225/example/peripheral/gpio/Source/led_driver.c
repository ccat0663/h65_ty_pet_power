
#include "gpio.h"
#include "led_driver.h"

#define LED_IO  P0

#define LED_NET_CFG_BLINK_MS    200
#define LED_NET_CONNECTING_BLINK_MS 500
#define LED_NET_CON_SUC_BLINK_MS 1000
#define LED_WIFI_CLOUD_CON_SUC_BLINK_MS 6000

static led_status_t led_status = led_init;

static uint16_t led_cnt_ms = 0;

static uint8_t led_enable = 1;

typedef struct
{
    uint16_t blink_ms;  //闪烁时间  0xffff:常亮 0:长灭
}led_blink_t;


static const led_blink_t blink_ctr[led_max] = 
{
    {0xffff},
    {LED_NET_CFG_BLINK_MS},
    {LED_NET_CONNECTING_BLINK_MS},
    {LED_NET_CON_SUC_BLINK_MS},
    {LED_WIFI_CLOUD_CON_SUC_BLINK_MS},
    {0},
};

void led_dirver_init(void)
{
    hal_gpio_pin_init(LED_IO, GPIO_OUTPUT);
    hal_gpio_pull_set(LED_IO, GPIO_PULL_UP_S);
    hal_gpio_fast_write(LED_IO, 1);

    led_status = led_init;
}

void led_pro(uint16_t tick_ms)
{
    led_cnt_ms += tick_ms;

    if(led_enable == 0)
    {
        return;
    }

    if(blink_ctr[led_status].blink_ms == 0)
    {
        hal_gpio_fast_write(LED_IO, 1);
    }
    else if(blink_ctr[led_status].blink_ms == 0xffff)
    {
        hal_gpio_fast_write(LED_IO, 0);
    }
    else if(blink_ctr[led_status].blink_ms > 0)
    {
        if(led_cnt_ms >= (blink_ctr[led_status].blink_ms * 2))
        {
            led_cnt_ms = 0;
            hal_gpio_fast_write(LED_IO, 1);
        }
        else if(led_cnt_ms >= blink_ctr[led_status].blink_ms)
        {
            hal_gpio_fast_write(LED_IO, 0);
        }
    }
    
}

void led_set_status(led_status_t status)
{
    if(led_status != status)
    {
        led_cnt_ms = 0;
        led_status = status;
    }
}

void led_set_enable(uint8_t en)
{
    led_enable = en;
}

void led_hal_set_state(uint8_t state)
{
    hal_gpio_fast_write(LED_IO, state);
}
