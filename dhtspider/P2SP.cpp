#define MAX_JOBS 5000
#define DOWNLOAD_DIR "/dev/null"
#define TMP_DIR "/mnt/d/webdata/torrents"
#define UPLOAD_DIR "/root/torup/"

#define DB_HOST "127.0.0.1"
#define DB_PORT 3306
#define DB_DATABASE "dbooster_torrents"
#define DB_USER "dbooster"
#define DB_PASSWORD "UxtmU6w3wNjYUMvV"

#include <stdio.h>
#include <list>
#include <vector>
#include <set>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <mysql.h>

#include "P2SP.h"
#include "libtorrent/utf8.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/peer_info.hpp"

using namespace std;
using namespace libtorrent;
using namespace boost;

struct HASH_HANDLE
{
    sha1_hash info_hash;
    DWORD hTorrent;
};

struct SETTINGS
{
    BOOL bInited;
    libtorrent::session* ses;
    std::map<DWORD, libtorrent::torrent_handle> handle_map;
    std::list<HASH_HANDLE> list_hash;
    libtorrent::dht_settings dht_set;
    libtorrent::session_settings ses_set;
    libtorrent::pe_settings pe_set;
    int outstanding_resume_data;
};

SETTINGS g_Settings = {0};
bool g_bNewTaskAdd = false;
unsigned int g_nFileProcessed = 0;

pthread_mutex_t g_ThreadLocker;
#define INIT_THREAD_LOCKER pthread_mutex_init(&g_ThreadLocker, 0);
#define DESTROY_THREAD_LOCKER pthread_mutex_destroy(&g_ThreadLocker);
#define LOCK_THREAD pthread_mutex_lock(&g_ThreadLocker);
#define UNLOCK_THREAD pthread_mutex_unlock(&g_ThreadLocker);

void TextLog( const char* fmt, ... )
{
#ifdef LOG
    va_list ap;
    va_start(ap, fmt);

    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");

    va_end(ap);

    return;
#endif
}

