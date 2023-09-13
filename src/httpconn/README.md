# http连接

1. `http_conn` 类定义了一系列枚举类型，包括请求方法 `METHOD`、检查请求状态 `CHECK_STATE`、HTTP 连接状态 `HTTP_CODE` 以及行解析状态 `LINE_STATE`。

2. 类中声明了一些常量，如文件名长度、读缓冲区大小、写缓冲区大小等。

3. `http_conn` 类的成员包括：
   - `m_sockfd`：连接套接字。
   - `m_address`：客户端地址信息。
   - `m_read_buf`：读缓冲区。
   - `m_read_idx`：当前读取位置。
   - `m_checked_idx`：当前解析位置。
   - `m_start_line`：当前行的起始位置。
   - `m_write_buf`：写缓冲区。
   - `m_write_idx`：当前写入位置。
   - `m_check_state`：解析状态。
   - `m_method`：请求方法。
   - `m_real_file`：请求的文件路径。
   - `m_url`：请求的URL。
   - `m_version`：HTTP 版本。
   - `m_host`：请求的主机名。
   - `m_content_length`：请求内容的长度。
   - `m_linger`：是否保持连接。
   - `m_file_address`：文件内存映射地址。
   - `m_file_stat`：文件状态信息。
   - `m_iv`：写缓冲区指针数组。
   - `m_iv_count`：写缓冲区指针数量。
   - `cgi`：是否启用 CGI。
   - `m_string`：存储请求头数据。
   - `bytes_to_send`：剩余发送字节数。
   - `bytes_have_send`：已经发送字节数。
   - `doc_root`：网站根目录路径。
   - `m_users`：保存用户名和密码的映射。
   - `m_trig_mode`：触发模式。
   - `m_close_log`：是否关闭日志。
   - `sql_user`：数据库用户名。
   - `sql_passward`：数据库密码。
   - `sql_name`：数据库名。
   - `m_epollfd`：epoll 文件描述符。
   - `m_user_count`：用户数量。
   - `mysql`：MySQL 连接。
   - `m_state`：连接状态。
   - `timer_flag`：计时器标志。
   - `improv`：是否启用性能优化。

4. `http_conn` 类中声明了一系列公有函数，包括：
   - `init`：初始化 HTTP 连接对象。
   - `close_conn`：关闭连接。
   - `process`：处理 HTTP 请求。
   - `read_once`：从客户端读取数据。
   - `write`：向客户端写入响应数据。
   - `get_address`：获取客户端地址结构的指针。
   - `initmysql_result`：初始化 MySQL 连接池。
   - `parse_request_line`：解析 HTTP 请求行。
   - `parse_headers`：解析 HTTP 请求头部。
   - `parse_content`：解析 HTTP 请求消息体。
   - `do_request`：处理 HTTP 请求的具体业务逻辑。
   - `get_line`：获取请求报文中当前行的起始位置。
   - `parse_line`：解析一行 HTTP 报文内容。
   - `unmap`：释放映射的文件内存。
   - `add_response`：将格式化的字符串添加到响应缓冲区中。
   - `add_content`：将指定内容添加到响应缓冲区中。
   - `add_state_line`：添加状态行到响应缓冲区中。
   - `add_headers`：添加响应头部信息到响应缓冲区中。
   - `add_content_type`：添加响应的内容类型到响应缓冲区中。
   - `add_content_length`：添加响应的内容长度到响应缓冲区中。
   - `add_linger`：添加连接状态到响应缓冲区中。
   - `add_blank_line`：添加空白行到响应缓冲区中。

这是 `http_conn` 类的实现文件，用于实现头文件中声明的各种成员函数以及一些全局函数。让我逐个解释这些函数的作用和实现。

## `set_nonblocking(int fd)` 函数

- 作用：将文件描述符设置为非阻塞模式。

- 实现：通过 `fcntl` 函数获取文件描述符的旧文件状态标志，然后使用按位或操作将 `O_NONBLOCK` 标志加入其中，最后再次调用 `fcntl` 函数将新的文件状态标志设置为文件描述符的状态。

## `add_fd(int epoll_fd, int fd, bool one_shot, int trig_mode)` 函数

- 作用：注册文件描述符到 epoll 事件表，可以选择是否开启 EPOLLONESHOT 模式。
- 实现：创建一个 `epoll_event` 结构体，设置其 `data.fd` 为待注册的文件描述符 `fd`，根据传入的触发模式设置事件类型，如果需要单次触发则添加 `EPOLLONESHOT` 标志。然后使用 `epoll_ctl` 函数将事件添加到 epoll 监听中，并通过 `set_nonblocking` 函数将文件描述符设置为非阻塞模式。

