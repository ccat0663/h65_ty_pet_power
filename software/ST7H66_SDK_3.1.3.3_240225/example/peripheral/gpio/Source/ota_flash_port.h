
#ifndef _OTA_FLASH_PORT_H 
#define _OTA_FLASH_PORT_H 
#ifdef __cplusplus
extern "C" {
#endif

enum{
    OTA_FLASH_INIT = 0,
    OTA_FLASH_WRITE_DATA,
    OTA_FLASH_CHECK_DATA_START,
    OTA_FLASH_CHECK_DATA_END,
};

typedef struct
{
    uint32_t total_len;
    uint16_t write_len;

}ota_write_data_event_t;

typedef struct
{
    uint8_t result;
    
}ota_check_end_event_t;


typedef void (*ota_flash_event_cb)(uint8_t event, void* data);

void ota_flash_init(void);

void ota_flash_reg_event_cb(ota_flash_event_cb cb);

uint8_t ota_flash_write(uint32_t addr, uint8_t *data, uint16_t len);

uint8_t ota_flash_finsh(void);

void ota_flash_reboot(void);

#ifdef __cplusplus
}
#endif
#endif	// _OTA_FLASH_PORT_H

