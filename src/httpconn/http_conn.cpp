#include "http_conn.h"

//http响应的状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

locker m_lock;
map<string, string> users;
int http_conn::m_epollfd = -1;//epoll文件描述符
int http_conn::m_user_count = 0;//用户数量

/**
 * @brief 设置文件描述符为非阻塞模式
 * @param fd 文件描述符
*/
int set_nonblocking(int fd){
    
    int old_option = fcntl(fd, F_GETFL);// 获取文件描述符的旧的文件状态标志
    int new_option = old_option | O_NONBLOCK;// 将旧的文件状态标志与 O_NONBLOCK 进行按位或操作，设置为非阻塞模式
    fcntl(fd, F_SETFL, new_option);// 将新的文件状态标志设置为文件描述符的状态
    return old_option;// 返回旧的文件状态标志，以便在需要时进行恢复
}

/**
 * @brief 注册文件描述符到 epoll 事件表,ET模式，开启EPOLLONESHOT
 * @param epollfd epoll文件描述符
 * @param fd 要注册的文件描述符
 * @param one_shot 是否开启EPOLLONESHOT模式
 * @param trig_mode 触发模式
*/
void add_fd(int epoll_fd,int fd,bool one_shot,int trig_mode){
    epoll_event event;
    event.data.fd = fd;

    // 根据传入的触发模式设置事件类型
    if(1==trig_mode){
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;//监听可读事件（有数据可读），使用边缘触发模式，并且可以检测连接的对端关闭
    }else{
        event.events = EPOLLIN | EPOLLRDHUP;//监听可读事件（有数据可读），并且可以检测连接的对端关闭
    }

    // 如果需要单次触发，则添加 EPOLLONESHOT 标志（事件只触发一次后即从epoll示例的就绪队列移除）
    if(one_shot){
        event.events |= EPOLLONESHOT;
    }

    // 将事件添加到 epoll 监听中
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);

    // 设置文件描述符为非阻塞模式
    set_nonblocking(fd);
}

/**
 * @brief 从内核时间表删除描述符
 * @param epollfd epoll文件描述符
 * @param fd 要删除的文件描述符
*/
void remove_fd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

/**
 * @brief 修改文件描述符在 epoll 中的事件监听模式。
 * 
 * @param epollfd epoll 实例的文件描述符。
 * @param fd 要修改监听模式的文件描述符。
 * @param ev 要设置的事件类型，如 EPOLLIN、EPOLLOUT 等。
 * @param trig_mode 触发模式，1 表示 ET 模式，0 表示 LT 模式。
 */
