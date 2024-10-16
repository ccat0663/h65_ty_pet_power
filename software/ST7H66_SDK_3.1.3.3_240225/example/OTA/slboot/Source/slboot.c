/**************************************************************************************************
*******
**************************************************************************************************/


/*********************************************************************
    INCLUDES
*/

#include "bcomdef.h"
#include "OSAL.h"
#include "slboot.h"
#include "OSAL_Tasks.h"
#include "slb.h"
#include "ota_flash.h"
#include "error.h"
__ATTR_SECTION_SRAM__ const uint8 tasksCnt = 0;
uint16* tasksEvents;
extern void bx_to_application(uint32_t run_addr);
extern uint32_t ota_slb_xip_addr;

__ATTR_SECTION_SRAM__ const pTaskEventHandlerFn tasksArr[2] =
{
    NULL,
    NULL
};

void osalInitTasks( void )
{
}

#define __APP_RUN_ADDR__        (0x1FFF1838)


#if   defined ( __CC_ARM )
__asm void __attribute__((section("ota_app_loader_area"))) jump2app(uint32_t entry)
{
    LDR R0, = __APP_RUN_ADDR__
              LDR R1, [R0, #4]
              BX R1
              ALIGN
}
#elif defined ( __GNUC__ )
void jump2app(uint32_t entry)
{
    __ASM volatile("ldr r0, %0\n\t"
                   "ldr r1, [r0, #4]\n\t"
                   "bx r1"
                   :"+m"(entry)
                  );
}
#endif

int __attribute__((section("ota_app_loader_area"))) run_application(void)
{
    int ret;
    uint32_t app_entry = __APP_RUN_ADDR__;
    HAL_ENTER_CRITICAL_SECTION();
    ret = ota_flash_load_app();

    if(ota_slb_xip_addr)
        app_entry = ota_slb_xip_addr;

    //bypass cache
    AP_PCR->CACHE_BYPASS = 1;

    if(ret == PPlus_SUCCESS)
    {
        jump2app(app_entry);
    }

    HAL_EXIT_CRITICAL_SECTION();
    return PPlus_SUCCESS;
}

void slboot_main(void)
{
    //check firmware update (exchange area)
    slb_boot_load_exch_zone();
    //boot firmware
    run_application();

    while(1)
    {
        ;
    }
}





/*********************************************************************
*********************************************************************/
