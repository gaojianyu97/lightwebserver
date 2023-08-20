#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "../lock/locker.h"
#include "../MySqlPool/sql_conn_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

#include <map>
#include <fstream>

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/stat.h>
/**
 * @brief http_conn类，用于处理http连接
*/
class http_conn{
public:
    http_conn(){}
    ~http_conn(){}

public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    /**
     * @brief 请求方法
    */
    enum METHOD {
        GET = 0,          // GET 请求
        POST,             // POST 请求
        HEAD,             // HEAD 请求
        PUT,              // PUT 请求
        DELETE,           // DELETE 请求
        TRACE,            // TRACE 请求
        OPTIONS,          // OPTIONS 请求
        CONNECT,          // CONNECT 请求
        PATH              // PATH 请求
    };

    /**
     * @brief 检查请求状态
    */
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,  // 检查请求行状态
        CHECK_STATE_HEADER,           // 检查请求头状态
        CHECK_STATE_CONTENT           // 检查请求正文状态
    };

    /**
     * @brief http连接状态
    */
    enum HTTP_CODE {
        NO_REQUEST,        // 无请求
        GET_REQUEST,       // GET 请求
        BAD_REQUEST,       // 错误请求
        NO_RESOURCE,       // 请求的资源不存在
        FORBIDDEN_REQUEST, // 禁止请求
        FILE_REQUEST,      // 文件请求
        INTERNAL_ERROR,    // 内部错误
        CLOSED_CONNECTION  // 连接已关闭
    };

    /**
     * @brief 行解析状态
    */
    enum LINE_STATE
    {
        LINE_OK = 0,    //行解析正常
        LINE_BAD,       //行解析错误
        LINE_OPEN       //行解析未完成
    };

private:
    int m_sockfd;                           //连接套接字
    sockaddr_in m_address;                  //客户端地址
    char m_read_buf[READ_BUFFER_SIZE];      //读缓冲区
    long m_read_idx;                        //当前读取位置
    long m_checked_idx;                     //当前解析位置
    int m_start_line;                       //当前行的起始位置
    char m_write_buf[WRITE_BUFFER_SIZE];    //写缓冲区
    int m_write_idx;                        //当前写入位置
    CHECK_STATE m_check_state;              //解析状态
    METHOD m_method;                        //请求方法
    char m_real_file[FILENAME_LEN];         //文件路径
    char *m_url;                            //请求URL
    char *m_version;                        //http版本
    char *m_host;                           //请求的主机名
    long m_content_length;                  //请求内容长度
    bool m_linger;                          //是否保持连接
    char *m_file_address;                   //文件内存映射地址
    struct stat m_file_stat;                //文件状态信息
    struct iovec m_iv[2];                   //写缓冲区指针数组
    int m_iv_count;                         //写缓冲区指针数量
    int cgi;                                //是否启用CGI
    char *m_string;                         //存储请求头数据
    int bytes_to_send;                      //剩余发送字节数
    int bytes_have_send;                    //已经发送字节数
    char *doc_root;                         // 网站根目录

    map<string, string> m_users;            //用户名和密码
    int m_trig_mode;                        //触发模式
    int m_close_log;                        //是否关闭日志

    char sql_user[100];                     //数据库用户名        
    char sql_passward[100];                 //数据库密码
    char sql_name[100];                     //数据库名

public:
    static int m_epollfd;                   //epoll文件描述符
    static int m_user_count;                //用户数量
    MYSQL *mysql;                           //MySql连接
    int m_state;                            //连接状态

public:
    /**
     * @brief 初始化 http_conn 对象
     * @param sockfd 套接字描述符，表示与客户端的连接
     * @param addr 客户端的地址结构，用于标识客户端的IP地址和端口号
     * @param root 网站根目录的路径
     * @param trig_mode 触发模式，指定使用的epoll触发模式(ET/LT)
     * @param close_log 是否关闭日志，用于控制是否关闭日志输出
     * @param user 数据库用户名
     * @param passward 数据库密码
     * @param sqlname 数据库名
     */
    void init(int sockfd, const sockaddr_in &addr, char *root, int trig_mode, int close_log, string user, string passward, string sqlname);

    /**
     * @brief 关闭连接
     * @param real_close 是否真正关闭连接，默认为 true，表示真正关闭，false 表示不真正关闭
     */    
    void close_conn(bool real_close = true);

    /**
     * @brief 处理请求
    */
    void process();

    /**
     * @brief 从客户端读取数据，仅一次性读取一定数据量的数据
     * @return 读取成功返回 true，读取失败返回 false
     */
    bool read_once();

    /**
     * @brief 向客户端写入响应数据
     *
     * @return 写入成功返回 true，写入失败返回 false
     */
    bool write();

    /**
     * @brief 获取客户端地址结构的指针
     * @return 指向客户端地址结构的指针
     */
    sockaddr_in *get_address(){
        return &m_address;
    }

    /**
     * @brief 初始化MySQL连接池
     *
     * 通过传入的连接池对象初始化当前HTTP连接的MySQL连接池
     *
     * @param conn_pool 连接池对象的指针
     */
    void initmysql_result(connection_pool *conn_pool);
    int timer_flag;
    int improv;

