#include "osal.h"
#include "crc16.h"
#include "fs_shadow.h"
#include "error.h"
#include "fs_autotest.h"
#include "cliface.h"
#include "clock.h"
#include "fs_test.h"
#include "fs.h"
#include "fsr_raid.h"
#include "flash.h"


#define FS_TEST_BUF_SIZE 16*1024
uint8_t fs_test_buf[FS_TEST_BUF_SIZE] __attribute__((aligned(4))) ; //assume that max file size is 16K

extern  uint32_t _pos;
extern  uint8_t _cnt_trigger;
extern uint8_t _cnt;

//preamble string add
void fst_preamble(uint8_t* params)
{
    //CLIT_preamble(params);
    CLIT_output("[%s, 0]",CLIT_preamble(params) )
}
//reset command for test
uint16_t fst_rst(uint32_t argc, uint8_t* argv[])
{
    CLIT_output("[%d, 0]",PPlus_SUCCESS);
    NVIC_SystemReset();
}
/*
    uint16_t fst_flist(uint32_t argc, uint8_t* argv[])
    {
    uint32_t fnum, i;
    uint16_t* fid = (uint16_t*)fs_test_buf;
    uint16_t* fsize = (uint16_t*)(fs_test_buf+ FS_TEST_BUF_SIZE/2);
    int ret = hal_fs_list(&fnum, fid, fsize);

    CLIT_outputs("[%d, %d , [",ret, fnum);

    for(i = 0; i< fnum; i++){
    if(i)CLIT_outputc(",");
    CLIT_outputc("[%d, %d]", fid[i], fsize[i]);
    }
    CLIT_outpute("]]");
    return 0;
    }
*/

//int hal_fs_get_size(uint32_t* fs_size, uint32_t* free_size,
//                    uint32_t* fsize_max, uint8_t* item_size, uint8_t* fs_snum)
//{
//    fs_ctx_t* pctx = &s_fs_ctx;
//    fs_cfg_t* pcfg = &(pctx->cfg);
//    uint32_t size = 0;

//    if(pctx->init_flg == false)
//        return PPlus_ERR_FS_UNINITIALIZED;

//    if(pctx->cur_item < FS_SECTOR_SIZE)
//    {
//        size = ((pctx->exch_sect + pcfg->fs_snum - pctx->cur_sect - 1)%pcfg->fs_snum) * FS_ITEMS_PER_SECT;
//        size += (FS_ITEMS_PER_SECT - pctx->cur_item);
//        size = size * FS_ITEM_DATA_LEN;
//    }

//    *free_size = size;
//    size = (pcfg->fs_snum-1)*FS_ITEMS_PER_SECT;
//    size = size*FS_ITEM_DATA_LEN;
//    *fs_size = size;
//    *item_size = FS_ITEM_DATA_LEN;
//    *fs_snum = pcfg->fs_snum;
//    *fsize_max = FS_FSIZE_MAX;
//    return PPlus_SUCCESS;
//}

//    //get total size, free size, max filesize, unit size, sector number
//    uint16_t fst_info(uint32_t argc, uint8_t* argv[])
//    {
//    uint32_t fs_size,free_size, fsize_max;
//    uint8_t item_size, sector_num;
//    int ret = hal_fs_get_size(&fs_size, &free_size, &fsize_max, &item_size, &sector_num);
//    CLIT_output("[%d, %d,%d,%d,%d,%d]",ret, fs_size, free_size, fsize_max, item_size, sector_num);

//    return 0;
//    }


//return read status and cksum of file
uint16_t fst_read( uint32_t argc, uint8_t* argv[])
{
    uint16_t fid =(uint16_t)CLI_strtoi(argv[0], strlen((const char*)argv[0]), 10);
    uint16_t size =(uint16_t)CLI_strtoi(argv[1], strlen((const char*)argv[1]), 10);
    uint16_t rd_size = 0;
    uint8_t x_flag = (uint8_t)(osal_rand()&0xff);
    fs_test_buf[size] = x_flag;

    int iret = hal_fs_item_read_entry(fid, fs_test_buf, size, &rd_size);

    if(iret)
    {
        CLIT_output("[%d, 0]",iret);
    }
    else
    {
        if(rd_size != size)
        {
            CLIT_output("[%d, 0]",PPlus_ERR_DATA_SIZE);
        }
        else
        {
            if(x_flag !=fs_test_buf[size])
            {
                CLIT_output("[%d, %d]",PPlus_ERR_INTERNAL,PPlus_ERR_DATA_CORRUPTION);
            }
            else
            {
                uint16_t crc=0;
                crc = crc16(0, fs_test_buf, size);
                CLIT_output("[%d,%d]",PPlus_SUCCESS,crc);
            }
        }
    }

    return 0;
}

