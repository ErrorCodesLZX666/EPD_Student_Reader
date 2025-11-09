#include "fat_app.h"
#include "malloc.h" // mymalloc
#include "ff.h"
#include <string.h>
#include <stdio.h>
#include "log.h"
#include "APP_GUI.h"
#include <stdlib.h>

// 如果在工程中需要 sdcard/w25qxx 初始化，请确保这两个头文件存在
// 并且实现了 sd_io_init() 和 W25QXX_Init()
#include "sdcard.h"
#include "w25qxx.h"

// 导出变量定义
FATFS *fs[_VOLUMES];
FIL *filin;
FIL *filout;
FILINFO *tfileinfo;
DIR *picdir;
u8 pname[_MAX_LFN + 1];
u8 buf_r[512];

// 磁盘信息
DiskInfo_t disk_sd = {0};
DiskInfo_t disk_spi = {0};

/*
 * exfuns_init
 * 为 FatFs 分配所需工作区与临时对象（使用工程内的 mymalloc）
 * 返回 0 成功，非 0 失败
 */
u8 exfuns_init(void)
{
    uint8_t i;

    // 检查 _VOLUMES 合理性（至少 1）
#if _VOLUMES < 1
#error "_VOLUMES must be >= 1"
#endif

    // 为每个卷分配 FATFS 工作区
    for (i = 0; i < _VOLUMES; i++)
    {
        fs[i] = (FATFS *)mymalloc(SRAMIN, sizeof(FATFS));
        if (fs[i] == NULL)
        {
            // 若分配失败，需回收已分配的并返回错误
            for (; i > 0; i--)
            {
                // note: 没有 myfree 函数原型，此处仅示意
                // myfree(fs[i-1]);
                fs[i - 1] = NULL;
            }
            return 1;
        }
        memset(fs[i], 0, sizeof(FATFS));
    }

    filin = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    filout = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
    picdir = (DIR *)mymalloc(SRAMIN, sizeof(DIR));
    tfileinfo = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO));

    if (!filin || !filout || !picdir || !tfileinfo)
    {
        // 简单处理：将已分配的指针设为 NULL（具体回收由上层管理）
        if (filin)
        { /* myfree(filin); */
            filin = NULL;
        }
        if (filout)
        { /* myfree(filout); */
            filout = NULL;
        }
        if (picdir)
        { /* myfree(picdir); */
            picdir = NULL;
        }
        if (tfileinfo)
        { /* myfree(tfileinfo); */
            tfileinfo = NULL;
        }
        return 2;
    }

    memset(filin, 0, sizeof(FIL));
    memset(filout, 0, sizeof(FIL));
    memset(picdir, 0, sizeof(DIR));
    memset(tfileinfo, 0, sizeof(FILINFO));
    memset(pname, 0, sizeof(pname));

    return 0;
}

/*
 * exf_getfree
 * 使用 FatFs 的 f_getfree 获取磁盘总/空闲扇区并转换为 KB 单位返回
 * drv 示例 "0:" 或 "1:"
 */
u8 exf_getfree(char *drv, u32 *total_kb, u32 *free_kb)
{
    FATFS *fs_ptr;
    DWORD free_clust;
    FRESULT res;

    *total_kb = 0;
    *free_kb = 0;

    res = f_getfree((const TCHAR *)drv, &free_clust, &fs_ptr);
    if (res != FR_OK)
    {
        return (u8)res;
    }

    // 计算扇区数
    // total sectors = (n_fatent - 2) * csize
    // 每簇扇区数 = csize
    // fs_ptr->ssize 是扇区字节数（若 _MAX_SS != 512 需要换算）
    {
        uint32_t tot_sect = (fs_ptr->n_fatent - 2) * fs_ptr->csize;
        uint32_t free_sect = (uint32_t)free_clust * fs_ptr->csize;
        // 输出分区
        LOGD("Calculate touched, Totoal Section = %d; Free Section = %d;\r\n", tot_sect, free_sect);

#if _MAX_SS != 512
        // 转换到以 512 字节扇区为基准
        tot_sect = (tot_sect * fs_ptr->ssize) / 512;
        free_sect = (free_sect * fs_ptr->ssize) / 512;
#endif
        // 每扇区 512 字节 -> 转换为 KB（512 bytes = 0.5 KB），因此 tot_sect>>1
        *total_kb = tot_sect >> 1;
        *free_kb = free_sect >> 1;
    }

    return (u8)res;
}

