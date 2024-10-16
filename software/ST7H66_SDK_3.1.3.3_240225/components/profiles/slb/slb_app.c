/**************************************************************************************************
*******
**************************************************************************************************/

/*************************************************************************************************
    Filename:       slb_app.c

    Revised:

    Revision:

    Description:    This file is used for slbota applications.
**************************************************************************************************/

#ifdef PHY_SLB_OTA_ENABLE

#include "slb_app.h"
#include "ppsp_serv.h"
#include "ppsp_impl.h"
#include "OSAL.h"
#include "clock.h"


static void simple_reset_hdlr(void);


static uint8 slb_ota_TaskID;

static ppsp_impl_clit_hdlr_t ble_ppsp_impl_appl_hdlr =
{
    .ppsp_impl_appl_rset_hdlr = simple_reset_hdlr,
};


static void simple_reset_hdlr(void)
{
    osal_start_timerEx(slb_ota_TaskID, SLB_OTA_RESET_EVENT, 3000);
}


void SLB_OTA_Init(uint8 task_id)
{
    slb_ota_TaskID = task_id;
    ppsp_serv_add_serv(PPSP_SERV_CFGS_SERV_FEB3_MASK);
    ppsp_impl_reg_serv_appl(&ble_ppsp_impl_appl_hdlr);
}


uint16 SLB_OTA_ProcessEvent(uint8 task_id, uint16 events)
{
    if(events & SLB_OTA_RESET_EVENT)
    {
        hal_system_soft_reset();
        return (events ^ SLB_OTA_RESET_EVENT);
    }

    return 0;
}


#endif
