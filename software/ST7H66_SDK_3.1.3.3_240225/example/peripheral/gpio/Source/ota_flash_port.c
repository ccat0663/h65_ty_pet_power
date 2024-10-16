
#include "types.h"
#include "OSAL.h"
#include "uart.h"
#include "flash.h"
#include "log.h"
#include "crc32.h"

#include "ota_flash_port.h"

#define OTA_NVM_BASE     0x11055000
#define OTA_NVM_LEN      (172*1024) //172K
#define OTA_NVM_SECT_LEN  (4*1024)

#define CRC_INIT_DATA 0xffffffff

#define READ_BUFF_SIZE 256

typedef struct
{
    uint32_t rec_len;

    uint32_t rec_data_crc;
    uint32_t read_data_crc;
    
    uint32_t erase_addr;

    uint8_t read_buff[READ_BUFF_SIZE];

}ota_flash_port_t;

static const char* ota_tag = "OTAF";

static ota_flash_port_t ota_flash_port = {0};

static ota_flash_event_cb ota_event_cb = NULL;


void ota_flash_init(void)
{
    ota_flash_port.rec_len = 0;
    ota_flash_port.rec_data_crc = CRC_INIT_DATA;
    ota_flash_port.read_data_crc = CRC_INIT_DATA;

    ota_flash_port.erase_addr = 0;

    if(ota_event_cb)
    {
        ota_event_cb(OTA_FLASH_INIT, NULL);
    }
}

void ota_flash_reg_event_cb(ota_flash_event_cb cb)
{
    ota_event_cb = cb;
}

uint8_t ota_flash_write(uint32_t addr, uint8_t *data, uint16_t len)
{
    uint8_t ret = 0;
    uint32_t flash_addr = OTA_NVM_BASE + addr;

    if ((0 == (flash_addr % OTA_NVM_SECT_LEN)) || (flash_addr & 0xf000) != (ota_flash_port.erase_addr & 0xf000))
    {
        ret = hal_flash_erase_sector(flash_addr);

        if(ret)
        {
            return ret;
        }

        ota_flash_port.erase_addr = flash_addr;
    }

    // first pack
    if(ota_flash_port.rec_len == 0)
    {
        // clean ota tag
        osal_memset(data, 0xff, 4);
    }

    ret = hal_flash_write_by_dma(flash_addr, (uint8_t*)data, len);

    ota_flash_port.rec_len += len;

    ota_flash_port.rec_data_crc = crc32_cyc_cal(ota_flash_port.rec_data_crc, data, len);

    if(ota_event_cb)
    {
        ota_write_data_event_t data = {0};

        data.write_len = len;
        data.total_len = ota_flash_port.rec_len;

        ota_event_cb(OTA_FLASH_WRITE_DATA, (void*)&data);
    }

    return ret;
}

uint8_t ota_flash_finsh(void)
{
    ota_check_end_event_t data = {0};
    uint32_t file_len = ota_flash_port.rec_len;
    uint32_t read_len = 0;

    if(ota_event_cb)
    {
        ota_event_cb(OTA_FLASH_CHECK_DATA_START, NULL);
    }

    while(file_len > 0)
    {
        osal_memset(ota_flash_port.read_buff, 0xff, READ_BUFF_SIZE);

        if(file_len > READ_BUFF_SIZE)
        {
            read_len = READ_BUFF_SIZE;
        }
        else
        {
            read_len = file_len;
        }

        hal_flash_read(OTA_NVM_BASE + (ota_flash_port.rec_len - file_len), ota_flash_port.read_buff, read_len);

        ota_flash_port.read_data_crc = crc32_cyc_cal(ota_flash_port.read_data_crc, ota_flash_port.read_buff, read_len);

        file_len -= read_len;
    }

    if(ota_flash_port.read_data_crc == ota_flash_port.read_data_crc)
    {
        data.result = 1;

        hal_flash_write_by_dma(OTA_NVM_BASE, (uint8_t*)ota_tag, 4);
    }
    else
    {
        data.result = 0;
    }

    if(ota_event_cb)
    {
        ota_event_cb(OTA_FLASH_CHECK_DATA_END, (void*)&data);
    }

    return 0;
}

void ota_flash_reboot(void)
{
    hal_system_soft_reset();
}
