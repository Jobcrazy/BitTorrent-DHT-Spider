#ifndef HEADER_P2SP_API
#define HEADER_P2SP_API

typedef unsigned int DWORD;
typedef DWORD UINT;
typedef unsigned long long UINT64;
typedef DWORD HANDLE;
typedef char CHAR;
typedef unsigned int BOOL;
typedef int INT;
typedef void VOID;
#define TRUE true
#define FALSE false
#define INVALID_HANDLE_VALUE -1
#define DUPLICATE_TASK_HANDLE -2
#define MAX_PATH 260
#define WINAPI
#include <stdarg.h>
#define SLEEP usleep

struct TASK_INFO
{
    UINT status;
    UINT64 size_total;
    UINT64 size_done;
    float percent;
    UINT speed_down;
    UINT speed_up;
    UINT peers_all;
    UINT seeds_all;
    UINT peers_conn;
    UINT seeds_conn;
};

struct TASK_FILE_INFO
{
    char szName[MAX_PATH];
    char szPathName[MAX_PATH];
    UINT priority;
    unsigned long long file_size;
    unsigned long long file_down;
};

enum state_t
{
    queued_for_checking,    //等待检查文件进度
    checking_files,         //检查已经下载的文件进度
    downloading_metadata,   //下载磁力链数据
    downloading,            //下载中
    finished,               //下载完成尚未开始做种
    seeding,                //下载完成开始做种
    allocating,             //分配磁盘空间
    checking_resume_data,   //检查快速恢复文件
    pause_s,                  //下载已暂停
    error,                  //下载过程出错暂停
    pending                 //正在暂停或正在停止
};