/**************************************************************************************************
*******
**************************************************************************************************/
/*******************************************************************************
    # File: lib_efuse.h
    # Hist:
     2022.7.7  YU File Creation
     2022.8.11 YU Reduce memory usage
 *******************************************************************************/
#ifndef LIB_EFUSE3_H
#define LIB_EFUSE3_H

#include <stdbool.h>
#include <stdint.h>

struct efuse_data_buff
{
    unsigned int data[2];
};

unsigned int lib_efuse_mft(void);
unsigned int lib_efuse_zigbee(void);
unsigned int lib_efuse_prog_ver(void);
unsigned int lib_efuse_chip_ver(void);
unsigned int lib_efuse_ble(void);
unsigned int lib_efuse_lotnum(void);
unsigned int lib_efuse_site(void);
unsigned int lib_efuse_time_stamp(void);
unsigned int lib_efuse_mesh(void);
unsigned int lib_efuse_multirole(void);
unsigned int lib_efuse_pass_flg(void);
void lib_efuse_load(uint32_t* efuse_data);
void lib_read_hw_version(uint32_t* buff);

#endif // LIB_EFUSE3_H