extern "C" BOOL WINAPI BT_Init( )
{
    LOCK_THREAD;
    if ( g_Settings.bInited )
    {
        UNLOCK_THREAD;
        return TRUE;
    }

    libtorrent::error_code ec;

    g_Settings.ses = new session
        (
        fingerprint("LT", LIBTORRENT_VERSION_MAJOR, LIBTORRENT_VERSION_MINOR, 0, 0)
        , session::add_default_plugins
        , alert::dht_notification | alert::status_notification
        );

    if ( !g_Settings.ses )
    {
        UNLOCK_THREAD;
        return FALSE;
    }

    //DHT相关设置
    g_Settings.ses_set.use_dht_as_fallback = false;

    //是否允许同一台机器的多个连接
    g_Settings.ses_set.allow_multiple_connections_per_ip = false;

    //在内网进行广播，用于组播无法使用的情况
    g_Settings.ses_set.broadcast_lsd = false;

    //不设置半开连接数的限制
    g_Settings.ses_set.half_open_limit = -1;
    g_Settings.ses_set.connection_speed = 50;
    g_Settings.ses_set.dht_upload_rate_limit = 20000;
    g_Settings.ses_set.recv_socket_buffer_size = 1024 * 1024;
    g_Settings.ses_set.send_socket_buffer_size = 1024 * 1024;
    g_Settings.ses_set.optimize_hashing_for_speed = true;
    g_Settings.ses_set.request_timeout = 10;
    //g_Settings.ses_set.peer_timeout = 20;
    //g_Settings.ses_set.inactivity_timeout = 20;

    g_Settings.ses_set.max_out_request_queue = 15000;
    g_Settings.ses_set.max_allowed_in_request_queue = 20000;
    g_Settings.ses_set.mixed_mode_algorithm = session_settings::prefer_tcp;
    g_Settings.ses_set.alert_queue_size = 50000;
    g_Settings.ses_set.file_pool_size = 500;
    g_Settings.ses_set.no_atime_storage = true;
    g_Settings.ses_set.connections_limit = 8000;
    g_Settings.ses_set.listen_queue_size = 2000;
    g_Settings.ses_set.unchoke_slots_limit = 500;
    g_Settings.ses_set.read_job_every = 1000;

    //g_Settings.ses_set.cache_size = 32768 * 2; //内存做磁盘缓存，太大了不好，用默认的16M即可
    g_Settings.ses_set.use_read_cache = true;
    g_Settings.ses_set.cache_buffer_chunk_size = 128;
    g_Settings.ses_set.read_cache_line_size = 32;
    g_Settings.ses_set.write_cache_line_size = 32;
    g_Settings.ses_set.low_prio_disk = false;
    g_Settings.ses_set.cache_expiry = 60 * 60;
    g_Settings.ses_set.lock_disk_cache = false;
    g_Settings.ses_set.max_queued_disk_bytes = 10 * 1024 * 1024;
    g_Settings.ses_set.disk_cache_algorithm = session_settings::avoid_readback;
    g_Settings.ses_set.explicit_read_cache = false;
    g_Settings.ses_set.allowed_fast_set_size = 0;
    g_Settings.ses_set.suggest_mode = session_settings::suggest_read_cache;
    g_Settings.ses_set.close_redundant_connections = true;
    g_Settings.ses_set.max_rejects = 10;
    g_Settings.ses_set.active_limit = MAX_JOBS;
    g_Settings.ses_set.active_tracker_limit = 2000;
    g_Settings.ses_set.active_dht_limit = MAX_JOBS;
    g_Settings.ses_set.active_seeds = 10;
    g_Settings.ses_set.choking_algorithm = session_settings::fixed_slots_choker;
    g_Settings.ses_set.send_buffer_watermark = 3 * 1024 * 1024;
    g_Settings.ses_set.send_buffer_watermark_factor = 150;
    g_Settings.ses_set.send_buffer_low_watermark = 1 * 1024 * 1024;
    g_Settings.ses_set.max_failcount = 3;
    g_Settings.ses_set.utp_dynamic_sock_buf = true;
    g_Settings.ses_set.max_http_recv_buffer_size = 6 * 1024 * 1024;
    g_Settings.ses_set.use_disk_cache_pool = true;

    //g_Settings.ses_set.dht_announce_interval = 60;

    //给Session应用设置
    g_Settings.ses->set_settings( g_Settings.ses_set );

    //加密相关的
    g_Settings.pe_set.out_enc_policy = pe_settings::enabled;
    g_Settings.pe_set.in_enc_policy = pe_settings::enabled;
    g_Settings.pe_set.allowed_enc_level = pe_settings::both;
    g_Settings.pe_set.prefer_rc4 = true;

    g_Settings.ses->set_pe_settings( g_Settings.pe_set );

    //开启端口准备开始上传和下载
    ec.clear();
    g_Settings.ses->listen_on(std::make_pair(6881, 7081), ec);

    if ( ec )
    {
        UNLOCK_THREAD;
        return FALSE;
    }

    //设置DHT路由器
    g_Settings.ses->add_dht_router(std::make_pair(
        std::string("router.bittorrent.com"), 6881));
    g_Settings.ses->add_dht_router(std::make_pair(
        std::string("router.utorrent.com"), 6881));
    g_Settings.ses->add_dht_router(std::make_pair(
        std::string("dht.transmissionbt.com"), 6881));
    g_Settings.ses->add_dht_router(std::make_pair(
        std::string("router.bitcomet.com"), 6881));

    //给Session应用设置
    g_Settings.dht_set.privacy_lookups = true;
    g_Settings.dht_set.max_dht_items = MAX_JOBS;
    g_Settings.dht_set.ignore_dark_internet = true;

    g_Settings.ses->set_dht_settings( g_Settings.dht_set );

    //开启DHT模式
    g_Settings.ses->start_dht();

    g_Settings.bInited = TRUE;

    UNLOCK_THREAD;
    return TRUE;
}

DWORD FindEmptyHandle()
{
    static DWORD hTemp = 0;
    hTemp++;
    if ( hTemp > MAX_JOBS * 2 )
    {
        hTemp = 1;
    }
    return hTemp;
}

BOOL FindSameTask( const sha1_hash & sha1Hash )
{
    if ( !g_Settings.bInited )
    {
        return FALSE;
    }

    if( g_Settings.ses->find_torrent( sha1Hash ).is_valid() )
    {
        return TRUE;
    }

    return FALSE;
}

extern "C" VOID WINAPI BT_LimitWANSpeed( INT DownLoadLimit, INT UpLoadLimit )
{
    LOCK_THREAD
    if ( !g_Settings.bInited )
    {
        UNLOCK_THREAD;
        return;
    }

    g_Settings.ses_set.upload_rate_limit = DownLoadLimit;
    g_Settings.ses_set.download_rate_limit = UpLoadLimit;
    
    g_Settings.ses->set_settings( g_Settings.ses_set );

    UNLOCK_THREAD;
    return;
}

