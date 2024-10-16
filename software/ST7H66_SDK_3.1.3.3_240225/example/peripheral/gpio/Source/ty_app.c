
#include "stdlib.h"

#include "gpio.h"
#include "uart.h"
#include "clock.h"

#include "ty_app.h"
#include "wifi.h"
#include "OSAL.h"
#include "log.h"
#include "watchdog.h"
#include "osal_snv.h"

#include "led_driver.h"
#include "key_driver.h"
#include "pzem_004_driver.h"
#include "dht11_driver.h"
#include "ota_flash_port.h"

//ty power IO
#define TY_POWER_IO P17

#define USER_TEMP_SENSOR    0   // 是否使能温湿度传感器，目前有问题，会导致看门狗复位
#define DHT_DATA_IO P14

#define TY_APP_RELOAD_TIME 10

#define TY_APP_OTA_RELOAD_TIME 1

#define TY_PZEM_NUM 3
#define TY_PZEM_PRO_TIME_MS    50
#define TY_SEC_TICK_TIME_MS    1000
#define TY_PZEM_REC_DATA_TIME_MS    200
#define TY_PZEM_DEALY_TIME_MS   2000

#define TY_TEMP_CHANGE_LIMIT    200     // 2度
#define TY_HUMI_CHANGE_LIMIT    2       // 2%
#define TY_TEMP_HUMI_UPDATA_TIME_MS    (5 * 60 * 1000)

#define TY_REAL_POWER_CHANGE_LIMIT     5 // 5w
#define TY_REAL_POWER_UPDATA_TIME_MS   (10 * 60 * 1000)

#define TY_DAY_ENERGY_UPDATA_TIME_MS   (10 * 60 * 1000)

#define TY_OTA_TIME_OUT_MS             (3 * 60 * 1000)

#define TY_UPDATA_REBOOT_TIME 200

#define TY_WARN_PUSH_INTERVAL_SEC      (60)

enum{
    SNV_ID_WARN_SWITCH = 30,
    SNV_ID_WARN_VALUE,
    SNV_ID_DAY_ENERGY,
    SNV_ID_TOTAL_ENEGRY,
};

// 涂鸦上报告警
enum{
    FIRST_POWER_WARN = 0,
    SEC_POWER_WARN,
    THIRD_POWER_WARN,
    TEMP_WARN,
    HUMI_WARN,
    TEMP_SENSOR_ERROR,
    FIRST_SENSOR_ERROR,
    SEC_SENSOR_ERROR,
    THIRD_SENSOR_ERROR,

    TY_WARN_MAX
};

typedef struct
{
    uint8_t is_vail;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t week;

}ty_time_t;

typedef struct
{
    uint8_t warn_switch[TY_PZEM_NUM];

    uint8_t temp_humi_warn_switch;

}snv_warn_switch_t;

typedef struct
{
    uint32_t warn_value[TY_PZEM_NUM];

    uint8_t temp_warn_value;
    uint8_t humi_warn_value;

}snv_warn_value_t;

typedef struct
{
    ty_time_t save_time;
    uint32_t total_enegry[TY_PZEM_NUM];

}snv_total_enegry_t;

typedef struct
{
    uint8_t warn_switch;

    uint32_t power_warn_value;

    uint32_t real_power;

    uint32_t day_energy;

    uint32_t total_energy;

}ty_pzem_data_t;

typedef struct
{
    uint8_t temp_humi_warn_switch;

    uint8_t temp_warn_value;
    uint8_t humi_warn_value;

    uint16_t temp; // temp x 100

    uint16_t humi; 

    uint32_t warn_push;

    ty_pzem_data_t pzem[TY_PZEM_NUM];

}ty_report_data_t;

typedef struct
{
    uint8_t data[128];

    uint8_t data_cnt;

}uart_buf_t;

typedef void (*pzem_change_uart)(uint8_t pzem_ctr_index);

enum{
    // 读取数据
    TY_PZEM_CTR_INIT = 0,
    TY_PZEM_CTR_CHANGE_UART,
    TY_PZEM_CTR_SEND_DATA,
    TY_PZEM_CTR_REC_DATA,
    TY_PZEM_CTR_READ_TEMP_HUMI,
    TY_PZEM_CTR_DELAY,

    // 清除数据
    TY_PZEM_CELAN_CTR_INIT,
    TY_PZEM_CELAN_CTR_CHANGE_UART,
    TY_PZEM_CELAN_CTR_SEND_DATA,
    TY_PZEM_CELAN_CTR_REC_DATA,
};

typedef struct {

    gpio_pin_e tx_pin;
    gpio_pin_e rx_pin;

	pzem_driver_t pzem_drv;

    pzem_data_t pzem_data;

    uint8_t err_cnt;

} ty_pzem_drv_t;

typedef struct {

    uint8_t pzem_ctr_index;
    uint8_t step;
    uint8_t step_cnt;
    uint8_t clean_ctr_index_bit;

    ty_pzem_drv_t ty_pzem_drv[TY_PZEM_NUM];

    pzem_change_uart change_uart;

} ty_pzem_ctr_t;

static void ty_pzem_send_data(uint8_t *data, uint8_t len);
static void ty_pzem_change_uart(uint8_t pzem_ctr_index);

static ty_time_t ty_time = {0};

static uint8_t ty_task_id = 0;

static uint8_t ota_is_start = 0;

static ty_report_data_t ty_report_data = {0};

static snv_total_enegry_t last_day_total_enegry = {0};

static uint8_t wara_interval_cnt[TY_WARN_MAX] = {0};

extern uint32_t g_system_reset_cause;