void modfd(int epollfd, int fd, int ev, int trig_mode)
{
    epoll_event event;
    event.data.fd = fd;
    
    // 根据触发模式设置事件类型和触发条件
    if (1==trig_mode)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    // 调用 epoll_ctl 函数修改文件描述符的事件监听模式
    // EPOLL_CTL_MOD 表示修改现有的事件监听
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void http_conn::initmysql_result(connection_pool *conn_pool){
    printf("http_conn::initmysql_result(connection_pool *conn_pool)\n");
    // 连接池取出
    MYSQL *mysql = NULL;
    connection_raii mysql_con(&mysql, conn_pool);

    //在user表中检索username、passward，浏览器端输入
    if(mysql_query(mysql,"SELECT username,passwd FROM user")){
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }

    //从表中检索出完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    //返回结果集中的列数
    int num_fields = mysql_num_fields(result);

    //返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}

void http_conn::close_conn(bool real_close){
    printf("http_conn::close_conn(bool real_close)\n");
    // 有效连接且确认关闭
    if (real_close && (m_sockfd != -1))
    {
        printf("close %d\n", m_sockfd);
        remove_fd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    } 
}

void http_conn::init(int sockfd, const sockaddr_in &addr, char *root, int trig_mode, int close_log, string user, string passward, string sqlname){
    printf("http_conn::init(int sockfd, const sockaddr_in &addr, char *root, int trig_mode, int close_log, string user, string passward, string sqlname)\n");
    m_sockfd = sockfd;
    m_address = addr;
    // 将套接字描述符添加到epoll事件监听中
    add_fd(m_epollfd, sockfd, true, m_trig_mode);
    m_user_count++;

    // 设置网站根目录、触发模式和日志记录参数
    doc_root = root;
    m_trig_mode = trig_mode;
    m_close_log = close_log;
    // 将数据库用户名、密码和名称复制到对应成员变量
    strcpy(sql_user, user.c_str());
    strcpy(sql_passward, passward.c_str());
    strcpy(sql_name, sqlname.c_str());

    init();

}

void http_conn::init(){
    printf("http_conn::init()\n");
    mysql = NULL;
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    cgi = 0;
    m_state = 0;
    timer_flag = 0;
    improv = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

http_conn::LINE_STATE http_conn::parse_line(){
    printf("http_conn::LINE_STATE http_conn::parse_line()\n");
    char temp;
    // 遍历读缓冲区中的数据
    for (; m_checked_idx < m_read_idx;++m_checked_idx){
        temp = m_read_buf[m_checked_idx];
        if(temp == '\r'){
            if((m_checked_idx+1) == m_read_idx)
                return LINE_OPEN; // 行未完整读取，需要继续读取
            else if(m_read_buf[m_checked_idx+1] == '\n'){
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;// 行已经完整读取，返回 LINE_OK
            }
            return LINE_BAD;// 不符合行终止符要求，返回 LINE_BAD
        }else if(temp == '\n'){
            if(m_checked_idx>1 && m_read_buf[m_checked_idx-1] == '\r'){
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;// 行已经完整读取，返回 LINE_OK
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

bool http_conn::read_once(){
    printf("http_conn::read_once()\n");
    if (m_read_idx >= READ_BUFFER_SIZE)
        return false;// 读缓冲区已满，无法继续读取数据

    int bytes_read = 0;

    //LT模式，阻塞读取
    if(0==m_trig_mode){
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        m_read_idx += bytes_read;

        if(bytes_read<=0)
            return false;
        return true;
    }

    //ET模式，非阻塞循环读取
    else{
        while(true){
            bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
            if(bytes_read==-1){
                if(errno==EAGAIN||errno==EWOULDBLOCK)break;
                return false;// 读取数据暂时不可用，跳出循环
            }else if(bytes_read==0)
                return false;// 连接已关闭，返回 false

            m_read_idx += bytes_read;
        }
        return true;
    }
}

http_conn::HTTP_CODE http_conn::parse_request_line(char *text){
    printf("http_conn::parse_request_line(char *text)\n");
    m_url = strpbrk(text, " \t");
    if(!m_url){
        printf("1\n");
        return BAD_REQUEST;
    }
        

    *m_url++ = '\0';
    char *method = text;
    if(strcasecmp(method,"GET")==0)
        m_method = GET;
    
    else if(strcasecmp(method,"POST")==0){
        m_method = POST;
        cgi = 1;
    }else{
        printf("2\n");
        return BAD_REQUEST;
    }
        

    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if(!m_version){
        printf("3\n");
        return BAD_REQUEST;
    }
        
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    if(strcasecmp(m_version,"HTTP/1.1")!=0){
        printf("4\n");
        return BAD_REQUEST;//只支持HTTP1.1协议
    }
        

    if(strncasecmp(m_url,"http://",7)==0){
        m_url += 7;
        m_url = strchr(m_url, '/');
    }

    if(strncasecmp(m_url,"https://",8)==0){
        m_url += 8;
        m_url = strchr(m_url, '/');
    }

    if(!m_url || m_url[0]!='/'){
        printf("5\n");
        return BAD_REQUEST;// URL格式错误，返回400 Bad Request
    }

    // 当URL为'/'时，将其替换为默认页面路径
    if(strlen(m_url)==1)
        strcat(m_url, "judge.html");

    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//解析请求头
http_conn::HTTP_CODE http_conn::parse_headers(char *text){

    printf("http_conn::parse_headers(char *text)\n");
    if (text[0] == '\0')
    {
        if(m_content_length != 0){
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if(strncasecmp(text, "Connection:", 11) == 0){
        text += 11;
        text += strspn(text, " \t");
        if(strcasecmp(text, "keep-alive") == 0){
            m_linger = true;//如果请求头中包含 Connection: keep-alive，设置为长连接
        }
    }else if(strncasecmp(text, "Content-length:", 15) == 0){
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);//解析请求头中的 Content-length 字段，用于后续请求内容解析
    }else if (strncasecmp(text, "Host:", 5) == 0){
        text += 5;
        text += strspn(text, " \t");
        m_host = text;//解析请求头中的 Host 字段，用于后续处理主机信息
    }else{
        LOG_INFO("oop!unknow header: %s", text);//未知的请求头字段
    }

    return NO_REQUEST;//继续解析请求头
}

//判断http请求是否被完整读入
http_conn::HTTP_CODE http_conn::parse_content(char *text){
    printf("http_conn::parse_content(char *text)\n");
    if (m_read_idx >= (m_content_length + m_checked_idx))
    {
        text[m_content_length] = '\0';
        //POST请求中最后为输入的用户名和密码
        m_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;

}

http_conn::HTTP_CODE http_conn::process_read(){
    printf("http_conn::process_read()\n");

    LINE_STATE line_state = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;
    //循环解析每一行请求数据，直到请求内容解析完毕或出错
    while((m_check_state == CHECK_STATE_CONTENT && line_state == LINE_OK) || ((line_state = parse_line()) == LINE_OK)){
        text = get_line();//获取当前行的文本数据
        m_start_line = m_checked_idx;//更新下一行的起始索引
        LOG_INFO("%s", text);//打印当前行的内容，用于调试
        printf("m_check_state:%d,line state:%d\n", m_check_state, line_state);
        // 根据当前解析状态进行不同的处理
        switch (m_check_state){
            //解析请求行
            case CHECK_STATE_REQUESTLINE:
            {
            printf("%s\n", text);
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST)
            {
                printf("6\n");
                return BAD_REQUEST;
                }
                break;
            }
            //解析请求头
            case CHECK_STATE_HEADER:
            {
                ret = parse_headers(text);
                if (ret == BAD_REQUEST){
                    printf("7\n");
                    return BAD_REQUEST;
                }else if(ret == GET_REQUEST)
                {
                    return do_request();//请求内容解析完毕，执行请求处理
                }
                break;
            }
            //解析请求内容
            case CHECK_STATE_CONTENT:
            {
                ret = parse_content(text);
                if (ret == GET_REQUEST)
                    return do_request();
                line_state = LINE_OPEN;//重置行状态为 LINE_OPEN，继续解析下一行内容
                break;
            }
            default:
                return INTERNAL_ERROR;
            }
    }
    return NO_REQUEST; //解析完毕，返回状态 NO_REQUEST

}

//生成要返回给客户端的响应文件地址
http_conn::HTTP_CODE http_conn::do_request(){
    printf("http_conn::do_request()\n");

    strcpy(m_real_file, doc_root);
    // printf("%s", m_real_file);
    int len = strlen(doc_root);
    //printf("m_url:%s\n", m_url);
    const char *p = strrchr(m_url, '/');

    //处理cgi
    if(cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3')){

        //根据标志判断是登录检测还是注册检测
        char flag = m_url[1];

        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/");
        strcat(m_url_real, m_url + 2);
        strncpy(m_real_file + len, m_url_real, FILENAME_LEN - len - 1);
        free(m_url_real);

        //将用户名和密码提取出来
        //user=123&passwd=123
        char name[100], password[100];
        int i;
        for (i = 5; m_string[i] != '&'; ++i)
            name[i - 5] = m_string[i];
        name[i - 5] = '\0';

        int j = 0;
        for(i = i + 10; m_string[i] != '\0'; ++i, ++j)
            password[j] = m_string[i];
        password[j] = '\0';

        if(*(p + 1) == '3'){
            //如果是注册，先检测数据库中是否有重名的
            //没有重名的，进行增加数据
            char *sql_insert = (char *)malloc(sizeof(char) * 200);
            strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, password);
            strcat(sql_insert, "')");

            if(users.find(name) == users.end()){
                m_lock.lock();
                int res = mysql_query(mysql, sql_insert);
                users.insert(pair<string, string>(name, password));
                m_lock.unlock();

                if (!res)
                    strcpy(m_url, "/log.html");
                else
                    strcpy(m_url, "/registerError.html");
            }else
                strcpy(m_url, "/registerError.html");
        }
        //如果是登录，直接判断
        //若浏览器端输入的用户名和密码在表中可以查找到，返回1，否则返回0
        else if(*(p + 1) == '2')
        {
            if(users.find(name) != users.end() && users[name] == password)
                strcpy(m_url, "/welcome.html");
            else
                strcpy(m_url, "/logError.html");
        }
    }

    if(*(p + 1) == '0'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/register.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }else if(*(p + 1) == '1'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/log.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }else if(*(p + 1) == '5'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/picture.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }else if(*(p + 1) == '6'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/video.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }else if(*(p + 1) == '7'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/fans.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else
        strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);

    if(stat(m_real_file, &m_file_stat) < 0)
        return NO_RESOURCE;

    if(!(m_file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    if(S_ISDIR(m_file_stat.st_mode)){
        printf("8\n");
        return BAD_REQUEST;
    }
        

    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;

}

void http_conn::unmap(){
    if(m_file_address){
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}


//将数据写入客户端连接的发送缓冲区
bool http_conn::write(){
    printf("http_conn::write()\n");
    int temp = 0;

    if(bytes_to_send == 0){
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_trig_mode);
        init();
        return true;
    }

    while(1){
        temp = writev(m_sockfd, m_iv, m_iv_count);

        if (temp < 0)
        {
            if (errno == EAGAIN)
            {
                modfd(m_epollfd, m_sockfd, EPOLLOUT, m_trig_mode);
                return true;
            }
            unmap();
            return false;
        }

        bytes_have_send += temp;
        bytes_to_send -= temp;
        if (bytes_have_send >= m_iv[0].iov_len)
        {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        }
        else
        {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }

        if (bytes_to_send <= 0)
        {
            unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN, m_trig_mode);

            if (m_linger)
            {
                init();
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}

bool http_conn::add_response(const char *format,...){
    printf("http_conn::add_response(const char *format,...)\n");
    if (m_write_idx >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if(len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx)){
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);

    LOG_INFO("request:%s", m_write_buf);

    return true;

}

bool http_conn::add_state_line(int state,const char *title){
    printf("http_conn::add_state_line(int state,const char *title)\n");
    return add_response("%s %d %s\r\n", "HTTP/1.1", state, title);
}

bool http_conn::add_headers(int content_len){
    printf("http_conn::add_headers(int content_len)\n");
    return add_content_length(content_len) && add_linger() && add_blank_line();
}

bool http_conn::add_content_length(int content_len){
    printf("http_conn::add_content_length(int content_len)\n");
    return add_response("Content-Length:%d\r\n", content_len);
}

bool http_conn::add_content_type(){
    printf("http_conn::add_content_type()\n");
    return add_response("Content-Type:%s\r\n", "text/html");
}

bool http_conn::add_linger(){
    printf("http_conn::add_linger()\n");
    return add_response("Connection:%s\r\n", (m_linger == true) ? "keep-alive" : "close");
}

bool http_conn::add_blank_line(){
    printf("http_conn::add_blank_line()\n");
    return add_response("%s", "\r\n");
}

bool http_conn::add_content(const char *content){
    printf("http_conn::add_content(const char *content)\n");
    return add_response("%s", content);
}

bool http_conn::process_write(HTTP_CODE ret){
    printf("http_conn::process_write(HTTP_CODE ret)\n");
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        add_state_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
            return false;
        break;
    }
    case BAD_REQUEST:
    {
        add_state_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
            return false;
        break;
    }
    case FORBIDDEN_REQUEST:
    {
        add_state_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
            return false;
        break;
    }
    case FILE_REQUEST:
    {
        add_state_line(200, ok_200_title);
        if (m_file_stat.st_size != 0)
        {
            add_headers(m_file_stat.st_size);
            m_iv[0].iov_base = m_write_buf;
            m_iv[0].iov_len = m_write_idx;
            m_iv[1].iov_base = m_file_address;
            m_iv[1].iov_len = m_file_stat.st_size;
            m_iv_count = 2;
            bytes_to_send = m_write_idx + m_file_stat.st_size;
            return true;
        }
        else
        {
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string))
                return false;
        }
    }
    default:
        return false;
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

void http_conn::process(){
    printf("http_conn::process()\n");
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_trig_mode);
        return;
    }
    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        close_conn();
    }
    modfd(m_epollfd, m_sockfd, EPOLLOUT, m_trig_mode);
}