extern "C" VOID WINAPI BT_LimitTaskSpeed( HANDLE hTorrent, INT DownLoadLimit, INT UpLoadLimit )
{
    LOCK_THREAD;
    if ( !g_Settings.bInited )
    {
        UNLOCK_THREAD;
        return;
    }

    DWORD dwTorrent = (DWORD)hTorrent;
    UpLoadLimit = UpLoadLimit < 0 ? -1 : UpLoadLimit;
    DownLoadLimit = DownLoadLimit < 0 ? -1 : DownLoadLimit;

    if ( g_Settings.handle_map.find(dwTorrent) == g_Settings.handle_map.end() )
    {
        UNLOCK_THREAD;
        return;
    }

    g_Settings.handle_map[dwTorrent].set_upload_limit( UpLoadLimit );
    g_Settings.handle_map[dwTorrent].set_download_limit( DownLoadLimit );

    UNLOCK_THREAD;
    return;
}

extern "C" VOID WINAPI BT_DeleteTask( HANDLE hTorrent )
{
    LOCK_THREAD;

    DWORD dwTorrent = (DWORD)hTorrent;

    if ( g_Settings.handle_map.find(dwTorrent) == g_Settings.handle_map.end() )
    {
        UNLOCK_THREAD;
        return;
    }

    if ( !g_Settings.handle_map[dwTorrent].is_valid() )
    {
        UNLOCK_THREAD;
        return;
    }

    g_Settings.handle_map[dwTorrent].pause();
    g_Settings.ses->remove_torrent( g_Settings.handle_map[dwTorrent] );
    g_Settings.handle_map.erase( g_Settings.handle_map.find(dwTorrent) );

    UNLOCK_THREAD;
    return;
}


extern "C" VOID WINAPI BT_Release()
{
    LOCK_THREAD;
    
    if ( !g_Settings.bInited )
    {
        UNLOCK_THREAD;
        return;
    }
    
    g_Settings.bInited = FALSE;    
    
    //删除session内的全部torrent
    std::map<DWORD, libtorrent::torrent_handle>::iterator iter = g_Settings.handle_map.begin();

    while ( iter != g_Settings.handle_map.end() )
    {
        
        iter->second.pause();
        
        while( iter->second.status().paused != true )
        {
            sleep(100);
        }
        
        g_Settings.ses->remove_torrent( iter->second );
        while( iter->second.is_valid() )
        {
            sleep(100);
        }
        g_Settings.handle_map.erase( iter++ );
    }

    g_Settings.ses->stop_dht();
    while ( g_Settings.ses->is_dht_running() )
    {
        sleep(100);
    }
    g_Settings.ses->stop_lsd();
    g_Settings.ses->stop_natpmp();
    g_Settings.ses->stop_upnp();
    g_Settings.ses->pause();
    while( g_Settings.ses->is_paused() != true )
    {
        sleep(100);
    }
    g_Settings.ses->abort();

    UNLOCK_THREAD;
}

extern "C" HANDLE WINAPI BT_Add_From_File( const CHAR* szAsciiFileName, const CHAR* szSavePath )
{
    LOCK_THREAD;
    if ( !g_Settings.bInited )
    {
        UNLOCK_THREAD;
        return INVALID_HANDLE_VALUE;
    }

    if ( !szAsciiFileName || !szSavePath )
    {
        UNLOCK_THREAD;
        return INVALID_HANDLE_VALUE;
    }

    error_code ec;
    ec.clear();

    boost::intrusive_ptr<torrent_info> t = new torrent_info(szAsciiFileName, ec);

    if ( ec )
    {
        UNLOCK_THREAD;
        return INVALID_HANDLE_VALUE;
    }

    add_torrent_params param;
    param.ti = t;
    param.auto_managed = false;
    param.upload_mode = true;
    param.share_mode = true;
    param.save_path = szSavePath;

    //看看是否有重复的HASH及保存路径
    if ( FindSameTask( t->info_hash() ) )
    {
        UNLOCK_THREAD;
        return DUPLICATE_TASK_HANDLE;
    }

    DWORD hTorrent = FindEmptyHandle();
    ec.clear();
    g_Settings.handle_map[hTorrent] = g_Settings.ses->add_torrent( param, ec );

    if ( ec )
    {
        UNLOCK_THREAD;
        return INVALID_HANDLE_VALUE;
    }

    if( g_Settings.ses->is_paused() )
    {
        g_Settings.ses->resume();
    }

    g_Settings.handle_map[hTorrent].pause();
    g_Settings.handle_map[hTorrent].resume();

    UNLOCK_THREAD;
    return (HANDLE)hTorrent;
}