static ty_pzem_ctr_t pzem_dev = 
{
    .pzem_ctr_index = 0,
    .step = TY_PZEM_CTR_INIT,
    .step_cnt = 0,
    .change_uart = ty_pzem_change_uart,
    .clean_ctr_index_bit = 0,

    .ty_pzem_drv[0] = {
        .tx_pin = P33,
        .rx_pin = P34,
        .pzem_data = {0},
        .err_cnt = 0,
        .pzem_drv = {
            .addr = PZEM_DEFAULT_ADDR,
            .uart_send = ty_pzem_send_data
        }
    },
    .ty_pzem_drv[1] = {
        .tx_pin = P32,
        .rx_pin = P31,
        .pzem_data = {0},
        .err_cnt = 0,
        .pzem_drv = {
            .addr = PZEM_DEFAULT_ADDR,
            .uart_send = ty_pzem_send_data
        }
    },
    .ty_pzem_drv[2] = {
        .tx_pin = P26,
        .rx_pin = P25,
        .pzem_data = {0},
        .err_cnt = 0,
        .pzem_drv = {
            .addr = PZEM_DEFAULT_ADDR,
            .uart_send = ty_pzem_send_data
        }
    },
};

static void dht_gpio_init(uint8_t mode)
{
    if(mode == DHT_GPIO_MODE_OUT)
    {
        hal_gpio_pin_init(DHT_DATA_IO, GPIO_OUTPUT);
        hal_gpio_pull_set(DHT_DATA_IO, GPIO_PULL_UP_S);
    }
    else
    {
        hal_gpio_pin_init(DHT_DATA_IO, GPIO_INPUT);
        hal_gpio_pull_set(DHT_DATA_IO, GPIO_PULL_UP_S);
    }
}

static void dht_gpio_set(uint8_t state)
{
    if(state == DHT_GPIO_STATE_HIGHT)
    {
        hal_gpio_fast_write(DHT_DATA_IO, 1);
    }
    else
    {
        hal_gpio_fast_write(DHT_DATA_IO, 0);
    }
}

static uint8_t dht_gpio_get(void)
{
    return hal_gpio_read(DHT_DATA_IO);
}

static dht_driver_ops_t dht_ops = 
{
    .delay_us = WaitUs,

    .gpio_init = dht_gpio_init,
    .gpio_set = dht_gpio_set,
    .gpio_get = dht_gpio_get
};

static uart_buf_t uart_rx_buf = {0};

static void ty_uart_init(uart_Hdl_t evt_handler)
{
    uart_Cfg_t cfg_user =
    {
        .tx_pin = P18,
        .rx_pin = P20,

        .rts_pin = GPIO_DUMMY,
        .cts_pin = GPIO_DUMMY,
        .baudrate = 115200,
        .use_fifo = TRUE,
        .hw_fwctrl = FALSE,
        .use_tx_buf = FALSE,
        .parity = FALSE,
        .evt_handler = evt_handler,
    };

    hal_uart_init(cfg_user, UART1); // uart init

    //power on
    hal_gpio_pin_init(TY_POWER_IO, GPIO_OUTPUT);
    hal_gpio_pull_set(TY_POWER_IO, GPIO_PULL_UP_S);
    hal_gpio_fast_write(TY_POWER_IO, 1);
}

void ty_uart_send_byte(uint8_t data)
{
    hal_uart_send_byte(UART1, data);
}

static void ty_pzem_uart_event_handle(uart_Evt_t* pev)
{
    if(pev == NULL)
    {
        return;
    }

    if(pev->len > 0)
    {
        memcpy(&uart_rx_buf.data[uart_rx_buf.data_cnt], pev->data, pev->len);

        uart_rx_buf.data_cnt += pev->len;
    }
}

static void ty_pzem_change_uart(uint8_t pzem_ctr_index)
{
    uart_Cfg_t cfg_user =
    {
        .tx_pin = pzem_dev.ty_pzem_drv[pzem_ctr_index].tx_pin,
        .rx_pin = pzem_dev.ty_pzem_drv[pzem_ctr_index].rx_pin,

        .rts_pin = GPIO_DUMMY,
        .cts_pin = GPIO_DUMMY,
        .baudrate = 9600,
        .use_fifo = TRUE,
        .hw_fwctrl = FALSE,
        .use_tx_buf = FALSE,
        .parity = FALSE,
        .evt_handler = ty_pzem_uart_event_handle,
    };

    hal_uart_deinit(UART0);

    hal_uart_init(cfg_user, UART0); // uart init
}

static void ty_pzem_send_data(uint8_t *data, uint8_t len)
{
    if(data)
    {
        hal_uart_send_buff(UART0, data, len);
    }
}

static void ty_uart_event_handle(uart_Evt_t* pev)
{
    if(pev == NULL)
    {
        return;
    }

    if(pev->len > 0)
    {
        uart_receive_buff_input(pev->data, pev->len);
    }
}

static void ty_key_event_cb(key_event_t *e)
{
    if(e->event == KEY_EVENT_LONG)
    {
        osal_set_event(ty_task_id, TY_KEY_LONG_EVENT);
    }
}

static void ty_dht_humi_temp_updata(uint16_t temp, uint16_t humi)
{
#if (USER_TEMP_SENSOR == 1)
    mcu_dp_value_update(DPID_TEMP_CURRENT, temp);
    mcu_dp_value_update(DPID_HUMIDITY_VALUE, humi);
#endif
}

static void ty_warn_switch_updata(uint8_t ch, uint8_t warn_switch)
{
    switch(ch)
    {
        case 0:

            mcu_dp_bool_update(DPID_FIRST_WARN_SWITCH, warn_switch);

            break;

        case 1:

            mcu_dp_bool_update(DPID_SECOND_WARN_SWITCH, warn_switch);

            break;

        case 2:

            mcu_dp_bool_update(DPID_THIRD_WARN_SWITCH, warn_switch);

            break;
    }
}

