#include <wait.h>
#include "public.h"
#include "tool/hash.h"
#include "WechatServiceClient.h"
#include "WechatServiceImpl.h"

WechatServiceClient &client = WechatServiceClient::getInstance();


void init_db_connections() {
    InitMysql();
    InitRedis();
}

void close_db_connections() {
    mysql_close(mysql_conn);
}

int login_user(char *username, char *passwd) {
    return mysql_verify(username, passwd);
}


int regist_user(char *user_name, char *passwd) {
    return regist_wechat_mysql(user_name, passwd);
}

bool delete_friend(const char *username, const char *friendname) {
    return delete_friend_mysql(username, friendname);
}

bool add_friend(const char *username, const char *friend_name) {
    return add_friend_mysql(username, friend_name);
}

void send_msg(int from_usr_id, char *msg, int friend_sock) {
    char cmd[1024];
    sprintf(cmd, "%d send you: %s", from_usr_id, msg);
    send(friend_sock, cmd, strlen(cmd), 0);
}

void RunRpcClient(char *json_request, char *reply) {
    message mm = {};
    json_2_msg(json_request, &mm);
    if (get_operation_type(mm.msg_type) == add) {
        client.AddFriend(mm.user_name, mm.friend_name, reply);
    }
    if (get_operation_type(mm.msg_type) == del) {
        client.DelFriend(mm.user_name, mm.friend_name, reply);
    }
}

void RpcServer() {
    init_db_connections();
    std::string server_address("0.0.0.0:50051");
    WechatServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // 等待服务器运行，直到需要关闭
    server->Wait();
}

int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }
    return 0;
}

int set_recv_timeout(int sockfd, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        return -1;
    }
    return 0;
}

int create_sock_and_bind() {
    int sockfd; // 主套接字
    struct sockaddr_in serv_addr; // 服务器地址结构
    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
    }
    // 设置套接字为非阻塞
    if (set_nonblocking(sockfd) < 0) {
        perror("set_nonblocking error\n");
    }

    // 设置接收超时时间
    if (set_recv_timeout(sockfd, 5) < 0) {
        exit(EXIT_FAILURE);
    }

    // 初始化服务器地址结构
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // 绑定套接字
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(EXIT_FAILURE);
    }

    // 监听套接字
    if (listen(sockfd, 5) < 0) {
        perror("ERROR on listen");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d\n", PORT);
    return sockfd;
}

void accept_client(int sockfd, struct epoll_event *event, int epollfd, int *max_fd) {
    struct sockaddr_in cli_addr; // 客户端地址结构
    socklen_t cli_len = sizeof(cli_addr); // 客户端地址长度
    // 接受新的客户端连接
    int connfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
    if (connfd < 0) {
        perror("ERROR on accept");
    }
    printf("New client connected from %s\n", inet_ntoa(cli_addr.sin_addr));
    // 将新的连接添加到epoll的监控列表中
    event->data.fd = connfd;
    event->events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, event) < 0) {
        perror("ERROR adding connfd to epoll");
        close(connfd); // 如果epoll_ctl失败，关闭connfd
    }
    // 更新最大文件描述符
    if (connfd > *max_fd) {
        *max_fd = connfd;
    }
}


void SendToClient(char *reply) {
    message mm = {};
    json_2_msg(reply, &mm);
    int id = get_id_by_name_mysql(mm.user_name);
    int *sockfd = find_in_hash(map_user_socket, id);
    if (sockfd) {
        send(*sockfd, reply, strlen(reply), 0);
    } else {
        save_msg_in_mysql(id, mm.content, id);
    }
}

void SendToFriend(char *user_name, char *friend_name, char *content) {
    int user_id = get_id_by_name_mysql(user_name);
    int friend_id = get_id_by_name_mysql(friend_name);
    int *sockfd = find_in_hash(map_user_socket, friend_id);
    if (sockfd) {
        send(*sockfd, content, strlen(content), 0);
    } else {
        save_msg_in_mysql(user_id, content, friend_id);
    }
}