extern "C" HANDLE WINAPI BT_Add_From_MagNet( const CHAR* szUrl, const CHAR* szSavePath )
{
    LOCK_THREAD;
    if ( !g_Settings.bInited )
    {
        UNLOCK_THREAD;
        return INVALID_HANDLE_VALUE;
    }

    if ( !szUrl || !szSavePath )
    {
        UNLOCK_THREAD;
        return INVALID_HANDLE_VALUE;
    }

    add_torrent_params param;
    param.save_path = szSavePath;
    param.auto_managed = false;
    param.upload_mode = true;
    param.share_mode = true;
    error_code ec;

    ec.clear();
    parse_magnet_uri(szUrl, param, ec);

    if ( ec )
    {
        UNLOCK_THREAD;
        return INVALID_HANDLE_VALUE;
    }

    //看看是否有重复的HASH及保存路径
    if ( FindSameTask( param.info_hash ) )
    {
        UNLOCK_THREAD;
        return DUPLICATE_TASK_HANDLE;
    }

    DWORD hTorrent = FindEmptyHandle();
    ec.clear();

    torrent_handle thTemp = g_Settings.ses->add_torrent( param, ec );

    if ( ec )
    {
        UNLOCK_THREAD;
        return INVALID_HANDLE_VALUE;
    }

    g_Settings.handle_map[hTorrent] = thTemp;
    HASH_HANDLE temp_hash_handle = { param.info_hash, hTorrent };
    g_Settings.list_hash.push_back( temp_hash_handle );

    if( g_Settings.ses->is_paused() )
    {
        g_Settings.ses->resume();
    }

    g_Settings.handle_map[hTorrent].pause();
    g_Settings.handle_map[hTorrent].resume();

    UNLOCK_THREAD;
    return (HANDLE)hTorrent;
}

BOOL save_torrent( torrent_handle & hTorrent, const CHAR* szFilePath )
{
    if ( !szFilePath )
    {
        return FALSE;
    }

    torrent_info ti = *hTorrent.torrent_file().get();
    create_torrent new_torrent(ti);
    std::vector<char> out;
    bencode(std::back_inserter(out), new_torrent.generate());

    FILE* fDest = fopen( szFilePath, "wb" );
    if ( !fDest )
    {
        UNLOCK_THREAD;
        return FALSE;
    }

    for ( unsigned int nIndex = 0; nIndex < out.size(); nIndex++ )
    {
        if( 1 != fwrite( &out[nIndex], 1, 1, fDest ) )
        {
            UNLOCK_THREAD;
            return FALSE;  
        }
    }
    fclose(fDest);

    return TRUE;
}

string ws2s(const wstring& ws)  
{  
    string curLocale = setlocale(LC_ALL, NULL); // curLocale = "C";  

    setlocale(LC_ALL, "chs");  

    const wchar_t* _Source = ws.c_str();  
    size_t _Dsize = 2 * ws.size() + 1;  
    char *_Dest = new char[_Dsize];  
    memset(_Dest,0,_Dsize);  
    wcstombs(_Dest,_Source,_Dsize);  
    string result = _Dest;  
    delete []_Dest;  

    setlocale(LC_ALL, curLocale.c_str());  

    return result;  
} 
int CreateDir(const   char   *sPathName)    
{    
    char   DirName[256];    
    strcpy(DirName,   sPathName);    
    int   i,len   =   strlen(DirName);    
    if(DirName[len-1]!='/')    
        strcat(DirName,   "/");      
    len   =   strlen(DirName);     
    for(i=1;   i<len;   i++)    
    {    
        if(DirName[i]=='/')    
        {    
            DirName[i]   =   0;    
            if(   access(DirName,   NULL)!=0   )    
            { 
                if(mkdir(DirName,   0755)==-1)    
                {     
                    perror("mkdir   error");     
                    return   -1;     
                }    
            }    
            DirName[i]   =   '/';    
        }    
    }    
    return   0;    
}

string get_save_dir( string & strHash )
{
    string strSaveDir = strHash.substr(0, 2) + "/" + strHash.substr( strHash.length() - 2, 2 ) + "/";
    return strSaveDir;
}