static void ty_temp_warn_switch_updata(uint8_t sw)
{
    mcu_dp_bool_update(DPID_TEMP_WARN_SWITCH, sw);
}

static void ty_warn_push_updata(uint32_t warn_push_flush, uint32_t warn_bit)
{
    if(warn_push_flush & (1 << FIRST_POWER_WARN))
    {
        if(warn_bit & (1 << FIRST_POWER_WARN))
        {
            mcu_dp_bool_update(DPID_FIRST_POWER_WARN, 1);
        }
        else
        {
            mcu_dp_bool_update(DPID_FIRST_POWER_WARN, 0);
        }
    }

    if(warn_push_flush & (1 << SEC_POWER_WARN))
    {
        if(warn_bit & (1 << SEC_POWER_WARN))
        {
            mcu_dp_bool_update(DPID_SEC_POWER_WARN, 1);
        }
        else
        {
            mcu_dp_bool_update(DPID_SEC_POWER_WARN, 0);
        }
    }

    if(warn_push_flush & (1 << THIRD_POWER_WARN))
    {
        if(warn_bit & (1 << THIRD_POWER_WARN))
        {
            mcu_dp_bool_update(DPID_THIRD_POWER_WARN, 1);
        }
        else
        {
            mcu_dp_bool_update(DPID_THIRD_POWER_WARN, 0);
        }
    }

    if(warn_push_flush & ((1 << TEMP_WARN) | (1 << HUMI_WARN)))
    {
        if(warn_bit & ((1 << TEMP_WARN) | (1 << HUMI_WARN)))
        {
            mcu_dp_bool_update(DPID_TEMP_HUMI_WARN, 1);
        }
        else
        {
            mcu_dp_bool_update(DPID_TEMP_HUMI_WARN, 0);
        }
    }

    if(warn_push_flush & (1 << TEMP_SENSOR_ERROR))
    {
        if(warn_bit & (1 << TEMP_SENSOR_ERROR))
        {
            mcu_dp_bool_update(DPID_TEMP_SENSOR_ERROR, 1);
        }
        else
        {
            mcu_dp_bool_update(DPID_TEMP_SENSOR_ERROR, 0);
        }
    }

    if(warn_push_flush & (1 << FIRST_SENSOR_ERROR))
    {
        if(warn_bit & (1 << FIRST_SENSOR_ERROR))
        {
            mcu_dp_bool_update(DPID_FIRST_SENSOR_ERROR, 1);
        }
        else
        {
            mcu_dp_bool_update(DPID_FIRST_SENSOR_ERROR, 0);
        }
    }

    if(warn_push_flush & (1 << SEC_SENSOR_ERROR))
    {
        if(warn_bit & (1 << SEC_SENSOR_ERROR))
        {
            mcu_dp_bool_update(DPID_SEC_SENSOR_ERROR, 1);
        }
        else
        {
            mcu_dp_bool_update(DPID_SEC_SENSOR_ERROR, 0);
        }
    }

    if(warn_push_flush & (1 << THIRD_SENSOR_ERROR))
    {
        if(warn_bit & (1 << THIRD_SENSOR_ERROR))
        {
            mcu_dp_bool_update(DPID_THIRD_SENSOR_ERROR, 1);
        }
        else
        {
            mcu_dp_bool_update(DPID_THIRD_SENSOR_ERROR, 0);
        }
    }
}

static void ty_real_power_updata(uint8_t ch, uint32_t power)
{
    switch(ch)
    {
        case 0:

            mcu_dp_value_update(DPID_FIRST_REAL_POWER, power);

            break;

        case 1:

            mcu_dp_value_update(DPID_SECOND_REAL_POWER, power);

            break;

        case 2:

            mcu_dp_value_update(DPID_THIRD_REAL_POWER, power);

            break;
    }
}

static void ty_three_real_power_updata(uint32_t value)
{
    mcu_dp_value_update(DPID_TOTAL_REAL_POWER, value);
}

static void ty_power_warn_value_updata(uint8_t ch, uint32_t value)
{
    switch(ch)
    {
        case 0:

            mcu_dp_value_update(DPID_FIRST_WARN_VALUE_SET, value);

            break;

        case 1:

            mcu_dp_value_update(DPID_SECOND_WARN_VALUE_SET, value);

            break;

        case 2:

            mcu_dp_value_update(DPID_THIRD_WARN_VALUE_SET, value);

            break;
    }
}

static void ty_temp_humi_warn_value_updata(uint8_t temp, uint8_t humi)
{
    mcu_dp_value_update(DPID_TEMP_WARN_VALUE_SET, temp);
    mcu_dp_value_update(DPID_HUMI_WARN_VALUE_SET, humi);
}

static void ty_day_energy_updata(uint8_t ch, uint32_t value)
{
    switch(ch)
    {
        case 0:

            mcu_dp_value_update(DPID_FIRST_DAY_ELE_ENERGY, value);

            break;

        case 1:

            mcu_dp_value_update(DPID_SENCOND_DAY_ELE_ENERGY, value);

            break;

        case 2:

            mcu_dp_value_update(DPID_THIRD_DAY_ELE_ENERGY, value);

            break;
    }
}

static void ty_day_three_energy_updata(uint32_t value)
{
    mcu_dp_value_update(DPID_DAY_THREE_ELE_ENERGY, value);
}