//return write res_status and cksum of file
uint16_t fst_write(uint32_t argc, uint8_t* argv[])
{		
    uint32_t free_size;
    hal_fs_get_free_size(&free_size);
    int iret;
    uint16_t fid =(uint16_t)CLI_strtoi(argv[0], strlen((const char*)argv[0]), 10);
    uint16_t size =(uint16_t)CLI_strtoi(argv[1], strlen((const char*)argv[1]), 10);
    uint8_t x_flag = (uint8_t)(osal_rand()&0xff);
    uint16_t crc=0;

    if(size <= free_size)
    {
        for(int i = 0; i<size; i++)
        {
            fs_test_buf[i] = (uint8_t)(osal_rand()&0xff);
        }

        crc = crc16(0, fs_test_buf, size);
        fs_test_buf[size] = x_flag;
        iret =  hal_fs_item_write_entry(fid, fs_test_buf, size);

        if(iret)
        {
            CLIT_output("[%d,0]",iret);
        }
        else
        {
            if(x_flag != fs_test_buf[size])
            {
                CLIT_output("[%d, %d]",PPlus_ERR_INTERNAL,PPlus_ERR_DATA_CORRUPTION);
            }
            else
            {
                CLIT_output("[%d, %d]",PPlus_SUCCESS,crc);
            }
        }
    }
    else
    {
        CLIT_output("[%d, 0]",PPlus_ERR_FS_NOT_ENOUGH_SIZE);
    }

    return 0;
}

//return read time of file
uint16_t fst_read_time(uint32_t argc, uint8_t* argv[])
{
    uint16_t fid =(uint16_t)CLI_strtoi(argv[0], strlen((const char*)argv[0]), 10);
    uint16_t size =(uint16_t)CLI_strtoi(argv[1], strlen((const char*)argv[1]), 10);
    uint16_t rd_size = size;
    int ret;
    uint32 t0 = hal_systick();
    ret = hal_fs_item_read(fid, fs_test_buf, 16*1024, &rd_size);
    uint32 t1= hal_systick()-t0;
    CLIT_output("[%d, %d]",t1, ret);
    return 0;
}

//return write res_status and cksum of file
uint16_t fst_write_time(uint32_t argc, uint8_t* argv[])
{
    uint32_t free_size;
    hal_fs_get_free_size(&free_size);
    int iret;
    uint16_t fid =(uint16_t)CLI_strtoi(argv[0], strlen((const char*)argv[0]), 10);
    uint16_t size =(uint16_t)CLI_strtoi(argv[1], strlen((const char*)argv[1]), 10);

    if(size <= free_size)
    {
        for(int i = 0; i<16*1024; i++)
        {
            fs_test_buf[i] = (uint8_t)(osal_rand()&0xff);
        }

        uint32 t0 = hal_systick();
        iret = hal_fs_item_write_entry(fid, fs_test_buf, size);
        uint32 t1= hal_systick()-t0;
        CLIT_output("[%d, %d]",t1,iret);
    }

    return 0;
}

//return del file status
uint16_t fst_del(uint32_t argc, uint8_t* argv[])
{
    uint16_t fid =(uint16_t)CLI_strtoi(argv[0], strlen((const char*)argv[0]), 10);
    uint16_t iret;
    iret = hal_fs_item_del_entry(fid);
    CLIT_output("[%d, 0]",iret);
    return 0;
}

//return del file status
uint16_t fst_ass_s(uint32_t argc, uint8_t* argv[])
{
    uint16_t fid =(uint16_t)CLI_strtoi(argv[0], strlen((const char*)argv[0]), 10);
    uint16_t iret;
    iret = fss_assert(fid);
    CLIT_output("[%d, 0]",iret);
    return 0;
}