string get_time_string()
{
    time_t timep;   
    struct tm *p; 
    char szTime[40] = {0};

    time(&timep);   
    p = localtime(&timep); 
    sprintf( szTime, "%d-%d-%d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday );
    return string(szTime);
}

string create_torrent_dir( string save_dir )
{
    char dir_path[260] = {0};
    sprintf( dir_path, "%s/%s", TMP_DIR, save_dir.c_str() );

    CreateDir( dir_path );
    return string(dir_path);
}

bool test_mysql_conn()
{
    bool bRet = false;

    //连接MYSQL服务器
    unsigned int conntime = 5;
    MYSQL* sqlconn = mysql_init( 0 );
    if ( !sqlconn )
    {
        TextLog( "mysql_init Fail" );
        mysql_close( sqlconn );
        return bRet;
    }

    mysql_options( sqlconn, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&conntime );

    sqlconn = mysql_real_connect( sqlconn, DB_HOST, DB_USER, DB_PASSWORD, DB_DATABASE, DB_PORT, 0, 0 );

    if ( !sqlconn )
    {
        TextLog( "MySQL Conn Fail..." );
        mysql_close( sqlconn );
        return bRet;
    }

    mysql_close( sqlconn );
    return true;
}

bool save_to_database( const string & info_hash, const string & filepath, const string & strtime, const torrent_info * torinfo )
{
    bool bRet = false;

    //连接MYSQL服务器
    unsigned int conntime = 5;
    MYSQL_RES* ret_ptr = 0;
    MYSQL_ROW row = 0;
    MYSQL* sqlconn = mysql_init( 0 );
    if ( !sqlconn )
    {
        TextLog( "mysql_init Fail" );
        mysql_close( sqlconn );
        return bRet;
    }

    mysql_options( sqlconn, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&conntime );

    sqlconn = mysql_real_connect( sqlconn, DB_HOST, DB_USER, DB_PASSWORD, DB_DATABASE, DB_PORT, 0, 0 );

    if ( !sqlconn )
    {
        TextLog( "MySQL Conn Fail..." );
        mysql_close( sqlconn );
        return bRet;
    }

    if( mysql_query(sqlconn, "set names utf8") )
    {
        TextLog( "Set UTF8 Success..." );
    }

    char szMySQLCmd[1024] = {0};
    sprintf( szMySQLCmd, "INSERT INTO db_torrents (`hash`, `path`, `date`, `name`, `length`, `ctime`) VALUES('%s','%s', '%s', '%s', '%llu', '%llu') on duplicate key update path='%s'", 
        info_hash.c_str(), filepath.c_str(), strtime.c_str(), torinfo->name().c_str(), torinfo->total_size(), torinfo->creation_date().get_value_or(0), filepath.c_str() );

    if ( 0 != mysql_query( sqlconn, szMySQLCmd ) )
    {
        TextLog( "Fail: %s", szMySQLCmd );
        mysql_close( sqlconn );
        return bRet;
    }

    TextLog( "Success: %s", szMySQLCmd );

    //取刚刚插入进去的id
    sprintf( szMySQLCmd, "SELECT `id` from db_torrents where `hash` = '%s'", info_hash.c_str() );
    if ( 0 != mysql_query( sqlconn, szMySQLCmd ) )
    {
        TextLog( "Fail: %s", szMySQLCmd );
        mysql_close( sqlconn );
        return bRet;
    }

    ret_ptr = mysql_use_result( sqlconn );
    if ( !ret_ptr )
    {
        TextLog( "Fail mysql_use_result: ", szMySQLCmd );
        mysql_close( sqlconn );
        return bRet;
    }

    row = mysql_fetch_row( ret_ptr );
    if ( !row )
    {
        mysql_free_result(ret_ptr);
        TextLog( "Fail mysql_free_result: ", szMySQLCmd );
        mysql_close( sqlconn );
        return bRet;
    }

    long long torrent_id = 0;
    sscanf( row[0], "%llu", &torrent_id );
    mysql_free_result(ret_ptr);

    //获取这个种子内的全部文件
    string strSQLCmd("INSERT INTO `db_files` (`tid`, `fname`, `fpath`, `fsize`) VALUES ");
    for ( int nIndex = 0; nIndex < torinfo->num_files(); nIndex++ )
    {
        //拼接要插入的字符串        
        char szTmp[1024] = {0};
        sprintf( szTmp, "('%llu','%s', '%s', '%llu')", 
            torrent_id, 
            torinfo->files().file_name(nIndex).c_str(), 
            torinfo->files().file_path(nIndex).c_str(), 
            torinfo->file_at(nIndex).size );

        strSQLCmd += szTmp;

        if ( nIndex == torinfo->num_files()-1 )
        {
            strSQLCmd += ";";
        }
        else
        {
            strSQLCmd += ",";
        }
    }

    if ( 0 != mysql_query( sqlconn, strSQLCmd.c_str() ) )
    {
        TextLog( "Fail: %s", strSQLCmd.c_str() );
        mysql_close( sqlconn );
        return bRet;
    }

    mysql_close( sqlconn );
    return true;
}

bool is_database_recorded( string info_hash )
{
    bool bRet = false;
    MYSQL_ROW row = 0;
    MYSQL_RES* ret_ptr = 0;

    //连接MYSQL服务器
    unsigned int conntime = 5;
    MYSQL* sqlconn = mysql_init( 0 );
    if ( !sqlconn )
    {
        TextLog( "mysql_init Fail" );
        mysql_close( sqlconn );
        return bRet;
    }

    mysql_options( sqlconn, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&conntime );

    sqlconn = mysql_real_connect( sqlconn, DB_HOST, DB_USER, DB_PASSWORD, DB_DATABASE, DB_PORT, 0, 0 );

    if ( !sqlconn )
    {
        TextLog( "MySQL Conn Fail...\n" );
        mysql_close( sqlconn );
        return bRet;
    }

    char szMySQLCmd[1024] = {0};
    sprintf( szMySQLCmd, "SELECT hash FROM db_torrents where `hash` = '%s'", 
        info_hash.c_str() );

    if ( mysql_query( sqlconn, szMySQLCmd ) )
    {
        TextLog( "Fail: %s\n", szMySQLCmd );
        mysql_close( sqlconn );
        return bRet;
    }
    else
    {
        ret_ptr = mysql_use_result( sqlconn );
        if ( ret_ptr )
        {
            row = mysql_fetch_row( ret_ptr );
            if ( row )
            {
                TextLog( "InDB: %s", info_hash.c_str() );
                bRet = true;
            }
            else
            {
                TextLog( "NoDB: %s", info_hash.c_str() );
            }
        }
        mysql_free_result(ret_ptr);
    }
    
    mysql_close( sqlconn );
    return bRet;
}

string hash_2_str( sha1_hash info_hash )
{
    char szHash[60] = {0};

    int n = 0;
    for ( ; n < 20; n++ )
    {
        sprintf(szHash+n*2, "%02hhx", info_hash[n]);
    }

    szHash[n*2] = 0;

    return string(szHash);
}

void process_torrents()
{
    //看看有哪些任务的状态变成了3，如果变成了3，则暂停这些任务，并保存其种子
    //批量获取种子信息
    std::map<DWORD, torrent_handle>::iterator iter = g_Settings.handle_map.begin();
    while ( iter != g_Settings.handle_map.end() )
    {
        if ( !(*iter).second.is_valid() )
        {
            iter++;
            continue;
        }

        if ( 
                (*iter).second.status().state == torrent_status::downloading ||
                (*iter).second.status().state == torrent_status::queued_for_checking ||
                (*iter).second.status().state == torrent_status::checking_files ||
                (*iter).second.status().state == torrent_status::finished ||
                (*iter).second.status().state == torrent_status::seeding ||    
                (*iter).second.status().state == torrent_status::allocating ||
                (*iter).second.status().state == torrent_status::checking_resume_data
            )
        {
            g_nFileProcessed++;

            (*iter).second.pause();

            //TODO:在此处保存种子文件
            string szHash = hash_2_str( (*iter).second.info_hash() );

            std::string strFilePath = szHash + ".torrent";

            string time_str = get_time_string();
            string save_dir = get_save_dir( szHash );

            string fullpath = create_torrent_dir( save_dir ) + strFilePath;

            save_torrent( (*iter).second, fullpath.c_str() );

            TextLog( "Meta Down: %s", szHash.c_str() );

            if ( !save_to_database( szHash.c_str(), save_dir + strFilePath, time_str, (*iter).second.torrent_file().get() ) )
            {
                //保存到数据库失败了
                TextLog( "Fail Save to DB: %s", strFilePath.c_str() );
            }

            //从全局数据里删除当前任务
            std::list<HASH_HANDLE>::iterator iter_hash = g_Settings.list_hash.begin();
            while ( iter_hash != g_Settings.list_hash.end() )
            {
                if ( iter_hash->info_hash == (*iter).second.info_hash() )
                {
                    g_Settings.list_hash.erase(iter_hash);
                    break;
                }
                iter_hash++;
            }

            g_Settings.ses->remove_torrent( g_Settings.ses->find_torrent( (*iter).second.info_hash() ) ); 
            g_Settings.handle_map.erase( iter++ );
            //移除结束

            continue;
        }
        iter++;
    }    
}

void my_handle_alert(libtorrent::alert* a)
{
    if ( metadata_received_alert* s = alert_cast<metadata_received_alert>(a) )
    {
        process_torrents();
    }
    else if ( state_changed_alert* s = alert_cast<state_changed_alert>(a) )
    {
        process_torrents();
    }
    else if ( torrent_added_alert* s =  alert_cast<torrent_added_alert>(a) )
    {
        process_torrents();
    }
    else if ( dht_announce_alert* s = alert_cast<dht_announce_alert>(a) )
    {
        //拿到HASH了，先查数据库有没有这个hash，没有的话，就添加成BT任务
        string strHash = hash_2_str( s->info_hash );

        TextLog( "Hash Recved: %s", strHash.c_str() );
        
        //看看现有任务里面有没有，有的话就直接放弃
        if( FindSameTask( s->info_hash ) )
        {
            TextLog("InTask: %s", strHash.c_str());
            return;
        }

        if ( is_database_recorded( strHash ) )
        {
            //数据库里已有，不再添加任务了
            return;
        }

        std::string strMagNet = std::string("magnet:?xt=urn:btih:") + strHash;

        //如果任务超过MAX_JOBS个，则删除头部的那一个
        if ( g_Settings.ses->get_torrents().size() >= MAX_JOBS )
        {
            HASH_HANDLE head_hash_handle = *g_Settings.list_hash.begin();            
        
            std::map<DWORD, libtorrent::torrent_handle>::iterator iter_handle_map = g_Settings.handle_map.find( head_hash_handle.hTorrent );

            TextLog("Del_Task: Handle: %u -- Hash: %s", head_hash_handle.hTorrent, hash_2_str(head_hash_handle.info_hash).c_str() );

            iter_handle_map->second.pause();
            g_Settings.ses->remove_torrent( iter_handle_map->second );
            g_Settings.handle_map.erase( g_Settings.handle_map.find( head_hash_handle.hTorrent ) );
            g_Settings.list_hash.pop_front();
        }

        HANDLE hTask = BT_Add_From_MagNet( strMagNet.c_str(), DOWNLOAD_DIR );
        BT_LimitTaskSpeed( hTask, 4096, 4096 );
        if(  hTask == INVALID_HANDLE_VALUE )
        {
            TextLog( "Fail to add magnet: %s", strMagNet.c_str() );
            return;
        }
        
        g_bNewTaskAdd = true;

        TextLog("AddTask: %s", strHash.c_str());
    }
    else if( dht_get_peers_alert* s = alert_cast<dht_get_peers_alert>(a) )
    {
        //拿到HASH了，先查数据库有没有这个hash，没有的话，就添加成BT任务
        string strHash = hash_2_str( s->info_hash );

        TextLog( "Hash Recved: %s", strHash.c_str() );

        //看看现有任务里面有没有，有的话就直接放弃
        if( FindSameTask( s->info_hash ) )
        {
            TextLog("InTask: %s", strHash.c_str());
            return;
        }

        if ( is_database_recorded( strHash ) )
        {
            //数据库里已有，不再添加任务了
            return;
        }

        std::string strMagNet = std::string("magnet:?xt=urn:btih:") + strHash;

        //如果任务超过MAX_JOBS个，则删除头部的那一个
        if ( g_Settings.ses->get_torrents().size() >= MAX_JOBS )
        {
            HASH_HANDLE head_hash_handle = *g_Settings.list_hash.begin();            

            std::map<DWORD, libtorrent::torrent_handle>::iterator iter_handle_map = g_Settings.handle_map.find( head_hash_handle.hTorrent );

            TextLog("Del_Task: Handle: %u -- Hash: %s", head_hash_handle.hTorrent, hash_2_str(head_hash_handle.info_hash).c_str() );

            iter_handle_map->second.pause();
            g_Settings.ses->remove_torrent( iter_handle_map->second );
            g_Settings.handle_map.erase( g_Settings.handle_map.find( head_hash_handle.hTorrent ) );
            g_Settings.list_hash.pop_front();
        }

        HANDLE hTask = BT_Add_From_MagNet( strMagNet.c_str(), DOWNLOAD_DIR );
        BT_LimitTaskSpeed( hTask, 4096, 4096 );
        if(  hTask == INVALID_HANDLE_VALUE )
        {
            TextLog( "Fail to add magnet: %s", strMagNet.c_str() );
            return;
        }

        g_bNewTaskAdd = true;

        TextLog("AddTask: %s", strHash.c_str());  
    }

    return;
}

void* WorkThread(void* arg)
{
    #define EVENT_SIZE  ( sizeof (struct inotify_event) )  
    #define EVENT_BUF_LEN     ( 10 * ( EVENT_SIZE + 16 ) ) 

    int fd = inotify_init();
    char buffer[EVENT_BUF_LEN];

    if (fd < 0) 
    {  
        perror("inotify_init");
        return 0;
    }

    int wd = inotify_add_watch( fd, UPLOAD_DIR, IN_CLOSE_WRITE );

    while(1)
    {
        int length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {  
            perror("read");  
        }

        int i = 0;
        while (i < length) 
        {  
            struct inotify_event *event = (struct inotify_event *) &buffer[i];  

            if (event->len) 
            {  
                if (event->mask & IN_CLOSE_WRITE) 
                {  
                    if (event->mask & IN_ISDIR) 
                    {  
                        TextLog("New directory %s created.\n", event->name);
                    } 
                    else 
                    {                          
                        string strTorPath(UPLOAD_DIR);
                        strTorPath += event->name;
                        TextLog("New file %s created.\n", strTorPath.c_str());

                        HANDLE hTask = BT_Add_From_File( strTorPath.c_str(), DOWNLOAD_DIR );
                        BT_LimitTaskSpeed( hTask, 4096, 4096 );
                        unlink( strTorPath.c_str() );
                    }
                } 
            }  
            i += EVENT_SIZE + event->len;  
        } 
    }

    inotify_rm_watch(fd, wd);
    close(fd); 
    return 0;
}

int main(int argc, char* argv[])
{
    pid_t pid = -1;
    int status = 0;

    while( 1 )
    {
        pid = fork();

        if ( pid < 0 )
        {
            TextLog( "fork err,exit..." );
            return 0;
        }
        else if ( pid > 0 )
        {
            //如果我就是父进程
            waitpid( pid, &status, 0 );
        }
        else if ( pid == 0 )
        {
            //如果我就是子进程
            break;
        }
    }

    mysql_thread_init();
    string  str_time;
    TextLog( "Date: %s -- LibTorrent: %u.%u.%u\n", get_time_string().c_str(), LIBTORRENT_VERSION_MAJOR, LIBTORRENT_VERSION_MINOR, LIBTORRENT_VERSION_TINY );
    if ( !test_mysql_conn() )
    {
        TextLog("Fatal error: Can't conn to MySQL server...");
        return 0;
    }

    INIT_THREAD_LOCKER;
    BT_Init();

    //开启一个线程，去监听种子上传目录
    pthread_attr_t tattr;
    int stacksize = PTHREAD_STACK_MIN;
    int ret;
    ret = pthread_attr_init( &tattr );
    pthread_attr_setstacksize( &tattr, stacksize /*128 * 1024*/ );

    pthread_t threadid = 0;
    pthread_create( &threadid, &tattr, WorkThread, NULL );
    //开启线程结束

    HANDLE x = BT_Add_From_MagNet( "magnet:?xt=urn:btih:5a3cddafece8cd324b1e2b6d9f118a09e66f349a", DOWNLOAD_DIR );
    BT_LimitTaskSpeed( x, 4096, 4096 );

    int nCount = 0;

    std::deque<alert*> vec_pa;
    std::deque<alert*>::iterator iter_pa;

    while ( 1 )
    { 
        alert const* a = g_Settings.ses->wait_for_alert( seconds(1) );
        if ( !a )
        {
            continue;
        }

        session_status sess_stat = g_Settings.ses->status();

        if ( g_bNewTaskAdd == true )
        {
            g_bNewTaskAdd = false;

            TextLog("TorCount:%u -- Done:%u -- HandleCoun: %u -- HashCount: %u", g_Settings.ses->get_torrents().size(), g_nFileProcessed,
                g_Settings.handle_map.size(), g_Settings.list_hash.size() );
            TextLog("DHT nodes: %u DHT cached nodes: %u total DHT size: %llu total observers: %u\n"
                , sess_stat.dht_nodes, sess_stat.dht_node_cache, sess_stat.dht_global_nodes
                , sess_stat.dht_total_allocations);
        }

        std::auto_ptr<alert> alert = g_Settings.ses->pop_alert();
        my_handle_alert( &*alert );

        if ( g_Settings.handle_map.size() > 1 )
        {
            g_Settings.handle_map[(DWORD)x].pause();
        }
        else
        {
            if ( g_Settings.handle_map[(DWORD)x].status().paused )
            {
                g_Settings.handle_map[(DWORD)x].resume();
            }
        }
    }

    mysql_thread_end();

    BT_Release();
    return 0;
}