static void ty_total_energy_updata(uint8_t ch, uint32_t value)
{
    switch(ch)
    {
        case 0:

            mcu_dp_value_update(DPID_FIRST_TOTAL_ELE_ENERGY, value);

            break;

        case 1:

            mcu_dp_value_update(DPID_SECOND_TOTAL_ELE_ENERGY, value);

            break;

        case 2:

            mcu_dp_value_update(DPID_THIRD_TOTAL_ELE_ENERGY, value);

            break;
    }
}

static void ty_total_three_energy_updata(uint32_t value)
{
    mcu_dp_value_update(DPID_TOTAL_THREE_ELE_ENERGY, value);
}

static void ty_reset_reason_updata(uint32_t reason)
{
    mcu_dp_value_update(DPID_SYS_RESET, reason);
}

static void ty_report_wara_push_interval_tick(void)
{
    for (uint8_t i = 0; i < TY_WARN_MAX; i++)
    {
        if(wara_interval_cnt[i] > 0)
        {
            wara_interval_cnt[i]--;
        }
    }
}

static void ty_report_warn_push_set(uint16_t warn)
{
    // 未达到报警间隔时间，不给报警
    if(wara_interval_cnt[warn] > 0)
    {
        return;
    }

    ty_report_data.warn_push |= (1 << warn);

    wara_interval_cnt[warn] = TY_WARN_PUSH_INTERVAL_SEC;
}

static void ty_report_warn_push_reset(uint16_t warn)
{
    ty_report_data.warn_push &= (~(1 << warn));
}

void ty_all_data_updata(void)
{
    ty_dht_humi_temp_updata(ty_report_data.temp, ty_report_data.humi);
    ty_warn_push_updata(0xffffffff, ty_report_data.warn_push);
    ty_temp_warn_switch_updata(ty_report_data.temp_humi_warn_switch);
    ty_temp_humi_warn_value_updata(ty_report_data.temp_warn_value, ty_report_data.humi_warn_value);
    ty_day_three_energy_updata(ty_report_data.pzem[0].day_energy + ty_report_data.pzem[1].day_energy + ty_report_data.pzem[2].day_energy);
    ty_total_three_energy_updata(ty_report_data.pzem[0].total_energy + ty_report_data.pzem[1].total_energy + ty_report_data.pzem[2].total_energy);
    ty_three_real_power_updata(ty_report_data.pzem[0].real_power + ty_report_data.pzem[1].real_power + ty_report_data.pzem[2].real_power);

    for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
    {
        ty_warn_switch_updata(i, ty_report_data.pzem[i].warn_switch);
        ty_real_power_updata(i, ty_report_data.pzem[i].real_power);
        ty_power_warn_value_updata(i, ty_report_data.pzem[i].power_warn_value);
        ty_day_energy_updata(i, ty_report_data.pzem[i].day_energy);
        ty_total_energy_updata(i, ty_report_data.pzem[i].total_energy);
    }
}

void ty_set_rtc_time(uint8_t *buf)
{
    if(buf == NULL)
    {
        return;
    }

    if(ty_time.is_vail == 0)
    {
        osal_start_timerEx(ty_task_id, TY_SEND_RESET_DP_EVENT, 1000);
    }

    ty_time.is_vail = 1;
    ty_time.year = buf[0];
    ty_time.month = buf[1];
    ty_time.day = buf[2];
    ty_time.hour = buf[3];
    ty_time.min = buf[4];
    ty_time.sec = buf[5];
    ty_time.week = buf[6];

    LOG("ty set time %d-%d-%d %d:%d:%d\n", ty_time.year, ty_time.month, ty_time.day,
                                           ty_time.hour, ty_time.min, ty_time.sec);
}

void ty_set_warn_switch(uint8_t ch, uint8_t set)
{
    snv_warn_switch_t warn_switch = {0};

    ty_report_data.pzem[ch].warn_switch = set;

    for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
    {
        warn_switch.warn_switch[i] = ty_report_data.pzem[i].warn_switch;
    }

    osal_snv_write(SNV_ID_WARN_SWITCH, sizeof(snv_warn_switch_t), &warn_switch);
}

void ty_set_temp_humi_warn_switch(uint8_t set)
{
    snv_warn_switch_t warn_switch = {0};

    ty_report_data.temp_humi_warn_switch = set;
    warn_switch.temp_humi_warn_switch = set;

    for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
    {
        warn_switch.warn_switch[i] = ty_report_data.pzem[i].warn_switch;
    }

    osal_snv_write(SNV_ID_WARN_SWITCH, sizeof(snv_warn_switch_t), &warn_switch);
}

void ty_set_warn_value(uint8_t ch, uint32_t set)
{
    snv_warn_value_t warn_value = {0};

    ty_report_data.pzem[ch].power_warn_value = set;

    for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
    {
        warn_value.warn_value[i] = ty_report_data.pzem[i].power_warn_value;
    }

    osal_snv_write(SNV_ID_WARN_VALUE, sizeof(snv_warn_value_t), &warn_value);
}

void ty_set_temp_warn_value(uint8_t set)
{
    snv_warn_value_t warn_value = {0};

    ty_report_data.temp_warn_value = set;
    warn_value.temp_warn_value = set;

    for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
    {
        warn_value.warn_value[i] = ty_report_data.pzem[i].power_warn_value;
    }

    osal_snv_write(SNV_ID_WARN_VALUE, sizeof(snv_warn_value_t), &warn_value);
}

void ty_set_humi_warn_value(uint8_t set)
{
    snv_warn_value_t warn_value = {0};

    ty_report_data.humi_warn_value = set;
    warn_value.humi_warn_value = set;

    for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
    {
        warn_value.warn_value[i] = ty_report_data.pzem[i].power_warn_value;
    }

    osal_snv_write(SNV_ID_WARN_VALUE, sizeof(snv_warn_value_t), &warn_value);
}