## `remove_fd(int epollfd, int fd)` 函数

- 作用：从内核事件表删除指定描述符。
- 实现：使用 `epoll_ctl` 函数的 `EPOLL_CTL_DEL` 操作删除指定的文件描述符，并关闭文件描述符。

## `modfd(int epollfd, int fd, int ev, int trig_mode)` 函数

- 作用：修改文件描述符在 epoll 中的事件监听模式。
- 实现：创建一个 `epoll_event` 结构体，根据传入的触发模式设置事件类型和触发条件。然后使用 `epoll_ctl` 函数的 `EPOLL_CTL_MOD` 操作修改文件描述符的事件监听模式。

## `http_conn::initmysql_result(connection_pool *conn_pool)` 函数

- 作用：初始化 MySQL 连接池，从数据库中读取用户名和密码，并存储在 `users` 对象中。
- 实现：首先获取一个 MySQL 连接，并执行 SQL 查询语句来获取用户名和密码信息。然后将查询结果存储到 `users` 对象中。

## `http_conn::close_conn(bool real_close)` 函数

- 作用：关闭连接，包括从 epoll 中移除监听，关闭文件描述符，减少用户数量。
- 实现：如果传入的参数 `real_close` 为真且套接字描述符有效（不为 -1），则通过 `remove_fd` 函数从 epoll 监听中移除连接，关闭套接字描述符，并减少用户数量。

## `http_conn::init(int sockfd, const sockaddr_in &addr, char *root, int trig_mode, int close_log, string user, string passward, string sqlname)` 函数

- 作用：初始化 `http_conn` 对象的各个成员变量，并将套接字描述符添加到 epoll 事件监听中。
- 实现：设置套接字描述符、客户端地址、根目录、触发模式和日志记录参数。然后调用 `init()` 函数进行初始化。

## `http_conn::init()` 函数

- 作用：初始化 `http_conn` 对象的各个成员变量。
- 实现：对 `http_conn` 的各个成员变量进行初始化，包括清空读写缓冲区、设置初始状态、标志等。

## `http_conn::parse_line()` 函数

- 作用：解析一行 HTTP 报文内容。
- 实现：遍历读缓冲区中的数据，查找行终止符（'\r\n'），如果找到则表示当前行已经完整读取，返回 `LINE_OK`，如果行未完整读取则返回 `LINE_OPEN`，如果不符合终止符要求则返回 `LINE_BAD`。

## `http_conn::read_once()` 函数

- 作用：从客户端读取数据，一次性读取一定数据量的数据。
- 实现：根据触发模式选择不同的读取方式。在 LT 模式下，使用阻塞读取，将读取到的数据存储在读缓冲区中。在 ET 模式下，使用非阻塞循环读取，直到没有数据可读或连接关闭为止。

## `http_conn::parse_request_line(char *text)` 函数

- 作用：解析 HTTP 请求行。
- 实现：这个函数解析 HTTP 请求行，包括请求方法、URL 和 HTTP 版本。它检查请求方法是否为 GET 或 POST，是否支持 HTTP/1.1 协议，以及对 URL 进行一些处理，确保其格式正确。如果解析失败或出现错误，会返回 `BAD_REQUEST`。

## `http_conn::parse_headers(char *text)` 函数

- 作用：解析 HTTP 请求头部。
- 实现：这个函数解析 HTTP 请求头部，包括 Connection、Content-length 和 Host 字段。它检查 Connection 字段以确定是否保持连接，解析 Content-length 字段以确定请求内容的长度，解析 Host 字段以获取主机信息。对于未知的请求头字段，会记录日志。函数返回 `NO_REQUEST` 表示继续解析请求头。

## `http_conn::parse_content(char *text)` 函数

- 作用：解析 HTTP 请求内容。
- 实现：这个函数用于解析 POST 请求中的请求体，例如用户名和密码。它根据 Content-length 字段确定请求内容的长度，并将请求体数据存储在 `m_string` 变量中。如果请求内容解析完毕，函数返回 `GET_REQUEST`，否则返回 `NO_REQUEST`。

## `http_conn::process_read()` 函数

- 作用：处理 HTTP 请求的读取和解析。
- 实现：这个函数是整个 HTTP 请求处理的核心。它循环解析每一行请求数据，包括请求行、请求头和请求内容。根据不同的解析状态和解析结果，执行相应的操作。如果解析到完整的 HTTP 请求，将调用 `do_request()` 函数执行具体的请求处理。

