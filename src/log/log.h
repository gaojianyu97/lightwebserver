#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

/**
 * @class Log类
 * @brief log日志类，用于程序中记录日志
*/
class Log{
private:
    char dir_name[128];//路径名
    char log_name[128];//log文件名
    int m_split_lines; //日志最大行数
    int m_log_buf_size;//日志缓冲区大小
    long long m_count; //日志行数记录
    int m_today;       //按天记录，记录当前哪一天
    FILE *m_fp;        //打开log的文件指针
    char *m_buf;       //日志缓冲区
    block_queue<string> *m_log_queue;//阻塞队列
    bool m_is_asnc;                  //是否同步标志位
    locker m_mutex;                  //互斥锁
    int m_close_log;                 //关闭日志文件

private:
    /**
     * @brief Log类的构造函数
    */
    Log();

    /**
     * @brief Log类的析构函数
    */
    virtual ~Log();

    /**
     * @brief 异步写入日志的线程函数，从阻塞队列中取出日志并写入文件。
    */
    void *async_write_log(){
        string single_log;
        while(m_log_queue->pop((single_log))){
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

public:
    /**
     * @brief 用于获取日志类的单例实例(懒汉模式)
    */
    static Log *get_instance(){
        static Log instance;
        return &instance;
    }

    /**
     * @brief 异步地刷新日志内容
    */
    static void *flush_log_thread(void *args){
        Log::get_instance()->async_write_log();
    }

    /**
     * @brief 初始化日志记录器，设置日志文件名、关闭日志标志、日志缓冲区大小、分割行数以及最大队列大小
     * @param filename 日志文件名
     * @param close_log 关闭日志标记(文件标识符)
     * @param log_buf_size 日志缓冲区大小，默认大小8192
     * @param split_lines 日志分割行数，默认大小5,000,000
     * @param max_queue_size 最大队列大小，默认大小0,异步需要设置阻塞队列的长度，同步不需要设置
     * @return true:初始化成功，false:初始化失败
    */
    bool Log::init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    /**
     * @brief 写入日志
     * @param level 0:LOG_DEBUG,1:LOG_INFO,2:LOG_WARN,3:LOG_ERROR
     * @param format 格式化字符串，用于指定日志的格式
    */
    void Log::write_log(int level, const char *format, ...);

    /**
     * @brief 刷新日志，将缓冲区中的日志内容写入文件
    */
    void Log::flush(void);
};

#define LOG_DEBUG(format, ...)if(0 == m_close_log){Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...)if(0 == m_close_log){Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...)if(0 == m_close_log){Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...)if(0 == m_close_log){Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#endif LOG_H