void ty_clean_all_data(uint8_t ch)
{
    if(ch >= TY_PZEM_NUM)
    {
        return;
    }

    pzem_dev.clean_ctr_index_bit |= (1 << ch);

    // 进入清除数据流程
    if(pzem_dev.step < TY_PZEM_CELAN_CTR_INIT)
    {
        pzem_dev.step = TY_PZEM_CELAN_CTR_CHANGE_UART;
    }
}

static void ty_pzem_get_data_pro(void)
{
    switch(pzem_dev.step)
    {
        // 读取数据
        case TY_PZEM_CTR_INIT:

            if(pzem_dev.change_uart)
            {
                pzem_dev.change_uart(pzem_dev.pzem_ctr_index);

                pzem_dev.step = TY_PZEM_CTR_SEND_DATA;
            }

            break;

        case TY_PZEM_CTR_CHANGE_UART:

            if(pzem_dev.change_uart)
            {
                pzem_dev.change_uart(pzem_dev.pzem_ctr_index);

                pzem_dev.step = TY_PZEM_CTR_SEND_DATA;
            }

            break;

        case TY_PZEM_CTR_SEND_DATA:

            pzem_get_all_data_async(&pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_drv);

            pzem_dev.step = TY_PZEM_CTR_REC_DATA;

            pzem_dev.step_cnt = 0;

            break;

        case TY_PZEM_CTR_REC_DATA:

            if((++pzem_dev.step_cnt * TY_PZEM_PRO_TIME_MS) >= TY_PZEM_REC_DATA_TIME_MS)
            {
                pzem_dev.step = TY_PZEM_CTR_CHANGE_UART;

                // 解析数据
                LOG("uart len = %d, data = \n", uart_rx_buf.data_cnt);
                LOG_DUMP_BYTE(uart_rx_buf.data, uart_rx_buf.data_cnt);

                if(pzem_analysis_all_data(&pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_drv, uart_rx_buf.data, 
                                    uart_rx_buf.data_cnt, &pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_data) == PZEM_OK)
                {
                    pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].err_cnt = 0;
                    ty_report_warn_push_reset(pzem_dev.pzem_ctr_index + FIRST_SENSOR_ERROR);
                    
                    LOG("pzem[%d] data = \n", pzem_dev.pzem_ctr_index);
                    LOG("voltage = %d\n", pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_data.voltage);
                    LOG("current = %d\n", pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_data.current);
                    LOG("power = %d\n", pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_data.power);
                    LOG("energy = %d\n", pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_data.energy);
                    LOG("frequency = %d\n", pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_data.frequency);
                    LOG("pf = %d\n", pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_data.pf);
                    LOG("alarms = %d\n", pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_data.alarms);
                }
                else
                {
                    pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].err_cnt++;

                    LOG("pzem[%d] data error cnt %d\n", pzem_dev.pzem_ctr_index, pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].err_cnt);

                    if(pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].err_cnt >= 3)
                    {
                        pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].err_cnt = 0;
                    
                        ty_report_warn_push_set(pzem_dev.pzem_ctr_index + FIRST_SENSOR_ERROR);
                    }
                }

                uart_rx_buf.data_cnt = 0;

                pzem_dev.pzem_ctr_index++;

                if(pzem_dev.pzem_ctr_index >= TY_PZEM_NUM)
                {
                    pzem_dev.step_cnt = 0;

                    pzem_dev.pzem_ctr_index = 0;

                    pzem_dev.step = TY_PZEM_CTR_READ_TEMP_HUMI;

                    for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
                    {
                        // 触发实时功率上传
                        if(abs(pzem_dev.ty_pzem_drv[i].pzem_data.power / 10 - ty_report_data.pzem[i].real_power) > TY_REAL_POWER_CHANGE_LIMIT)
                        {
                            osal_set_event(ty_task_id, TY_REAL_POWER_UPDATA_EVENT);
                        }

                        ty_report_data.pzem[i].real_power = pzem_dev.ty_pzem_drv[i].pzem_data.power / 10; // 1w为单位

                        ty_report_data.pzem[i].total_energy = pzem_dev.ty_pzem_drv[i].pzem_data.energy;

                        ty_report_data.pzem[i].day_energy = (ty_report_data.pzem[i].total_energy > last_day_total_enegry.total_enegry[i]) ?
                                                            (ty_report_data.pzem[i].total_energy - last_day_total_enegry.total_enegry[i]) : 0;

                        // 判断实时功率是否需要告警
                        if((ty_report_data.pzem[i].warn_switch == 1) && 
                            (ty_report_data.pzem[i].real_power > (ty_report_data.pzem[i].power_warn_value * 100)))
                        {
                            ty_report_warn_push_set(i + FIRST_POWER_WARN);
                        }
                        else
                        {
                            ty_report_warn_push_reset(i + FIRST_POWER_WARN);
                        }
                    }
                }
            } 

            break;

        case TY_PZEM_CTR_READ_TEMP_HUMI:
		{
#if (USER_TEMP_SENSOR == 1)
            static uint8_t temp_sensor_error_cnt = 0;
            dht_driver_data_t data = {0};

            if(dht_get_temp_humi_data(&data))
            {
                temp_sensor_error_cnt = 0;
                ty_report_warn_push_reset(TEMP_SENSOR_ERROR);

                if((abs(data.temp - ty_report_data.temp) > TY_TEMP_CHANGE_LIMIT) || 
                    (abs(data.humi / 100 - ty_report_data.humi) > TY_HUMI_CHANGE_LIMIT))
                {
                    osal_set_event(ty_task_id, TY_TEMP_HUMI_UPDATA_EVENT); // 触发温湿度上传
                }

                ty_report_data.temp = data.temp;

                ty_report_data.humi = data.humi / 100;

                if(ty_report_data.temp_humi_warn_switch == 1)
                {
                    if(ty_report_data.temp > (ty_report_data.temp_warn_value * 100))
                    {
                        ty_report_warn_push_set(TEMP_WARN);
                    }
                    else
                    {
                        ty_report_warn_push_reset(TEMP_WARN);
                    }

                    if(ty_report_data.humi > ty_report_data.humi_warn_value)
                    {
                        ty_report_warn_push_set(HUMI_WARN);
                    }
                    else
                    {
                        ty_report_warn_push_reset(HUMI_WARN);
                    }
                }
                else
                {
                    ty_report_warn_push_reset(TEMP_WARN);
                    ty_report_warn_push_reset(HUMI_WARN);
                }

                LOG("read temp = %d, humi = %d \n", ty_report_data.temp, ty_report_data.humi);
            }
            else
            {
                temp_sensor_error_cnt++;

                if(temp_sensor_error_cnt >= 3)
                {
                    temp_sensor_error_cnt = 0;
                    
                    ty_report_warn_push_set(TEMP_SENSOR_ERROR);
                }
            }
#endif

            osal_set_event(ty_task_id, TY_WARN_PUSH_UPDATA_EVENT);

            pzem_dev.step = TY_PZEM_CTR_DELAY;
        }

            break;

        case TY_PZEM_CTR_DELAY:

            if((++pzem_dev.step_cnt * TY_PZEM_PRO_TIME_MS) >= TY_PZEM_DEALY_TIME_MS)
            {
                pzem_dev.step_cnt = 0;

                pzem_dev.step = TY_PZEM_CTR_CHANGE_UART;
            }

            break;


        // 清除数据
        case TY_PZEM_CELAN_CTR_INIT:
            break;

        case TY_PZEM_CELAN_CTR_CHANGE_UART:

            if(pzem_dev.clean_ctr_index_bit == 0)
            {
                pzem_dev.step_cnt = 0;

                pzem_dev.pzem_ctr_index = 0;

                pzem_dev.step = TY_PZEM_CTR_CHANGE_UART;

                // 清除完数据，上报一次所有数据
                ty_all_data_updata();
            }
            else if(pzem_dev.change_uart)
            {
                for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
                {
                    if((pzem_dev.clean_ctr_index_bit >> i) & 0x01 == 0x01)
                    {
                        pzem_dev.pzem_ctr_index = i;

                        break;
                    }
                }

                pzem_dev.change_uart(pzem_dev.pzem_ctr_index);

                pzem_dev.step = TY_PZEM_CELAN_CTR_SEND_DATA;
            }

            break;

        case TY_PZEM_CELAN_CTR_SEND_DATA:

            pzem_clean_all_data_async(&pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_drv);

            pzem_dev.step = TY_PZEM_CELAN_CTR_REC_DATA;

            pzem_dev.step_cnt = 0;

            break;

        case TY_PZEM_CELAN_CTR_REC_DATA:

            if((++pzem_dev.step_cnt * TY_PZEM_PRO_TIME_MS) >= TY_PZEM_REC_DATA_TIME_MS)
            {
                pzem_dev.clean_ctr_index_bit &= (~(1 << pzem_dev.pzem_ctr_index));

                pzem_dev.step = TY_PZEM_CELAN_CTR_CHANGE_UART;

                // 解析数据
                LOG("uart len = %d, data = \n", uart_rx_buf.data_cnt);
                LOG_DUMP_BYTE(uart_rx_buf.data, uart_rx_buf.data_cnt);

                if(pzem_analysis_clean_all_data(&pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_drv, uart_rx_buf.data, 
                                    uart_rx_buf.data_cnt, &pzem_dev.ty_pzem_drv[pzem_dev.pzem_ctr_index].pzem_data) == PZEM_OK)
                {
                    LOG("pzem[%d] clean data success \n", pzem_dev.pzem_ctr_index);

                    // 清除数据
                    ty_report_data.pzem[pzem_dev.pzem_ctr_index].day_energy = 0;
                    ty_report_data.pzem[pzem_dev.pzem_ctr_index].total_energy = 0;

                    last_day_total_enegry.total_enegry[pzem_dev.pzem_ctr_index] = 0;

                    osal_snv_write(SNV_ID_TOTAL_ENEGRY, sizeof(snv_total_enegry_t), &last_day_total_enegry);
                }
                else
                {
                    LOG("pzem[%d] clean data error\n", pzem_dev.pzem_ctr_index);
                }

                uart_rx_buf.data_cnt = 0;
            }

            break;
    }
}

