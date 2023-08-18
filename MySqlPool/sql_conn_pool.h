#ifndef SQL_CONN_POOL_H
#define SQL_CONN_POOL_H

#include "./lock/locker.h"
#include "./log/log.h"

#include <list>
#include <error.h>
#include <string.h>
#include <mysql/mysql.h>

using namespace std;

/**
 * @brief connection_pool类，数据库连接池
*/
class connection_pool{
private:
    /**
     * @brief connection_pool类构造函数
    */
    connection_pool();

    /**
     * @brief connection_pool类析构函数
    */
    ~connection_pool();

    int m_max_conn;         //最大连接数
    int m_cur_conn;         //当前已使用的连接数
    int m_free_conn;        //当前空闲的连接数
    locker lock;            //锁
    list<MYSQL *> conn_list;//连接池
    sem reserve;            //信号量

public:
    string m_url;           //主机地址
    string m_port;          //数据库端口号
    string m_user;          //登录数据库用户名
    string m_passward;      //登录数据库密码
    string m_database_name; //登录数据库名称
    int m_close_log;        //日志开关

public:
    /**
     * @brief 获取数据库连接
    */
    MYSQL *get_connecton();

    /**
     * @brief 释放连接
    */
    bool release_connection(MYSQL *conn);

    /**
     * @brief 获取空闲连接数
    */
    int get_free_conn();

    /**
     * @brief 销毁连接池
    */
    void destory_pool();

    /**
     * @brief 单例模式
    */
    static connection_pool * get_instance();

    /**
     * @brief 初始化连接池，设置数据库连接参数并创建连接
     * @param url 数据库主机地址
     * @param user 登录数据库的用户名
     * @param passWord 登录数据库的密码
     * @param database_name 使用的数据库名
     * @param port 数据库端口号
     * @param max_conn 最大连接数
     * @param close_log 日志开关，用于控制是否记录日志
     */
    void init(string url, string user, string passward, string database_name, int port, int max_conn, int close_log);


};


/**
 * @brief connection_raii类，数据库连接资源管理类，用于在作用域结束时自动释放连接，避免资源泄露
*/
class connection_raii{
private:
    MYSQL *conn_raii;           //数据库连接的地址
    connection_pool *pool_raii; //连接池对象的地址

public:
    /**
     * @brief 构造函数，初始化连接和连接池
     * @param conn 数据库链接地址
     * @param conn_pool 连接池对象 
    */
    connection_raii(MYSQL **conn, connection_pool *conn_pool);

    /**
     * @brief 析构函数，在作用域结束时释放连接
    */
    ~connection_raii();
};

#endif SQL_CONN_POOL_H