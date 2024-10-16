/**************************************************************************************************
*******
**************************************************************************************************/

/**************************************************************************************************
    Filename:       bsp_button_task.h
    Revised:        $Date $
    Revision:       $Revision $


**************************************************************************************************/
#ifndef __BSP_BUTTON_TASK_H__
#define __BSP_BUTTON_TASK_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
    INCLUDES
*/
#include "bus_dev.h"
#include "bsp_gpio.h"
#include "bsp_button.h"
#include "log.h"

/*********************************************************************
    CONSTANTS
*/
typedef struct _bsp_button_Cfg_t
{
    #ifdef BSP_BTN_LONG_PRESS_ENABLE
    uint16 bsp_long_press_start_cnt;
    uint16 bsp_long_press_keep_cnt;
    #endif
    #if (BSP_COMBINE_BTN_NUM > 0)
    uint32_t usr_combine_btn_array[BSP_COMBINE_BTN_NUM];
    uint8 combine_btn_num;
    #endif
    const GPIO_Pin_e col_pin[MATRIX_KEYBOARD_ROW];
    const GPIO_Pin_e row_pin[MATRIX_KEYBOARD_ROW];
    bsp_app* bsp_evt_cb;
} bsp_button_Cfg_t;
/*********************************************************************
    MACROS
*/
#define BSP_SOFT_POLLING_EVT (0x0001)
#define BSP_POWER_ON_EVT (0x0004)

#define POWER_REPEATED_RELATIVE_TIME (210 * 1000 / 625)
#define BSP_SOFT_SCAN_POLLING_TIME (10)

void system_run_keyboard_checking(void);
void kscan_enter_power_off_gpio_option(void);
void btp_button_init(bsp_button_Cfg_t* button_cfg);
uint16 Bsp_Btn_ProcessEvent(uint8 task_id, uint16 events);
void Bsp_Btn_Init(uint8 task_id);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* BSP_BUTTON_TASK_H */