static void ty_ota_flash_event(uint8_t event, void* data)
{
    static uint8_t led_state = 0;

    switch (event)
    {
    case OTA_FLASH_INIT:

        led_set_enable(0);

        ota_is_start = 1;

        LOG("\n\n ota start \n\n");

        osal_start_timerEx(ty_task_id, TY_OTA_DONE_REBOOT_EVENT, TY_OTA_TIME_OUT_MS);

        break;

    case OTA_FLASH_WRITE_DATA:
    {
        led_hal_set_state(led_state);

        led_state = (!led_state);

        if(data)
        {
            ota_write_data_event_t *event_data = (ota_write_data_event_t*)data;

            LOG("total len = %d, write len = %d\n", event_data->total_len, event_data->write_len);
        }

    }
        break;

    case OTA_FLASH_CHECK_DATA_START:

        led_hal_set_state(1);

        break;

    case OTA_FLASH_CHECK_DATA_END:

        led_hal_set_state(0);

        hal_gpio_fast_write(TY_POWER_IO, 0);

        osal_start_timerEx(ty_task_id, TY_OTA_DONE_REBOOT_EVENT, TY_UPDATA_REBOOT_TIME);

        if(data)
        {
            ota_check_end_event_t *event_data = (ota_check_end_event_t*)data;

            LOG("ota result = %d\n", event_data->result);
        }

        break;
    
    default:
        break;
    }
}