//shadow read
uint16_t fst_read_s( uint32_t argc, uint8_t* argv[])
{
    uint16_t fid =(uint16_t)CLI_strtoi(argv[0], strlen((const char*)argv[0]), 10);
    uint16_t size =(uint16_t)CLI_strtoi(argv[1], strlen((const char*)argv[1]), 10);
    uint8_t x_flag = (uint8_t)(osal_rand()&0xff);
    fs_test_buf[size] = x_flag;
    int iret = fss_read(fid, size, (void*)fs_test_buf);

    if(iret)
    {
        CLIT_output("[%d, 0]",iret);
    }
    else
    {
        {
            if(x_flag !=fs_test_buf[size])
            {
                CLIT_output("[%d, %d]",PPlus_ERR_INTERNAL,PPlus_ERR_DATA_CORRUPTION);
            }
            else
            {
                uint16_t crc=0;
                crc = crc16(0, fs_test_buf, size);
                CLIT_output("[%d,%d]",PPlus_SUCCESS,crc);
            }
        }
    }

    return 0;
}

//return write res_status and cksum of file
//shadow write
uint16_t fst_write_s(uint32_t argc, uint8_t* argv[])
{
    uint32_t free_size;
    hal_fs_get_free_size(&free_size);
    int iret;
    uint16_t fid =(uint16_t)CLI_strtoi(argv[0], strlen((const char*)argv[0]), 10);
    uint16_t size =(uint16_t)CLI_strtoi(argv[1], strlen((const char*)argv[1]), 10);
    uint8_t x_flag = (uint8_t)(osal_rand()&0xff);
    uint16_t crc=0;

    if(size <= free_size)
    {
        for(int i = 0; i<size; i++)
        {
            fs_test_buf[i] = (uint8_t)(osal_rand()&0xff);
        }

        crc = crc16(0, fs_test_buf, size);
        fs_test_buf[size] = x_flag;
        iret =  fss_write(fid, size, fs_test_buf);

        if(iret)
        {
            CLIT_output("[%d,0]",iret);
        }
        else
        {
            if(x_flag != fs_test_buf[size])
            {
                CLIT_output("[%d, %d]",PPlus_ERR_INTERNAL,PPlus_ERR_DATA_CORRUPTION);
            }
            else
            {
                CLIT_output("[%d, %d]",PPlus_SUCCESS,crc);
            }
        }
    }
    else
    {
        CLIT_output("[%d, 0]",PPlus_ERR_FS_NOT_ENOUGH_SIZE);
    }

    return 0;
}

//return del file status
uint16_t fst_del_s(uint32_t argc, uint8_t* argv[])
{
    uint16_t fid =(uint16_t)CLI_strtoi(argv[0], strlen((const char*)argv[0]), 10);
    uint16_t iret;
    iret = fss_del(fid);
    CLIT_output("[%d, 0]",iret);
    return 0;
}


//return available free size
uint16_t fst_free(uint32_t argc, uint8_t* argv[])
{ 
    uint32_t free_size;
    if (0==hal_fs_get_free_size_entry(&free_size))
		{
			CLIT_output("[%d, 0]",free_size);
		}
    /*
        uint32_t total_size = FS_SECTOR_NUM * FS_SECTOR_NUM_BUFFER_SIZE;


        if( free_size> total_size)
        {

            CLIT_output("[%d, 0]",PPlus_ERR_FATAL);
        }
        else if(free_size ==0)

        {

            CLIT_output("[%d, 0]",PPlus_ERR_FS_FULL);
        }
        else
        {

            CLIT_output("[%d, 0]",PPlus_SUCCESS);

            CLIT_output("[%d, %d]",free_size, total_size);
        }
    */
    return 0;
}

//return garbage_size and garbage_count
uint16_t fst_garbage(uint32_t argc, uint8_t* argv[])
{
    uint32_t  garbage_size, garbage_count;
    garbage_size = hal_fs_get_garbage_size_entry(&garbage_count);
    CLIT_output("[%d, %d]",garbage_size, garbage_count);
    return 0;
}


//garbage_collect and return available free size
uint16_t fst_clean(uint32_t argc, uint8_t* argv[])
{
    int iret = hal_fs_garbage_collect_entry();
    uint32_t size = 0;
    hal_fs_get_free_size(&size);
    CLIT_output("[%d, %d]", iret, size);
    return 0;
}

//uint16_t fst_trigger_break(uint32_t argc, uint8_t* argv[])
//{
//		_pos =(uint32_t)CLI_strtoi(argv[0], strlen((const char*)argv[0]), 16);
//		_cnt_trigger =(uint8_t)CLI_strtoi(argv[1], strlen((const char*)argv[1]), 10);