private:
    /**
     * @brief 初始化连接
    */
    void init();

    /**
     * @brief 处理读取HTTP请求的函数
     * @return HTTP请求处理结果，可能的取值有：
     *         - NO_REQUEST: 请求不完整，需要继续读取
     *         - GET_REQUEST: 获取到一个完整的HTTP请求
     *         - BAD_REQUEST: 请求语法错误
     *         - NO_RESOURCE: 请求的资源不存在
     *         - FORBIDDEN_REQUEST: 请求的资源权限不足
     *         - FILE_REQUEST: 请求的是文件资源
     *         - INTERNAL_ERROR: 服务器内部错误
     *         - CLOSED_CONNECTION: 客户端已关闭连接
     */
    HTTP_CODE process_read();

    /**
     * @brief 处理写入HTTP响应的函数
     * @param ret HTTP请求处理结果，可以是 process_read() 返回的结果
     * @return 写入响应的结果，返回 true 表示写入成功，返回 false 表示写入失败
     */
    bool process_write(HTTP_CODE ret);

    /**
     * @brief 解析HTTP请求行
     * @param text 待解析的HTTP请求行数据
     * @return 解析结果，包括请求行解析状态，如成功解析请求行则返回相应的HTTP请求方法
     */
    HTTP_CODE parse_request_line(char *text);

    /**
     * @brief 解析HTTP请求头部
     * @param text 待解析的HTTP请求头部数据
     * @return 解析结果，包括头部解析状态，如成功解析头部则返回相应的HTTP状态码
     */
    HTTP_CODE parse_headers(char *text);

    /**
     * @brief 解析HTTP请求消息体
     * @param text 待解析的HTTP请求消息体数据
     * @return 解析结果，包括消息体解析状态，如成功解析消息体则返回相应的HTTP状态码
     */
    HTTP_CODE parse_content(char *text);

    /**
     * @brief 处理HTTP请求的具体业务逻辑，生成响应报文
     * @return 生成响应报文的状态，如成功处理则返回相应的HTTP状态码
     */
    HTTP_CODE do_request();

    /**
     * @brief 获取请求报文中当前行的起始位置
     * @return 指向当前行起始位置的指针
     */
    char *get_line(){
        return m_read_buf + m_start_line;
    }

    /**
     * @brief 解析一行HTTP报文内容
     * @return 当前行的解析状态
     */
    LINE_STATE parse_line();

    /**
     * @brief 释放映射的文件内存
     */
    void unmap();

    /**
     * @brief 将格式化的字符串添加到响应缓冲区中
     * @param format 格式化字符串
     * @param ... 格式化参数
     * @return 是否添加成功
     */    
    bool add_response(const char *format, ...);

    /**
     * @brief 将指定内容添加到响应缓冲区中
     * @param content 要添加的内容
     * @param ... 格式化参数
     * @return 是否添加成功
     */    
    bool add_content(const char *content);

    /**
     * @brief 添加状态行到响应缓冲区中
     * @param state HTTP状态码
     * @param title 状态描述
     * @return 是否添加成功
     */
    bool add_state_line(int state, const char *title);

    /**
     * @brief 添加响应头部信息到响应缓冲区中
     * @param content_length 响应内容长度
     * @return 是否添加成功
     */
    bool add_headers(int content_length);

    /**
     * @brief 添加响应的内容类型到响应缓冲区中
     * @return 是否添加成功
     */
    bool add_content_type();

    /**
     * @brief 添加响应的内容长度到响应缓冲区中
     * @param content_length 响应内容长度
     * @return 是否添加成功
     */
    bool add_content_length(int content_length);

    /**
     * @brief 添加连接状态到响应缓冲区中
     * @return 是否添加成功
     */
    bool add_linger();

    /**
     * @brief 添加连接状态到响应缓冲区中
     * @return 是否添加成功
     */
    bool add_blank_line();
};

#endif