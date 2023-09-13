# 服务器

`WebServer` 类是一个简单的Web服务器类，用于初始化服务器参数，创建线程池、数据库连接池、事件监听等，并处理客户端请求。

以下是对 `WebServer` 类的构造函数和部分成员函数的详细说明：

## 构造函数 `WebServer::WebServer()`

- 作用：构造函数用于创建 `WebServer` 对象，并初始化一些成员变量。
- 实现：在构造函数中进行了以下操作：
  - 创建了 `http_conn` 类对象数组，用于管理客户端连接。
  - 获取当前工作目录，用于设置服务器的根目录。
  - 分配并初始化了用于存储定时器的 `client_data` 数组。

## 析构函数 `WebServer::~WebServer()`

- 作用：析构函数用于释放 `WebServer` 对象相关的资源。
- 实现：在析构函数中进行了以下操作：
  - 关闭了 epoll 文件描述符和监听套接字。
  - 关闭了管道文件描述符。
  - 释放了 `http_conn` 类对象数组和定时器数组。
  - 删除了线程池对象。

## `void WebServer::init(...)`

- 作用：初始化服务器的配置参数。
- 参数：多个参数用于设置服务器的各项配置，例如端口号、数据库用户名、密码等。
- 实现：该方法接受多个参数，并将这些参数设置为 `WebServer` 对象的成员变量，用于配置服务器的各项参数。

## `void WebServer::trig_mode()`

- 作用：根据配置参数设置服务器的事件触发模式。
- 实现：该方法根据配置参数 `m_TRIGMode` 的值来设置服务器的事件触发模式，包括 LT + LT、LT + ET、ET + LT、ET + ET 四种模式的组合。

## `void WebServer::log_write()`

- 作用：初始化日志记录器。
- 实现：如果配置参数 `m_close_log` 不为 0，该方法初始化了日志记录器，并设置日志的文件名和相关参数。

## `void WebServer::sql_pool()`

- 作用：初始化数据库连接池。
- 实现：该方法创建了一个数据库连接池对象 `m_connPool`，并使用配置参数初始化连接池的相关信息，包括数据库地址、用户名、密码等。

## `void WebServer::thread_pool()`

- 作用：创建并启动线程池。
- 实现：该方法创建了一个线程池对象 `m_pool`，并启动了线程池中的线程，用于处理客户端请求。

## `void WebServer::event_listen()`

- 作用：创建并初始化监听套接字，将其加入到 epoll 事件监听中。
- 实现：该方法完成了以下操作：
  - 创建监听套接字 `m_listenfd`。
  - 设置套接字属性，包括优雅关闭连接。
  - 绑定监听套接字到指定端口并开始监听。
  - 初始化了网络工具类 `utils`，用于设置信号处理函数。
  - 创建 epoll 文件描述符 `m_epollfd`，并将监听套接字加入到 epoll 事件监听中。
  - 创建了管道用于处理信号。
  - 设置了一些信号处理函数和定时器。

## `void WebServer::timer(int connfd, struct sockaddr_in client_address)`

- 作用：该函数负责为新连接初始化一个定时器。
- 实现：
  - 它使用提供的`connfd`、`client_address`和其他配置参数初始化了一个`http_conn`对象。
  - 初始化一个包含客户端信息和定时器的`client_data`对象。
  - 定时器设置为在 3 个时间槽（默认为 15 秒）后到期。
  - 使用`utils.m_timer_lst.add_timer(timer)`将定时器添加到定时器链表中。

## `void WebServer::adjust_timer(util_timer *timer)`

- 作用：该函数用于调整现有定时器的到期时间。
- 实现：
  - 更新定时器的到期时间，设置为从当前时间开始的 3 个时间槽（默认为 15 秒）后。
  - 调用`utils.m_timer_lst.adjust_timer(timer)`来调整定时器在定时器链表中的位置。