void ty_app_init(uint8_t task_id)
{
    snv_warn_switch_t warn_switch = {0};
    snv_warn_value_t warn_value = {0};

    ty_task_id = task_id;

    // snv data init
    if(osal_snv_read(SNV_ID_WARN_SWITCH, sizeof(snv_warn_switch_t), &warn_switch) != SUCCESS)
    {
        osal_memset(&warn_switch, 0, sizeof(snv_warn_switch_t));
    }

    if(osal_snv_read(SNV_ID_WARN_VALUE, sizeof(snv_warn_value_t), &warn_value) != SUCCESS)
    {
        for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
        {
            warn_value.warn_value[i] = 1;
        }
    }

    if(osal_snv_read(SNV_ID_TOTAL_ENEGRY, sizeof(snv_total_enegry_t), &last_day_total_enegry) != SUCCESS)
    {
        osal_memset(&last_day_total_enegry, 0, sizeof(snv_total_enegry_t));
    }

    for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
    {
        ty_report_data.pzem[i].warn_switch = warn_switch.warn_switch[i];
        ty_report_data.pzem[i].power_warn_value = warn_value.warn_value[i];
    }

    ty_report_data.temp_humi_warn_switch = warn_switch.temp_humi_warn_switch;

    ty_report_data.temp_warn_value = warn_value.temp_warn_value;
    ty_report_data.humi_warn_value = warn_value.humi_warn_value;

    led_dirver_init();
	
    key_driver_init(ty_key_event_cb);

    dht_driver_init(&dht_ops);

    wifi_protocol_init();

    ty_uart_init(ty_uart_event_handle);

    watchdog_config(WDG_4S);

    ota_flash_reg_event_cb(ty_ota_flash_event);

    osal_start_reload_timer(ty_task_id, TY_SEC_TICK_EVENT, TY_SEC_TICK_TIME_MS);

    osal_start_timerEx(ty_task_id, TY_UART_REC_DATA, TY_APP_RELOAD_TIME);

    osal_start_timerEx(ty_task_id, TY_PZEM_PRO_EVENT, TY_PZEM_PRO_TIME_MS);

#if (USER_TEMP_SENSOR == 1)
    osal_start_timerEx(ty_task_id, TY_TEMP_HUMI_UPDATA_EVENT, TY_TEMP_HUMI_UPDATA_TIME_MS);
#endif

    osal_start_timerEx(ty_task_id, TY_REAL_POWER_UPDATA_EVENT, TY_REAL_POWER_UPDATA_TIME_MS);

    osal_start_timerEx(ty_task_id, TY_DAY_ENERGY_UPDATA_EVENT, TY_DAY_ENERGY_UPDATA_TIME_MS);
}

