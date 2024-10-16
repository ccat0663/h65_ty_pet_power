#include "bus_dev.h"
#include "jump_function.h"
#include "pwrmgr.h"
#include "mcu.h"
#include "ll_hw_drv.h"

#define RECONFIG_BB_DEALY  40
typedef struct
{
    uint16_t currevt;
    uint16_t nextevt;
    uint16_t expevt;
    bool restore_flag;
}EvtType;

EvtType materEvtCache[MAX_NUM_LL_CONN];
extern uint8_t g_maxConnNum;
extern llConns_t g_ll_conn_ctx;
extern llConnState_t* conn_param;
extern uint32_t  g_counter_traking_cnt;
extern void ll_scheduler_multiconn(uint32 time);
extern void enterSleepProcess1(uint32 time);
extern void ll_hw_go1(void);

void ll_scheduler_multiroleconn(uint32 time)
{

    for (uint8_t i = 0; i < g_maxConnNum; i++)
    {
        if ((i != g_ll_conn_ctx.currentConn) && conn_param[i].active)
        {
            if (g_ll_conn_ctx.scheduleInfo[i].linkRole == LL_ROLE_MASTER && (conn_param[i].expirationEvent < conn_param[i].nextEvent))
            {
                materEvtCache[i].currevt = conn_param[i].currentEvent;
                materEvtCache[i].nextevt = conn_param[i].nextEvent;
                materEvtCache[i].expevt = conn_param[i].expirationEvent;
                materEvtCache[i].restore_flag = true;
                if (conn_param[i].expirationEvent == 0)
                {
                    conn_param[i].expirationEvent = 1;
                }
                conn_param[i].nextEvent = 0;
            }
        }

    }
    ll_scheduler_multiconn(time);

    for (uint8_t i = 0; i < g_maxConnNum; i++)
    {
        if ( conn_param[i].active && materEvtCache[i].restore_flag && (g_ll_conn_ctx.scheduleInfo[i].linkRole == LL_ROLE_MASTER) && (conn_param[i].currentEvent == conn_param[i].nextEvent) )
        {
            conn_param[i].currentEvent = materEvtCache[i].currevt;
            conn_param[i].nextEvent = materEvtCache[i].nextevt;
            conn_param[i].expirationEvent = materEvtCache[i].expevt;
            materEvtCache[i].restore_flag = false;
        }
    }

}

uint8 llSetupNextMasterEvent1( void )
{
    for (uint8_t i = 0; i < g_maxConnNum; i++)
    {
        if ( conn_param[i].active && (g_ll_conn_ctx.scheduleInfo[i].linkRole == LL_ROLE_MASTER) && materEvtCache[i].restore_flag )
        {
            conn_param[i].currentEvent = materEvtCache[i].currevt;
            conn_param[i].nextEvent = materEvtCache[i].nextevt;
            conn_param[i].expirationEvent = materEvtCache[i].expevt;
            materEvtCache[i].restore_flag = false;
        }

    }

    return llSetupNextMasterEvent0();
}


void enterSleepProcess2(uint32 time)
{
    g_counter_traking_cnt=0;
    enterSleepProcess1(time);
}
void ll_patch_multi_role(void)
{
    JUMP_FUNCTION(LL_SCHEDULER)                     =   (uint32_t)&ll_scheduler_multiroleconn;
    JUMP_FUNCTION(LL_SETUP_NEXT_MASTER_EVT)         =   (uint32_t)&llSetupNextMasterEvent1;
}

void ll_patch_sleep(void)
{
    JUMP_FUNCTION(ENTER_SLEEP_PROCESS)              =   (uint32_t)&enterSleepProcess2;
}

void ll_hw_go2(void)
{
	ll_hw_go1();
	uint32_t register_value = *(volatile uint32_t*)(BB_HW_BASE+ 0xbc);
	uint8_t register_bb_value = (uint8_t)((register_value>>16) & 0xFF);
	if (register_bb_value > 127)
	{
			*(volatile uint32_t*)(BB_HW_BASE+ 0xbc) = (register_value & ~(0xff << 16)) | (RECONFIG_BB_DEALY << 16);
	}
}

void ll_patch_adv(void)
{
    JUMP_FUNCTION(LL_HW_GO)                         =   (uint32_t)&ll_hw_go2;
}