## `void WebServer::deal_timer(util_timer *timer, int sockfd)`

- 作用：当定时器到期时，此函数被调用，执行与定时器相关的操作。
- 实现：
  - 调用与定时器关联的回调函数`cb_func`，并传递与套接字`sockfd`关联的`client_data`对象。
  - 使用`utils.m_timer_lst.del_timer(timer)`从定时器链表中删除定时器。
  - 记录关闭套接字的相关信息。

## `bool WebServer::deal_clinetdata()`

- 作用：此函数处理接受的客户端连接并为它们设置定时器。
- 实现：
  - 如果服务器处于 LT 模式（`m_LISTENTrigmode`为 0），它使用`accept`接受单个客户端连接。
  - 如果服务器处于 ET 模式（`m_LISTENTrigmode`为 1），它使用循环接受多个客户端连接。
  - 对于每个接受的连接，它检查服务器是否过载（达到了最大客户端数量），如果没有，则调用`timer`函数为连接设置定时器。

## `bool WebServer::deal_with_signal(bool &timeout, bool &stop_server)`

- 作用：此函数处理服务器接收到的信号。
- 实现：
  - 从管道`m_pipefd[0]`中读取信号并进行处理。
  - 如果接收到的信号是`SIGALRM`，则将`timeout`标志设置为`true`。
  - 如果接收到的信号是`SIGTERM`，则将`stop_server`标志设置为`true`。

## `void WebServer::deal_with_read(int sockfd)`

- 作用：该函数用于处理读事件，根据服务器的工作模式（Reactor 或 Proactor）不同，执行不同的操作。
- 实现：
  - 如果服务器使用 Reactor 模式（`m_actormodel`为 1），则会进行以下操作：
    - 调整与当前连接关联的定时器。
    - 将读事件放入请求队列。
    - 等待`users[sockfd].improv`变为 1，然后检查定时器标志并执行必要的操作。
  - 如果服务器使用 Proactor 模式（`m_actormodel`为 0），则会进行以下操作：
    - 调整与当前连接关联的定时器。
    - 调用`users[sockfd].read_once()`从套接字中读取数据，并在成功读取时将读事件放入请求队列。
    - 如果读取失败，将调用`deal_timer`来处理定时器。

## `void WebServer::deal_with_write(int sockfd)`

- 作用：该函数用于处理写事件，根据服务器的工作模式（Reactor 或 Proactor）不同，执行不同的操作。
- 实现：
  - 如果服务器使用 Reactor 模式（`m_actormodel`为 1），则会进行以下操作：
    - 调整与当前连接关联的定时器。
    - 将写事件放入请求队列。
    - 等待`users[sockfd].improv`变为 1，然后检查定时器标志并执行必要的操作。
  - 如果服务器使用 Proactor 模式（`m_actormodel`为 0），则会进行以下操作：
    - 调整与当前连接关联的定时器。
    - 调用`users[sockfd].write()`来将数据发送到套接字，并在成功发送时记录信息。
    - 如果发送失败，将调用`deal_timer`来处理定时器。

## `void WebServer::event_loop()`

- 作用：该函数是服务器的事件循环主体，用于处理各种事件，包括新连接、信号、读事件和写事件。
- 实现：
  - 通过调用`epoll_wait`等待事件的发生。
  - 遍历事件数组，根据不同的事件类型执行不同的操作。
  - 对于新连接事件，调用`deal_clinetdata`来处理。
  - 对于关闭、错误等事件，调用`deal_timer`来处理定时器。
  - 对于信号事件，调用`deal_with_signal`来处理信号。
  - 对于读事件，调用`deal_with_read`来处理。
  - 对于写事件，调用`deal_with_write`来处理。
  - 如果`timeout`标志为真，执行定时器处理，并重置`timeout`。

这些函数协同工作，使服务器能够在不同模式下处理客户端连接和事件，并进行定时器管理以确保连接的有效性。