//		_cnt = 0;

//		CLIT_output("[%d]", 0);

//		return 0;
//}

//uint16_t fst_trigger_destory(uint32_t argc, uint8_t* argv[])
//{
//		_pos =(uint32_t)CLI_strtoi(argv[0], strlen((const char*)argv[0]), 16);
//		_cnt_trigger =(uint8_t)CLI_strtoi(argv[1], strlen((const char*)argv[1]), 10);
//LOG("get = %#x,%d \n",_pos,_cnt_trigger);
//		_cnt = 0;

//		CLIT_output("[%d]", 0);

//		return 0;
//}


//format and eraser all files

uint16_t fst_format(uint32_t argc, uint8_t* argv[])
{
#ifdef FS_USE_RAID
   int result = fsr_format(FS_OFFSET_ADDRESS, BACKUP_BASE_ADDR,FS_SECTOR_NUM);
	#else
	 int result = hal_fs_format(FS_OFFSET_ADDRESS, FS_SECTOR_NUM);
	#endif
    CLIT_output("[%d, 0]",result);
    return 0;
}

extern fs_raid_t fs_raid;
uint16_t fst_init(uint32_t argc, uint8_t* argv[])
{
	  int ret;

    if(hal_fs_initialized() == FALSE)
    {
			#ifdef FS_USE_RAID
         ret = hal_fs_init_entry(FS_OFFSET_ADDRESS,BACKUP_BASE_ADDR,FS_SECTOR_NUM);
			#else	
				 hal_fs_ctx(&fs_raid.fs_m);
				 ret = hal_fs_init_entry(FS_OFFSET_ADDRESS,FS_SECTOR_NUM);
			#endif
    }
    CLIT_output("[%d, 0]",ret);
		return 0;
}

//uint16_t fst_check_reset_flag(uint32_t argc, uint8_t* argv[])
//{
//	int ret;
//	ret = fsr_check_reset_flag();

//	CLIT_output("[%d, 0]",ret);
//	return 0;
//}


CLI_COMMAND fst_cmd_list[] =
{
    {"?",           "Help",         fst_help    },
    {"help",        "Help",         fst_help    },

    {"fst_rst",     "fst_reset",    fst_rst     },//reset board
    //{"fst_l",       "fst_flist",    fst_flist   },//file list
    {"fst_r",       "fst_read",     fst_read    },//read file
    {"fst_w",       "fst_write",    fst_write   },//write file
    {"fst_d",       "fst_del",      fst_del     },//delete file
    {"fst_as",      "fst_assert_s", fst_ass_s   },//shadow file assert crc error
    {"fst_rs",      "fst_read_s",   fst_read_s  },//read shadow file
    {"fst_ws",      "fst_write_s",  fst_write_s },//write shadow file
    {"fst_ds",      "fst_del_s",    fst_del_s   },//delete shadow file
    {"fst_free",    "fst_free",     fst_free    },//get free size
    {"fst_g",       "fst_garbage",  fst_garbage },//get garbage information
    {"fst_gc",      "fst_clean",    fst_clean   },//garbage collection
    {"fst_fmt",     "fst_format",   fst_format  },//fs format
    //{"fst_info",    "fst_info",     fst_info    },//fs info, get fs size, free size, item size and sector number
//    {"fst_trg_brk",     "fst_trigger_break",  fst_trigger_break },//trigger a break when do write or do garbage collect
//		{"fst_trg_dsy",     "fst_trigger_destory",  fst_trigger_destory },//trigger a data destory when do write or do garbage collect
    {"fst_rt",      "fst_read_time",     fst_read_time    },//read file time
    {"fst_wt",      "fst_write_time",    fst_write_time   },//write file time
	  //{"fst_cek",      "fst_check",    fst_check_reset_flag   },//check_init
};

static uint16_t fst_help(uint32_t argc, uint8_t* argv[])
{
    CLI_help(fst_cmd_list, sizeof(fst_cmd_list)/sizeof(CLI_COMMAND));
    return 0;
}


void fst_cmd_register(void)
{
    CLI_init(fst_cmd_list, sizeof(fst_cmd_list)/sizeof(CLI_COMMAND));	
    fst_init(0,0);
}