uint16_t ty_app_pro_event(uint8_t task_id, uint16_t events)
{
    if(task_id != ty_task_id)
    {
        return 0;
    }

    if( events & TY_UART_REC_DATA)
    {
        static uint8_t old_wifi_state = 0;

        uint8_t wifi_state = mcu_get_wifi_work_state();

        switch(wifi_state)
        {
            case SMART_CONFIG_STATE:
            case AP_STATE:
            case SMART_AND_AP_STATE:

                led_set_status(led_net_cfg);

                break;
            case WIFI_NOT_CONNECTED:

                led_set_status(led_net_connect);

                break;
            case WIFI_CONNECTED:

                led_set_status(led_net_con_suc);

                break;
            case WIFI_CONN_CLOUD:

                led_set_status(led_cloud_con_suc);

                break;

            case WIFI_LOW_POWER:

                led_set_status(led_wifi_low_power);

                break;

            default:
                led_set_status(led_net_cfg);
                break;  
        }

        if(wifi_state != old_wifi_state)
        {
            if(wifi_state == WIFI_CONN_CLOUD)
            {
                osal_start_timerEx(ty_task_id, TY_CONNECT_CLOUD_UPDATA_ALL_DATA_EVENT, 4000); // 断网重连上报所有数据
            }

            old_wifi_state = wifi_state;
        }

        hal_watchdog_feed();

        wifi_uart_service();

        if(ota_is_start == 0)
        {
            led_pro(TY_APP_RELOAD_TIME);

            key_pro(TY_APP_RELOAD_TIME);

            osal_start_timerEx(ty_task_id, TY_UART_REC_DATA, TY_APP_RELOAD_TIME);
        }
        else
        {
            osal_start_timerEx(ty_task_id, TY_UART_REC_DATA, TY_APP_OTA_RELOAD_TIME);
        }

        return (events ^ TY_UART_REC_DATA);
    }

    if( events & TY_KEY_LONG_EVENT)
    {
        LOG("TY_KEY_LONG_EVENT\r\n");
        //判断当前状态，未配网->进入配网，已配网->退网
        switch(mcu_get_wifi_work_state())
        {
            case SMART_CONFIG_STATE:
            case AP_STATE:
            case SMART_AND_AP_STATE:

                //配网中，不处理

                break;
            case WIFI_NOT_CONNECTED:
            case WIFI_CONNECTED:
            case WIFI_CONN_CLOUD:

                //退网
                mcu_reset_wifi();

                break;

            case WIFI_LOW_POWER:

                //进入配网
                mcu_set_wifi_mode(AP_CONFIG);

                break;

            default:
                break;  
        }

        return (events ^ TY_KEY_LONG_EVENT);
    }

    if(events & TY_PZEM_PRO_EVENT)
    {
        if(ota_is_start != 1)
        {
            ty_pzem_get_data_pro();

            osal_start_timerEx(ty_task_id, TY_PZEM_PRO_EVENT, TY_PZEM_PRO_TIME_MS);
        }

        return (events ^ TY_PZEM_PRO_EVENT);
    }

    if(events & TY_SEC_TICK_EVENT)
    {
        if(ty_time.is_vail == 0)
        {
            mcu_get_system_time();
        }
        else
        {
            ty_time.sec++;
            if(ty_time.sec == 2)
            {
                mcu_get_system_time();
            }

            if(ty_time.sec >= 60)
            {
                ty_time.sec = 0;

                ty_time.min++;

                if(ty_time.min >= 60)
                {
                    ty_time.min = 0;

                    ty_time.hour++;

                    if(ty_time.hour >= 24)
                    {   
                        ty_time.hour = 0;

                        // ty_time.day++;  // 等待rtc时间同步
                    }
                }
            }

            if((ty_time.day != last_day_total_enegry.save_time.day) || 
               (ty_time.month != last_day_total_enegry.save_time.month) || 
               (ty_time.year != last_day_total_enegry.save_time.year))
            {
                last_day_total_enegry.save_time.year = ty_time.year;
                last_day_total_enegry.save_time.month = ty_time.month;
                last_day_total_enegry.save_time.day = ty_time.day;

                for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
                {
                    ty_report_data.pzem[i].day_energy = 0;

                    ty_report_data.pzem[i].total_energy = pzem_dev.ty_pzem_drv[i].pzem_data.energy;

                    last_day_total_enegry.total_enegry[i] = pzem_dev.ty_pzem_drv[i].pzem_data.energy;
                }

                osal_snv_write(SNV_ID_TOTAL_ENEGRY, sizeof(snv_total_enegry_t), &last_day_total_enegry);

                osal_set_event(ty_task_id, TY_DAY_ENERGY_UPDATA_EVENT);
            }
        }

        ty_report_wara_push_interval_tick();

        return (events ^ TY_SEC_TICK_EVENT);
    }

    if(events & TY_TEMP_HUMI_UPDATA_EVENT)
    {
        ty_dht_humi_temp_updata(ty_report_data.temp, ty_report_data.humi);

        osal_start_timerEx(ty_task_id, TY_TEMP_HUMI_UPDATA_EVENT, TY_TEMP_HUMI_UPDATA_TIME_MS);

        return (events ^ TY_TEMP_HUMI_UPDATA_EVENT);
    }

    if(events & TY_REAL_POWER_UPDATA_EVENT)
    {
        for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
        {
            ty_real_power_updata(i, ty_report_data.pzem[i].real_power);
        }

        ty_three_real_power_updata(ty_report_data.pzem[0].real_power + ty_report_data.pzem[1].real_power + ty_report_data.pzem[2].real_power);

        osal_start_timerEx(ty_task_id, TY_REAL_POWER_UPDATA_EVENT, TY_REAL_POWER_UPDATA_TIME_MS);

        return (events ^ TY_REAL_POWER_UPDATA_EVENT);
    }

    if(events & TY_DAY_ENERGY_UPDATA_EVENT)
    {
        for (uint8_t i = 0; i < TY_PZEM_NUM; i++)
        {
            ty_day_energy_updata(i, ty_report_data.pzem[i].day_energy);
            ty_total_energy_updata(i, ty_report_data.pzem[i].total_energy);
        }

        ty_day_three_energy_updata(ty_report_data.pzem[0].day_energy + ty_report_data.pzem[1].day_energy + ty_report_data.pzem[2].day_energy);
        ty_total_three_energy_updata(ty_report_data.pzem[0].total_energy + ty_report_data.pzem[1].total_energy + ty_report_data.pzem[2].total_energy);

        osal_start_timerEx(ty_task_id, TY_DAY_ENERGY_UPDATA_EVENT, TY_DAY_ENERGY_UPDATA_TIME_MS);

        return (events ^ TY_DAY_ENERGY_UPDATA_EVENT);
    }

    if(events & TY_WARN_PUSH_UPDATA_EVENT)
    {
        static uint32_t old_wara_push = 0;

        if(old_wara_push != ty_report_data.warn_push)
        {
            LOG("ty warn push old:0x%04x rel:0x%04x xor:0x%04x\n", old_wara_push, ty_report_data.warn_push, 
                                                                   old_wara_push ^ ty_report_data.warn_push) ;

            ty_warn_push_updata(old_wara_push ^ ty_report_data.warn_push, ty_report_data.warn_push);

            old_wara_push = ty_report_data.warn_push;
        }

        return (events ^ TY_WARN_PUSH_UPDATA_EVENT);
    }

    if(events & TY_OTA_DONE_REBOOT_EVENT)
    {
        LOG("\n\nTY_OTA_DONE_REBOOT_EVENT\n\n");

        ota_flash_reboot();

        return (events ^ TY_OTA_DONE_REBOOT_EVENT);
    }

    if(events & TY_SEND_RESET_DP_EVENT)
    {
        LOG("TY_SEND_RESET_DP_EVENT\n");

        ty_reset_reason_updata(g_system_reset_cause);

        return (events ^ TY_SEND_RESET_DP_EVENT);
    }

    if(events & TY_CONNECT_CLOUD_UPDATA_ALL_DATA_EVENT)
    {
        LOG("TY_CONNECT_CLOUD_UPDATA_ALL_DATA_EVENT\n");

        ty_all_data_updata();

        return (events ^ TY_CONNECT_CLOUD_UPDATA_ALL_DATA_EVENT);
    }
		
	return 0;
}


