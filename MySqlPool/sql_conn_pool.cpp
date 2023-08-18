#include "sql_conn_pool.h"

using namespace std;

connection_pool::connection_pool(){
    m_cur_conn = 0;
    m_free_conn = 0;
}


connection_pool::~connection_pool(){
    destory_pool();
}

connection_pool *connection_pool::get_instance(){
    static connection_pool conn_pool;
    return &conn_pool;
}

void connection_pool::init(string url,string user,string passsward,string database_name,int port,int max_conn,int close_log){
    //设置连接池属性
    m_url = url;
    m_port = port;
    m_user = user;
    m_passward = passsward;
    m_database_name = database_name;
    m_close_log = close_log;

    //循环创建指定数量的数据库连接
    for (int i = 0; i < max_conn;i++){
        MYSQL *conn = NULL;
        conn = mysql_init(conn);

        if(conn==NULL){
            LOG_ERROR("MySql Errror");
            exit(1);
        }

        //连接到数据库
        conn = mysql_real_connect(conn, url.c_str(), user.c_str(), passsward.c_str(), database_name.c_str(), port, NULL, 0);

        if(conn==NULL){
            LOG_ERROR("MySql Error");
            exit(1);
        }

        //将连接添加到池中
        conn_list.push_back(conn);
        m_free_conn++;
    }

    //初始化信号量，初始值为空闲连接的数量
    reserve = sem(m_free_conn);

    //设置最大连接数
    m_max_conn = m_free_conn;
}

MYSQL *connection_pool::get_connecton(){
    MYSQL *conn = NULL;
    if(0==conn_list.size())
        return NULL;

    //等待信号量，表示获得一个连接
    reserve.wait();

    lock.lock();//上锁

    //从连接池前端取出一个连接
    conn = conn_list.front();
    conn_list.pop_front();


    //更新当前连接数和空闲连接数
    m_free_conn--;
    m_cur_conn++;

    lock.unlock();//解锁
    return conn;//返回获得的连接
}

bool connection_pool::release_connection(MYSQL *conn){
    if(conn==NULL)
        return false;

    lock.lock();

    //将连接放回连接池底部
    conn_list.push_back(conn);
    m_free_conn++;
    m_cur_conn--;

    lock.unlock();

    //释放信号量，表示连接已释放会连接池
    reserve.post();

    return true;
}

void connection_pool::destory_pool(){
    lock.lock();
    if(conn_list.size()>0){
        list<MYSQL *>::iterator it;

        //循环关闭连接
        for (it = conn_list.begin(); it != conn_list.end();it++){
            MYSQL *conn = *it;
            mysql_close(conn);
        }
        m_cur_conn = 0;
        m_free_conn = 0;
        conn_list.clear();//连接池清零，重置现存连接数和空闲连接数为0
    }

    lock.unlock();//解锁
}

int connection_pool::get_free_conn(){
    return this->m_free_conn;
}

connection_raii::connection_raii(MYSQL **conn,connection_pool *conn_pool){
    *conn = conn_pool->get_connecton();

    conn_raii = *conn;
    pool_raii = conn_pool;
}

connection_raii::~connection_raii(){
    pool_raii->release_connection(conn_raii);
}