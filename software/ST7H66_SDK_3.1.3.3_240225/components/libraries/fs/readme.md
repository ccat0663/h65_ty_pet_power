选择双fs or 单fs 通过宏控制，即在fs.h中定义FS_USE_RAID 则选用双fs，无定义FS_USE_RAID则选用单fs;
用户自定义fs的地址，包含fs的地址（FS_OFFSET_ADDRESS）以及备份的地址（BACKUP_BASE_ADDR）；
默认fs的sector size = 4KB；
使用单fs时，需先调用API:hal_fs_ctx，给指针结构体一个指向；