/*
 * 封装：尝试挂载驱动；若挂载失败则尝试格式化一次再挂载
 * drv 示例 "0:" / "1:"
 * work_fs 指向 fs[] 中的工作区
 * 返回 FR_OK(0) 表示最终挂载成功，非 0 表示失败
 */
static FRESULT fatfs_mount_with_format_retry(const TCHAR *drv, FATFS *work_fs)
{
    FRESULT res;

    // 第一次尝试挂载（如果已有文件系统会成功）
    res = f_mount(work_fs, drv, 1);
    if (res == FR_OK)
    {
        return res;
    }

    // 挂载失败：尝试格式化一次（默认分配簇大小由 f_mkfs 的 second 参数决定）
    // 注意：f_mkfs 的参数依实现而定，本处使用 0 表示默认参数（若你的 FatFs 需要不同参数请修改）
    res = f_mkfs(drv, 0, 0);
    if (res != FR_OK)
    {
        return res;
    }

    // 格式化成功后再次挂载
    res = f_mount(work_fs, drv, 1);
    return res;
}

/*
 * sd_fatfs_init
 * 初始化 SD 底层（调用 sd_io_init），然后尝试挂载 SD（"0:"）
 * 若挂载失败则尝试格式化一次并重新挂载
 * 最后更新 disk_sd 结构体
 */
void sd_fatfs_init(void)
{
    disk_sd.present = false;
    disk_sd.mounted = false;
    disk_sd.total_kb = 0;
    disk_sd.free_kb = 0;

    // 初始化 SD 底层（该函数由你的 sdcard 驱动提供）
    sd_error_enum err = sd_io_init();
    if (err != SD_OK)
    {
        LOGE("SDIO init failed... ERROR_CODE = %d\r\n", err);
        // SD 初始化失败
        return;
    }

    // 尝试挂载并在必要时格式化重试
    FRESULT result = fatfs_mount_with_format_retry("0:", fs[0]);
    if (result == FR_OK)
    {
        disk_sd.present = true;
        disk_sd.mounted = true;
        LOGD("0: Mount successful! \r\n");
        // 获取容量信息
        exf_getfree("0:", &disk_sd.total_kb, &disk_sd.free_kb);
    }
    else
    {
        // 挂载/格式化失败，标记为未挂载（present 保持 false）
        disk_sd.present = false;
        disk_sd.mounted = false;
        LOGD("0: Mount failed! \r\n");
    }
}

/*
 * spi_fatfs_init
 * 初始化 SPI Flash（调用 W25QXX_Init），随后尝试挂载 SPI 所在的逻辑盘（"1:"）
 * 若挂载失败则尝试格式化一次并重新挂载
 * 最后更新 disk_spi 结构体
 */
void spi_fatfs_init(void)
{
    disk_spi.present = false;
    disk_spi.mounted = false;
    disk_spi.total_kb = 0;
    disk_spi.free_kb = 0;

    // 初始化 SPI Flash 底层（W25QXX_Init 由你的 SPI flash 驱动提供）
    uint8_t w25Res = W25QXX_Init();
    if (w25Res == 0)
    {

        LOGE("Flash Init Failed... ERROR_CODE=%d\r\n", w25Res);
        // 初始化失败（W25QXX_Init 返回非零表示成功，0 表示失败）
        return;
    }

    // 尝试挂载并在必要时格式化重试
    FRESULT result = fatfs_mount_with_format_retry("1:", fs[1]);
    if (result == FR_OK)
    {
        LOGD("0: Mount successful! \r\n");
        disk_spi.present = true;
        disk_spi.mounted = true;
        exf_getfree("1:", &disk_spi.total_kb, &disk_spi.free_kb);
    }
    else
    {
        LOGD("0: Mount failed!  ERROR_CODES = %d\r\n", result);
        disk_spi.present = false;
        disk_spi.mounted = false;
    }
}

