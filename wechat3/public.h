//
// Created by HP on 2024/9/12.
//

#ifndef WECHAT3_PUBLIC_H
#define WECHAT3_PUBLIC_H

#include <iostream>
#include <sys/socket.h>
#include <cstdio>
#include "tool/cjson.h"
#include <cerrno>
#include <pthread.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <mysql/mysql.h>
#include <hiredis/hiredis.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cctype>
#include <unistd.h>
#include <pthread.h>
#include <grpc/grpc.h>
#include <grpc/support/log.h>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <string>
#include <grpcpp/grpcpp.h>
#include "proto/wechat.grpc.pb.h"
#include "tool/uthash.h"
#include "proto/wechat.pb.h"
#include <ctime>
#include <csignal>

#define MAX_EVENTS 10
typedef struct {
    int user_id;    // 哈希表的键
    int sock_fd;  // 哈希表的值
    UT_hash_handle hh;  // 哈希句柄
} IntHash;

typedef struct message {
    char msg_type[128];
    int user_id;
    char user_name[128];
    char passwd[128];
    char friend_name[128];
    char content[1024];
    int is_login;
} message;


#define PORT 8081
#define MAX_EVENTS 10

extern const char *hostname;
extern const char *username;
extern const char *password; // MySQL密码
extern const char *dbname; // 要连接的数据库名
extern redisContext *redis_ctx;
extern MYSQL *mysql_conn;

void RunServer();

void RunClient();

void json_2_msg(const char *json_string, message *msg_out);

char *msg_2_json(const message *msg);

void collect_user_regist_log_input(message *msg);

void collect_user_input(message *msg);

void init_db_connections();

void RpcServer();

void close_db_connections();

bool add_friend(const char *uername, const char *friendname);

bool delete_friend(const char *username, const char *friendname);

int regist_user(char *writer_name, char *passwd);

void InitMysql();

void InitRedis();

int regist_wechat_mysql(char *user_name, char *passwd);

int get_id_by_name_mysql(const char *name);

void SendToFriend(char *user_name, char *friend_name, char *content);

bool add_friend_mysql(const char *username, const char *friendname);

bool delete_friend_mysql(const char *username, const char *friend_name);

void save_msg_in_mysql(int user_id, char *content, int friend_id);

void get_offline_msg_mysql(int socket_fd, int userid, char *offmsg);

int login_user(char *username, char *passwd);

int mysql_verify(char *username, char *passwd);

typedef enum {
    login,
    del,
    regist,
    add,
    communicate,
    quit,
    operation_unkown // 添加一个未知操作的枚举值
} operation_type;

operation_type get_operation_type(const char *str);

#endif //WECHAT3_PUBLIC_H
