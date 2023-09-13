# lightwebserver

This project is  to provide the  learning of webserver for those who know none of it.  
Thanks very much for tinywebserver(https://github.com/qinguoyi/TinyWebServer.git)

## 目录

[1、配置基础环境](./doc/1.配置基础环境.md)  

[2、压力测试](./doc/2.测试结果.md)

[3、线程池](./src/threadpool/README.md)

[4、锁](./src/lock/README.md)

[5、数据库连接](./src/MySqlPool/README.md)

[6、定时器](./src/timer/README.md)

[7、http连接](./src/httpconn/README.md)

[8、日志](./src/log/README.md)

[9、服务器](./src/webserver/README.md)

## 编译及运行

- 测试前确认已安装MySQL数据库

    ```C++
    // 建立yourdb库
    create database yourdb;

    // 创建user表
    USE yourdb;
    CREATE TABLE user(
        username char(50) NULL,
        passwd char(50) NULL
    )ENGINE=InnoDB;

    // 添加数据
    INSERT INTO user(username, passwd) VALUES('name', 'passwd');
    ```

- 修改main.cpp中的数据库初始化信息

    ```bash
    //数据库登录名,密码,库名
    string user = "root";
    string passwd = "root";
    string databasename = "yourdb";
    ```

- build

    ```bash
    make server
    ```

- 启动server

    ```bash
    ./server
    ```

- 浏览器端

    ```bash
    ip:9006
    ```

- 清除编译

    ```bash
    make clean
    ```

## 运行

个性化运行

------

```bash
./server [-p port] [-l LOGWrite] [-m TRIGMode] [-o OPT_LINGER] [-s sql_num] [-t thread_num] [-c close_log] [-a actor_model]
```

温馨提示:以上参数不是非必须，不用全部使用，根据个人情况搭配选用即可.

- -p，自定义端口号
  - 默认9006

- -l，选择日志写入方式，默认同步写入
  - 0，同步写入
  - 1，异步写入

- -m，listenfd和connfd的模式组合，默认使用LT + LT
  - 0，表示使用LT + LT
  - 1，表示使用LT + ET
  - 2，表示使用ET + LT
  - 3，表示使用ET + ET

- -o，优雅关闭连接，默认不使用
  - 0，不使用
  - 1，使用

- -s，数据库连接数量
  - 默认为8

- -t，线程数量
  - 默认为8

- -c，关闭日志，默认打开
  - 0，打开日志
  - 1，关闭日志

- -a，选择反应堆模型，默认Proactor
  - 0，Proactor模型
  - 1，Reactor模型