u8 getDiskInfo(DiskInfo_t *flash_disk, DiskInfo_t *sd_disk)
{
    memcpy(flash_disk, &disk_spi, sizeof(disk_spi));
    memcpy(sd_disk, &disk_sd, sizeof(disk_sd));
    return 0;
}

void sd_read_dir_txt(const char *path, char ***text, int *count)
{
    DIR dir;
    FILINFO fno;
    FRESULT res;

    *text = NULL;
    *count = 0;

    // 打开目录
    res = f_opendir(&dir, path);
    if (res != FR_OK)
    {
        LOGE("Open dir failed : %d\r\n", res);
        return;
    }

    // 遍历文件
    while (1)
    {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0)
            break; // 结束
        if (fno.fattrib & AM_DIR)
            continue; // 跳过子目录

        // 检查是否为 .txt 文件
        const char *ext = strrchr(fno.fname, '.');
        if (ext && (!strcmp(ext, ".txt") || !strcmp(ext, ".TXT")))
        {

            // 扩展文件名列表
            char **new_list = realloc(*text, (*count + 1) * sizeof(char *));
            if (!new_list)
            {
                LOGE("Memory allocation failed!\r\n");
                break;
            }
            *text = new_list;

            // 为单个文件名分配内存并复制
            (*text)[*count] = malloc(strlen(fno.fname) + 1);
            if (!(*text)[*count])
            {
                LOGE("Memory allocation failed!\r\n");
                break;
            }
            strcpy((*text)[*count], fno.fname);
            (*count)++;
        }
    }

    f_closedir(&dir);
}

/**
 * @brief 从文件中读取指定范围的数据
 * @param path        文件路径，如 "0:/novel.txt"
 * @param start       起始字节偏移（从 0 开始）
 * @param end         结束字节偏移（不含 end）
 * @param buff        输出缓冲区
 * @param buff_size   缓冲区大小
 * @param bytes_read  返回实际读取字节数
 * @retval FRESULT    FatFs 状态码（FR_OK 表示成功）
 */
FRESULT sd_read_range(const char *path, FSIZE_t start, FSIZE_t end, char *buff, UINT buff_size, UINT *bytes_read)
{

    FIL file;
    FRESULT res;
    UINT br = 0;
    FSIZE_t file_size;
    FSIZE_t read_len;

    *bytes_read = 0;

    // 打开文件只读
    res = f_open(&file, path, FA_READ);
    if (res != FR_OK)
    {
        LOGE("File open failed: %d\r\n", res);
        return res;
    }

    file_size = f_size(&file);

    // 参数校验
    if (start >= file_size)
    {
        LOGE("Start offset exceeds file size\r\n");
        f_close(&file);
        return FR_INVALID_PARAMETER;
    }
    if (end > file_size)
        end = file_size;
    if (end <= start)
    {
        LOGE("Invalid byte range\r\n");
        f_close(&file);
        return FR_INVALID_PARAMETER;
    }

    read_len = end - start;
    if (read_len > buff_size)
        read_len = buff_size; // 避免越界

    // 移动文件指针到 start 位置
    res = f_lseek(&file, start);
    if (res != FR_OK)
    {
        LOGE("f_lseek failed: %d\r\n", res);
        f_close(&file);
        return res;
    }

    // 读取指定范围数据
    res = f_read(&file, buff, read_len, &br);
    if (res != FR_OK)
    {
        LOGE("f_read failed: %d\r\n", res);
    }
    else
    {
        *bytes_read = br;
        buff[br] = '\0'; // 确保字符串结尾
    }

    f_close(&file);
    return res;
}



