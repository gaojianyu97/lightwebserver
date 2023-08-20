#include "log.h"
#include <time.h>
#include <string.h>
#include <sys/time.h>

using namespace std;

Log::Log(){
    m_count = 0;
    m_is_asnc = false;
}

Log::~Log(){
    if(m_fp!=NULL){
        fclose(m_fp);
    }
}

bool Log::init(const char *file_name,int close_log,int log_buf_size,int split_lines,int max_queue_size){
    if(max_queue_size>=1){
        m_is_asnc = true;
        m_log_queue = new block_queue<string>(max_queue_size);
        pthread_t tid;
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }

    m_close_log = close_log;
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);
    m_split_lines = split_lines;

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);//时间转化
    struct tm m_tm = *sys_tm;

    const char *p = strrchr(file_name, '/');
    char log_full_name[512] = {0};

    if(p==NULL){
        // 如果路径中没有斜杠，直接生成日志文件名，不包含路径
        snprintf(log_full_name, 511, "%d_%02d_%02d_%s", m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday, file_name);
    }else{
        // 如果路径中有斜杠，分离出路径和文件名部分
        // 复制文件名部分到 log_name
        strcpy(log_name, p + 1);
        // 将路径部分拷贝到 dir_name，长度为 p - file_name + 1
        strncpy(dir_name, file_name, p - file_name + 1);
        // 生成完整的日志文件名，包含路径和文件名
        snprintf(log_full_name, 511, "%s%d_%02d_%02d_%s", dir_name,m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday, log_name);
    }

    m_today = m_tm.tm_mday;
    m_fp = fopen(log_full_name, "a");
    if(m_fp==NULL){
        return false;
    }

    return true;
}

void Log::write_log(int level,const char *format,...){
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm m_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;    
    case 3:
        strcpy(s, "[error]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }

    m_mutex.lock();//上锁
    m_count++;
    //判断是否需要创建新的log文件
    //如果日期发生变化，则需要创建新的log文件，文件名为:(路径名/)年_月_日_文件名；
    //如果log文件达到最大函数，则需要创建log文件，文件名为:(路径名/)年_月_日_文件名.数字
    if(m_today!=m_tm.tm_mday||m_count%m_split_lines==0){
        char new_log[512] = {0};
        fflush(m_fp); // 刷新流的缓冲区
        fclose(m_fp);
        char tail[16] = {0};

        snprintf(tail, 16, "%d_%02d_%02d_", m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday);

        if(m_today!=m_tm.tm_mday){
            snprintf(new_log, 511, "%s%s%s", dir_name, tail, log_name);
            m_today = m_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            snprintf(new_log, 511, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }

        m_fp = fopen(new_log, "a");
    }

    m_mutex.unlock();//解锁
    va_list valist;//存储可变参数列表的信息，以便在函数内部访问日志输出的格式化字符串和参数
    va_start(valist, format);

    string log_str;
    m_mutex.lock();//上锁

    //写入的具体时间内容格式

    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s",
                     m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday,
                     m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec, now.tv_usec, s);

    int m = vsnprintf(m_buf + n, m_log_buf_size - n - 1, format, valist);

    m_buf[m + n] = '\n';
    m_buf[m + n + 1] = '\0';
    log_str = m_buf;

    m_mutex.unlock();//解锁

    if(m_is_asnc&&!m_log_queue->full()){
        m_log_queue->push(log_str);
    }else{
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valist);
}

void Log::flush(void){
    m_mutex.lock();//上锁
    fflush(m_fp);
    m_mutex.unlock();//解锁
}