## `http_conn::do_request()` 函数

- 作用：处理 HTTP 请求，生成要返回给客户端的响应文件地址。
- 实现：根据请求的 URL 中的标识字符（例如，'0' 表示注册，'1' 表示登录），构建相应的响应文件路径。如果是 CGI 请求（标识字符为 '2' 或 '3'），则根据标志判断是登录检测还是注册检测，提取用户名和密码，并根据操作结果更新 URL。最后，根据 URL 构建完整的文件路径，并获取该文件的状态信息。如果文件不存在、不可读或是目录，会分别返回 `NO_RESOURCE`、`FORBIDDEN_REQUEST` 或 `BAD_REQUEST`。如果一切正常，函数返回 `FILE_REQUEST` 表示请求文件存在。

## `http_conn::unmap()` 函数

- 作用：解除映射文件。
- 实现：如果文件映射地址 `m_file_address` 不为空，使用 `munmap` 函数解除映射，释放内存。

## `http_conn::write()` 函数

- 作用：将数据写入客户端连接的发送缓冲区。
- 实现：该函数通过 `writev` 函数将数据写入客户端连接的发送缓冲区。它不断尝试写入数据，直到所有数据都写入或出现错误。如果写入过程中出现 `EAGAIN` 错误，表示发送缓冲区已满，会修改监听事件，等待下一次可写事件。如果写入成功，会更新已发送字节数 `bytes_have_send` 和待发送字节数 `bytes_to_send`。如果所有数据都已发送，解除文件映射并根据是否需要保持连接来决定如何处理连接。

## `http_conn::add_response(const char *format, ...)` 函数

- 作用：将格式化字符串添加到发送缓冲区。
- 实现：通过 `vsprintf` 函数将格式化的字符串添加到 `m_write_buf` 中，该函数支持可变参数列表。如果添加的字符串超过发送缓冲区的剩余空间，会截断字符串，确保不会超出缓冲区大小。函数返回 `true` 表示添加成功，否则返回 `false`。

## `http_conn::add_state_line(int state, const char *title)` 函数

- 作用：添加 HTTP 状态行到发送缓冲区。
- 实现：调用 `add_response` 函数，将 HTTP 状态行（如 "HTTP/1.1 200 OK"）添加到发送缓冲区中，其中 `state` 表示状态码，`title` 表示状态描述。

## `http_conn::add_headers(int content_len)` 函数

- 作用：添加 HTTP 响应头到发送缓冲区。
- 实现：通过调用一系列函数（`add_content_length`、`add_linger`、`add_blank_line`）依次添加内容长度、连接状态和空行到发送缓冲区。参数 `content_len` 表示响应内容的长度。

## `http_conn::add_content_length(int content_len)` 函数

- 作用：添加响应头中的 Content-Length 字段到发送缓冲区。
- 实现：调用 `add_response` 函数，将 Content-Length 字段添加到发送缓冲区中。

## `http_conn::add_content_type()` 函数

- 作用：添加响应头中的 Content-Type 字段到发送缓冲区。
- 实现：调用 `add_response` 函数，将 Content-Type 字段添加到发送缓冲区中，通常为 "text/html"。

## `http_conn::add_linger()` 函数

- 作用：添加响应头中的 Connection 字段到发送缓冲区，用于控制连接是否保持。
- 实现：调用 `add_response` 函数，根据成员变量 `m_linger` 的值（是否保持连接），将 Connection 字段添加到发送缓冲区中。

## `http_conn::add_blank_line()` 函数

- 作用：添加一个空行到发送缓冲区，用于分隔响应头和响应内容。
- 实现：调用 `add_response` 函数，将空行添加到发送缓冲区中。

## `http_conn::add_content(const char *content)` 函数

- 作用：添加响应内容到发送缓冲区。
- 实现：调用 `add_response` 函数，将响应内容（通常为 HTML 文本）添加到发送缓冲区中。

## `http_conn::process_write(HTTP_CODE ret)` 函数

- 作用：处理写操作，根据不同的 HTTP 状态码构建响应，准备发送给客户端。
- 实现：根据传入的 `ret` 参数，选择性地添加不同的响应头、响应内容，并设置数据发送的缓冲区（`m_iv` 数组）。最后，根据需要发送的数据长度设置 `bytes_to_send`。

## `http_conn::process()` 函数

- 作用：处理 HTTP 请求的入口函数，调用 `process_read` 进行请求解析，然后根据解析结果调用 `process_write` 处理响应，最后修改 epoll 监听事件以实现响应的发送或连接的关闭。