// 写入小说索引
int save_novel_index(const char *path, NovelIndex *index)
{
    FIL file;
    FRESULT res;

    res = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        LOGE("Open file failed: %s (%d)\r\n", path, res);
        return -1;
    }
    LOGI("Open index file successfull : %s\r\n", path);

    UINT bw;
    res = f_write(&file, index, sizeof(NovelIndex), &bw);
    LOGI("write finished.\r\n");
    if (res != FR_OK || bw != sizeof(NovelIndex)) {
        LOGE("Write index failed: %s\r\n", path);
        f_close(&file);
        return -2;
    }
    LOGI("Index saved: total_bytes=%lu, total_pages=%lu, file_path=%s\r\n",
         (unsigned long)index->total_bytes,
         (unsigned long)index->total_pages,
         path);
    f_close(&file);
    return 0;
}

int load_novel_index(const char *path, NovelIndex *index)
{
    FIL file;
    FRESULT res;
    UINT br;

    res = f_open(&file, path, FA_READ);
    if (res != FR_OK) {
        LOGE("Index file is not found: %s\r\n", path);
        memset(index, 0, sizeof(NovelIndex));
        return -1; // 未找到文件
    }

    res = f_read(&file, index, sizeof(NovelIndex), &br);
    f_close(&file);

    if (res != FR_OK || br != sizeof(NovelIndex)) {
        LOGE("Read index file failed: %s\r\n", path);
        memset(index, 0, sizeof(NovelIndex));
        return -2;
    }

    return 0; // 成功
}



/**
 * @brief  计算整本小说的页数（基于 UI_CalcReaderPageChars）
 * @param  path        小说文件路径，如 "0:/novel.txt"
 * @param  font_size   字体大小
 * @param  index       输出索引信息结构体（会被写入 total_bytes / total_pages）
 * @retval FRESULT     FatFs 结果码（FR_OK 表示成功）
 */
FRESULT Novel_CalcIndex(const char *path, uint8_t font_size, NovelIndex *index)
{
    FIL file;
    FRESULT res;
    UINT bytes_read;
    FSIZE_t file_size;
    FSIZE_t offset = 0;
    uint32_t total_pages = 0;
    // 分配一个临时缓冲区用于分页计算
    static char read_buf[2048]; // 可根据内存情况调大或调小

    // 打开文件
    res = f_open(&file, path, FA_READ);
    if (res != FR_OK) {
        LOGE("Open File failed: %s (%d)\r\n", path, res);
        return res;
    }

    // 获取文件总大小
    file_size = f_size(&file);
    index->total_bytes = (uint32_t)file_size;
    index->total_pages = 0;
    index->current_page = 0;

    LOGI("开始计算分页，总字节数: %lu\r\n", (unsigned long)file_size);

    // 定义计算了好多页
    uint32_t calculated_pages = 0;

    // 主循环：遍历整个文件
    while (offset < file_size) {
        // 每次读取一段数据
        FSIZE_t end = offset + sizeof(read_buf);
        if (end > file_size) end = file_size;

        res = sd_read_range(path, offset, end, read_buf, sizeof(read_buf), &bytes_read);
        if (res != FR_OK || bytes_read == 0) {
            LOGE("读取出错或结束: offset=%lu res=%d\r\n", (unsigned long)offset, res);
            break;
        }

        // 计算当前页能显示多少个字符
        uint16_t used_bytes = UI_CalcReaderPageBytes(read_buf, font_size);
        // 初始化当前map的内存区域,并且填充对于的值
        //index->page_bytes_map[calculated_pages] = mymalloc(SRAMIN, sizeof(uint16_t));
        if (used_bytes == 0) {
            // 理论上不可能为0，防止死循环  
            LOGE("分页异常，used_chars=0 在 offset=%lu\r\n", (unsigned long)offset);
            break;
        }

        offset += used_bytes;
        total_pages++;
        calculated_pages++;
        // 防止最后一页没有读完的尾页被跳过
        if (offset >= file_size) break;
    }

    f_close(&file);

    index->total_pages = total_pages;
    LOGI("分页完成，总页数: %lu\r\n", (unsigned long)index->total_pages);
    return FR_OK;
}