void handle_client(int socket_fd) {
    char json_string[512];
    // 读取客户端发送的数据
    ssize_t read_size = recv(socket_fd, json_string, sizeof(json_string) - 1, 0);
    if (read_size <= 0) {
        int *p_user_id = find_in_hash(map_socket_user, socket_fd);
        if (p_user_id && *p_user_id > 0) {
            delete_from_hash(&map_user_socket, *p_user_id);
            delete_from_hash(&map_socket_user, socket_fd);
        }
        return;
    }
    message m = {};
    json_string[read_size] = '\0';
    json_2_msg(json_string, &m);
    if (strlen(m.msg_type) == 0) {
        return;
    }
    operation_type op = get_operation_type(m.msg_type);
    char reply_msg[512] = {0};
    switch (op) {
        case regist:
            m.user_id = regist_user(m.user_name, m.passwd);
            if(m.user_id > 0){
                sprintf(reply_msg, "%d REGISTER SUCCESS!", m.user_id);
                m.is_login = 1;
            } else{
                sprintf(reply_msg, "%d REGISTER FAILE!\n", m.user_id);
                m.is_login = 0;
            }
            break;
        case login:
            m.user_id = login_user(m.user_name, m.passwd);
            if (m.user_id == 0) {
                sprintf(reply_msg, "user_name or passwd not correct!\n");
                break;
            }
            sprintf(reply_msg, "%d LOGIN SUCCESS!", m.user_id);
            m.is_login = 1;
            break;
        case communicate:
            SendToFriend(m.user_name, m.friend_name, m.content);
            break;
        default:
            char reply[1024] = {};
            RunRpcClient(json_string, reply);
            send(socket_fd, reply, strlen(reply), 0);
            break;
    }
    if (m.user_id > 0 && (op == regist || op == login)) {
        strcpy(m.content, reply_msg);
        char *reply_json = msg_2_json(&m);
        send(socket_fd, reply_json, strlen(reply_json), 0);
        add_to_hash(&map_user_socket, m.user_id, socket_fd);
        add_to_hash(&map_socket_user, socket_fd, m.user_id);
        char offmsg[1024] = {};
        get_offline_msg_mysql(socket_fd, m.user_id, offmsg);//获取离线消息数量
        send(socket_fd, offmsg, strlen(offmsg), 0);
    }
    if (m.user_id == 0) {
        send(socket_fd, reply_msg, strlen(reply_msg), 0);
    }
}

int tools_init(int *max_fd, struct epoll_event *event, int *epollfd) {
    // 创建epoll实例
    *epollfd = epoll_create1(0);
    if (*epollfd < 0) {
        perror("ERROR opening epoll");
        exit(EXIT_FAILURE);
    }
    //创建并绑定套接字
    int sockfd = create_sock_and_bind();
    // 将sockfd添加到epoll的监控列表中
    (*event).data.fd = sockfd;
    (*event).events = EPOLLIN | EPOLLET;
    if (epoll_ctl(*epollfd, EPOLL_CTL_ADD, sockfd, event) < 0) {
        perror("ERROR adding sockfd to epoll");
        exit(EXIT_FAILURE);
    }
    *max_fd = sockfd;
    return sockfd;
}

void *process_connection(void *arg) {
    struct epoll_event event, events[MAX_EVENTS]; // epoll事件
    int n, max_fd = 0; // epoll事件数量和最大文件描述符
    int epollfd; // epoll实例文件描述符
    int sockfd = tools_init(&max_fd, &event, &epollfd);
    // epoll事件循环
    while (1) {
        n = epoll_wait(epollfd, events, MAX_EVENTS, 0);
        if (n < 0) {
            perror("epoll_wait error");
            exit(EXIT_FAILURE);
        }
        int i = 0;
        for (i = 0; i < n; i++) {
            if (events[i].data.fd == sockfd) {
                accept_client(sockfd, &event, epollfd, &max_fd);
            } else {
                handle_client(events[i].data.fd);
            }
        }
    }
}

void RunServer() {
    // 初始化数据库连接
    init_db_connections();
    pthread_t thread;
    if (pthread_create(&thread, NULL, process_connection, NULL)) {
        perror("Failed to create thread 1");
        return;
    }
    // 等待两个线程结束
    pthread_join(thread, NULL);
    // 服务器运行结束后关闭数据库连接
    close_db_connections();
}