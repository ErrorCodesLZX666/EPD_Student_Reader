#ifndef _FAT_APP_H
#define _FAT_APP_H

#include "gd32f4xx.h"
#include "ff.h"
#include <stdint.h>
#include <stdbool.h>

// 兼容原代码中的短类型定义
#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t

// 磁盘类型枚举（0: SD 卡, 1: SPI Flash）
typedef enum {
    DISK_SD = 0,
    DISK_SPI = 1,
} DiskType_e;

// 磁盘信息结构：初始化后可读取
typedef struct {
    bool     present;    // 设备是否存在（初始化成功并挂载或有效）
    bool     mounted;    // 是否已挂载
    u32      total_kb;   // 总容量，单位 KB（0 表示未知/失败）
    u32      free_kb;    // 空闲容量，单位 KB
} DiskInfo_t;

// 导出给外部模块使用的磁盘信息
extern DiskInfo_t disk_sd;
extern DiskInfo_t disk_spi;

// FatFs 工作区与临时对象（由 exfuns_init 分配）
extern FATFS *fs[_VOLUMES]; // 每个逻辑卷的工作区
extern FIL   *filin;
extern FIL   *filout;
extern FILINFO *tfileinfo;
extern DIR   *picdir;
extern u8    pname[_MAX_LFN + 1];
extern u8    buf_r[512];    // 通用读缓冲

// 函数接口
// 为 FatFs 分配必要的工作区（必须在调用挂载之前执行）
u8 exfuns_init(void);

// 初始化并挂载 SD（若无文件系统则尝试格式化一次并重新挂载）
// 成功或失败信息反映在 disk_sd 中
void sd_fatfs_init(void);

// 初始化并挂载 SPI Flash（同上）
// 成功或失败信息反映在 disk_spi 中
void spi_fatfs_init(void);

// 获取指定驱动盘符的总/空闲容量（drv 格式示例："0:" 或 "1:"）
// 返回 FR_OK(0) 表示成功；total/free 以 KB 为单位
u8 exf_getfree(char *drv, u32 *total_kb, u32 *free_kb);

// 获取当前的磁盘信息
u8 getDiskInfo(DiskInfo_t* flash_disk,DiskInfo_t* sd_disk);





// 下面是自定义的 =======================================
// 获取某一个目录总的所有txt文件
void sd_read_dir_txt(const char* path, char ***text, int *count);

//=======================================
// 读取一个文件的部分内容
FRESULT sd_read_range(const char *path, FSIZE_t start, FSIZE_t end, char *buff, UINT buff_size, UINT *bytes_read);

//=======================================
// 加载保存某一本图书的索引文件
typedef struct {
    uint32_t total_bytes;
    uint32_t total_pages;
    uint32_t current_page;
    uint32_t current_bytes;
} NovelIndex;
int save_novel_index(const char *path, NovelIndex *index);
int load_novel_index(const char *path, NovelIndex *index);
//=======================================
// 计算某一个小说的文件索引
FRESULT Novel_CalcIndex(const char *path, uint8_t font_size, NovelIndex *index);

#endif // _FAT_APP_H
