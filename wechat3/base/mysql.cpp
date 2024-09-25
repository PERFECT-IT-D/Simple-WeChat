//
// Created by HP on 2024/9/12.
//
#include "../public.h"

MYSQL *mysql_conn = nullptr;
const char *hostname = "localhost";
const char *username = "root";
const char *password = "ecs-user"; // MySQL密码
const char *dbname = "wechat"; // 要连接的数据库名
void get_data_with_cache(char *cached_result, const char *query_key, const char *sql_query) {
    redisReply *reply = (redisReply *) redisCommand(redis_ctx, "GET %s", query_key);
    if (reply != NULL) {
        if (reply->type == REDIS_REPLY_STRING) {
            strcpy(cached_result, reply->str);
            freeReplyObject(reply);
        } else {
            freeReplyObject(reply);
            // 执行 MySQL 查询
            if (mysql_query(mysql_conn, sql_query)) {
                fprintf(stderr, "MySQL query error: %s\n", mysql_error(mysql_conn));
                return;
            }
            MYSQL_RES *result = mysql_store_result(mysql_conn);
            if (result == NULL) {
                fprintf(stderr, "MySQL store result error: %s\n", mysql_error(mysql_conn));
                return;
            }
            // 将结果复制到缓存
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row) {
                strcpy(cached_result, row[0]);
                reply = (redisReply *) redisCommand(redis_ctx, "SET %s %s EX 1000", query_key, cached_result);
                //要有缓存失效时间
                freeReplyObject(reply); // 释放回复对象
            } else {
                printf("No data found\n");
            }
            mysql_free_result(result);
        }
    } else {
        fprintf(stderr, "Redis command failed\n");
    }
}
void InitMysql() {
    mysql_conn = mysql_init(NULL);
    if (mysql_conn == NULL) {
        fprintf(stderr, "MySQL init failed\n");
        exit(EXIT_FAILURE);
    }
    if (!mysql_real_connect(mysql_conn, hostname, username, password, dbname, 0, NULL, 0)) {
        fprintf(stderr, "MySQL connect error: %s\n", mysql_error(mysql_conn));
        exit(EXIT_FAILURE);
    }
}

int regist_wechat_mysql(char *user_name, char *passwd) {
    // 准备 SQL 语句
    char query_insert[256];
    char query_select[256];
    sprintf(query_insert, "INSERT INTO t_wechat_users (user_name, user_passwd) VALUES ('%s', '%s')", user_name, passwd);
    sprintf(query_select, "SELECT user_id FROM t_wechat_users WHERE user_name = '%s'", user_name);
    // 执行插入操作
    if (mysql_query(mysql_conn, query_insert)) {
        fprintf(stderr, "MySQL insert error: %s\n", mysql_error(mysql_conn));
        return 0;
    }

    // 执行查询操作
    if (mysql_query(mysql_conn, query_select)) {
        fprintf(stderr, "MySQL select error: %s\n", mysql_error(mysql_conn));
        return 0;
    }

    // 获取查询结果
    MYSQL_RES *result = mysql_store_result(mysql_conn);
    if (result == NULL) {
        fprintf(stderr, "MySQL store result error: %s\n", mysql_error(mysql_conn));
        return 0;
    }

    // 获取查询结果行
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL) {
        fprintf(stderr, "No user found with user_name: %s\n", user_name);
        mysql_free_result(result);
        return 0;
    }

    // 提取 user_id
    int user_id = atoi(row[0]);
    mysql_free_result(result);
    return user_id;
}
int mysql_verify(char *user_name, char *passwd) {
    char _query[256];
    sprintf(_query, "SELECT user_id FROM t_wechat_users WHERE user_name = '%s' AND user_passwd = '%s'", user_name,
            passwd);
    if (mysql_query(mysql_conn, _query)) {
        fprintf(stderr, "MySQL query error: %s\n", mysql_error(mysql_conn));
        return 0;
    }
    MYSQL_RES *result = mysql_store_result(mysql_conn);
    if (result == NULL) {
        fprintf(stderr, "MySQL store result error: %s\n", mysql_error(mysql_conn));
        return 0;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == NULL || atoi(row[0]) == 0) {
        // 密码错误或没有找到对应的 writer_id
        fprintf(stderr, "Invalid password or writer_id not found\n");
        mysql_free_result(result);
        return 0;
    } else {
        int user_id = atoi(row[0]);
        return user_id;
    }
}

bool add_friend_mysql(const char *username, const char *friendname) {
    // 获取调用此函数的用户的ID，通过用户名
    int user_id = get_id_by_name_mysql(username);
    // 获取被添加为朋友的用户的ID，通过朋友名
    int friend_id = get_id_by_name_mysql(friendname);

    // 检查获取到的user_id或friend_id是否为-1，如果是，则表示用户不存在
    if (user_id == -1 || friend_id == -1) {
        fprintf(stderr, "One or both users do not exist!\n");
        return false;
    }

    // 初始化预处理语句对象
    MYSQL_STMT *stmt = mysql_stmt_init(mysql_conn);
    // 检查stmt是否初始化成功，如果失败则输出错误信息并返回false
    if (!stmt) {
        fprintf(stderr, "MySQL stmt init error: %s\n", mysql_error(mysql_conn));
        return false;
    }

    // 定义SQL插入语句，使用问号?作为参数占位符
    const char *query = "INSERT INTO t_friend_ship (user_id, friend_id) VALUES (?, ?)";
    // 使用预处理语句准备执行SQL命令
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        fprintf(stderr, "MySQL prepare error: %s\n", mysql_error(mysql_conn));
        mysql_stmt_close(stmt);
        return false;
    }

    // 定义两个MYSQL_BIND结构体数组，用于绑定user_id和friend_id到预处理语句的占位符
    MYSQL_BIND param[2];
    // 使用memset函数将param数组的所有元素初始化为0
    memset(param, 0, sizeof(param));

    // 绑定第一个参数，user_id
    param[0].buffer_type = MYSQL_TYPE_LONG; // 参数类型为长整型
    param[0].buffer = (char *)&user_id;     // 参数的内存地址
    // 绑定第二个参数，friend_id
    param[1].buffer_type = MYSQL_TYPE_LONG; // 参数类型为长整型
    param[1].buffer = (char *)&friend_id;   // 参数的内存地址

    // 将param数组绑定到预处理语句stmt上
    if (mysql_stmt_bind_param(stmt, param)) {
        fprintf(stderr, "MySQL bind param error: %s\n", mysql_error(mysql_conn));
        mysql_stmt_close(stmt);
        return false;
    }

    // 执行预处理语句
    if (mysql_stmt_execute(stmt)) {
        int error_code = mysql_stmt_errno(stmt); // 获取错误码
        fprintf(stderr, "MySQL execute error %d: %s\n", error_code, mysql_stmt_error(stmt)); // 输出错误信息
        // 如果错误码为1062，表示尝试插入的记录在数据库中已存在，即两个用户已经是好友
        if (error_code == 1062) {
            fprintf(stderr, "Already friends!\n");
        } else {
            fprintf(stderr, "Failed to add friend.\n");
        }
        mysql_stmt_close(stmt);
        return false;
    }

    // 检查是否有记录被插入，如果没有，可能是两个用户已经是好友
    if (mysql_stmt_affected_rows(stmt) == 0) {
        fprintf(stderr, "No rows inserted. They might already be friends.\n");
        mysql_stmt_close(stmt);
        return false;
    }

    // 关闭预处理语句，释放资源
    mysql_stmt_close(stmt);
    // 插入成功，返回true
    return true;
}

bool delete_friend_mysql(const char *username, const char *friendname) {
    // 获取用户名对应的用户ID
    int user_id = get_id_by_name_mysql(username);
    // 获取朋友名对应的用户ID
    int friend_id = get_id_by_name_mysql(friendname);

    // 如果friend_id无效（-1表示未找到），则返回false
    if (friend_id == -1 || user_id == -1) {
        fprintf(stderr, "One or both users do not exist!\n");
        return false;
    }

    // 初始化预处理语句对象
    MYSQL_STMT *stmt = mysql_stmt_init(mysql_conn);
    // 检查stmt是否初始化成功，如果失败则输出错误信息并返回false
    if (!stmt) {
        fprintf(stderr, "MySQL stmt init error: %s\n", mysql_error(mysql_conn));
        return false;
    }

    // 定义SQL删除语句，使用问号?作为参数占位符
    const char *query = "DELETE FROM t_friend_ship WHERE user_id = ? AND friend_id = ?";
    // 使用预处理语句准备执行SQL命令
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        fprintf(stderr, "MySQL prepare error: %s\n", mysql_error(mysql_conn));
        mysql_stmt_close(stmt);
        return false;
    }

    // 定义两个MYSQL_BIND结构体数组，用于绑定user_id和friend_id到预处理语句的占位符
    MYSQL_BIND param[2];
    // 使用memset函数将param数组的所有元素初始化为0
    memset(param, 0, sizeof(param));

    // 绑定第一个参数，user_id
    param[0].buffer_type = MYSQL_TYPE_LONG; // 参数类型为长整型
    param[0].buffer = (char *)&user_id;     // 参数的内存地址

    // 绑定第二个参数，friend_id
    param[1].buffer_type = MYSQL_TYPE_LONG; // 参数类型为长整型
    param[1].buffer = (char *)&friend_id;   // 参数的内存地址

    // 将param数组绑定到预处理语句stmt上
    if (mysql_stmt_bind_param(stmt, param)) {
        fprintf(stderr, "MySQL bind param error: %s\n", mysql_error(mysql_conn));
        mysql_stmt_close(stmt);
        return false;
    }

    // 执行预处理语句
    if (mysql_stmt_execute(stmt)) {
        fprintf(stderr, "MySQL execute error: %s\n", mysql_error(mysql_conn));
        mysql_stmt_close(stmt);
        return false;
    }

    // 检查是否有记录被删除，如果没有，输出提示信息
    if (mysql_stmt_affected_rows(stmt) == 0) {
        fprintf(stderr, "No rows deleted. They might not be friends.\n");
        mysql_stmt_close(stmt);
        return false;
    }

    // 关闭预处理语句，释放资源
    mysql_stmt_close(stmt);
    // 删除成功，返回true
    return true;
}

int get_id_by_name_mysql(const char *name) {
    // 检查输入是否为空
    if (name == NULL) {
        fprintf(stderr, "Invalid user name\n");
        return -1;
    }

    // 初始化预处理语句
    MYSQL_STMT *stmt = mysql_stmt_init(mysql_conn);
    if (!stmt) {
        fprintf(stderr, "MySQL stmt init error: %s\n", mysql_error(mysql_conn));
        return -1;
    }

    // 定义SQL查询语句，使用问号?作为参数占位符
    const char *query = "SELECT user_id FROM t_wechat_users WHERE user_name = ?";
    // 使用预处理语句准备执行SQL命令
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        fprintf(stderr, "MySQL prepare error: %s\n", mysql_error(mysql_conn));
        mysql_stmt_close(stmt);
        return -1;
    }

    // 定义MYSQL_BIND结构体数组，用于绑定参数
    MYSQL_BIND param[1];
    memset(param, 0, sizeof(param));

    // 绑定参数，user_name
    param[0].buffer_type = MYSQL_TYPE_STRING;
    param[0].buffer = (char*)name;         // 参数的内存地址
    param[0].buffer_length = strlen(name); // 参数的长度

    // 将param数组绑定到预处理语句stmt上
    if (mysql_stmt_bind_param(stmt, param)) {
        fprintf(stderr, "MySQL bind param error: %s\n", mysql_error(mysql_conn));
        mysql_stmt_close(stmt);
        return -1;
    }

    // 执行预处理语句
    if (mysql_stmt_execute(stmt)) {
        fprintf(stderr, "MySQL execute error: %s\n", mysql_error(mysql_conn));
        mysql_stmt_close(stmt);
        return -1;
    }

    // 获取查询结果
    MYSQL_RES *result = mysql_stmt_result_metadata(stmt);
    if (result == NULL) {
        fprintf(stderr, "MySQL get result error: %s\n", mysql_error(mysql_conn));
        mysql_stmt_close(stmt);
        return -1;
    }

    // 将结果绑定到C变量
    MYSQL_BIND read_bind[1];
    memset(read_bind, 0, sizeof(read_bind));

    int user_id;
    read_bind[0].buffer_type = MYSQL_TYPE_LONG;
    read_bind[0].buffer = (char *)&user_id;

    if (mysql_stmt_bind_result(stmt, read_bind)) {
        fprintf(stderr, "MySQL bind result error: %s\n", mysql_error(mysql_conn));
        mysql_free_result(result);
        mysql_stmt_close(stmt);
        return -1;
    }

    // 获取一行结果
    if (mysql_stmt_fetch(stmt) == MYSQL_DATA_TRUNCATED) {
        fprintf(stderr, "Data truncated\n");
        mysql_free_result(result);
        mysql_stmt_close(stmt);
        return -1;
    }



    // 释放资源
    mysql_free_result(result);
    mysql_stmt_close(stmt);

    // 返回获取到的用户ID
    return user_id;
}

void save_msg_in_mysql(int user_id, char *content, int friend_id) {
    // 检查内容是否为空
    if (content == NULL) {
        fprintf(stderr, "Message content is null\n");
        return;
    }

    // 初始化预处理语句
    MYSQL_STMT *stmt = mysql_stmt_init(mysql_conn);
    if (!stmt) {
        fprintf(stderr, "MySQL stmt init error: %s\n", mysql_error(mysql_conn));
        return;
    }

    // 定义SQL插入语句，使用问号?作为参数占位符
    const char *query = "INSERT INTO t_save_msg (user_id, msg, friend_id) VALUES (?, ?, ?)";
    // 使用预处理语句准备执行SQL命令
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        fprintf(stderr, "MySQL prepare error: %s\n", mysql_error(mysql_conn));
        mysql_stmt_close(stmt);
        return;
    }

    // 定义MYSQL_BIND结构体数组，用于绑定参数
    MYSQL_BIND param[3];
    memset(param, 0, sizeof(param));

    // 绑定第一个参数，user_id
    param[0].buffer_type = MYSQL_TYPE_LONG;
    param[0].buffer = (char *)&user_id;

    // 绑定第二个参数，content
    param[1].buffer_type = MYSQL_TYPE_STRING;
    param[1].buffer = content;
    param[1].buffer_length = strlen(content);

    // 绑定第三个参数，friend_id
    param[2].buffer_type = MYSQL_TYPE_LONG;
    param[2].buffer = (char *)&friend_id;

    // 将param数组绑定到预处理语句stmt上
    if (mysql_stmt_bind_param(stmt, param)) {
        fprintf(stderr, "MySQL bind param error: %s\n", mysql_error(mysql_conn));
        mysql_stmt_close(stmt);
        return;
    }

    // 执行预处理语句
    if (mysql_stmt_execute(stmt)) {
        fprintf(stderr, "MySQL execute error: %s\n", mysql_error(mysql_conn));
    }
    // 关闭预处理语句，释放资源
    mysql_stmt_close(stmt);
}

void get_offline_msg_mysql(int socket_fd, int userid, char *offmsg) {
    char query[256];
    MYSQL_RES *result = NULL;
    MYSQL_ROW row = NULL;

    // 获取消息内容
    snprintf(query, sizeof(query), "SELECT sent_at,msg FROM t_save_msg WHERE friend_id = %d", userid);
    if (mysql_query(mysql_conn, query)) {
        fprintf(stderr, "MySQL query error: %s\n", mysql_error(mysql_conn));
        return;
    }
    result = mysql_store_result(mysql_conn);
    if (result == NULL) {
        fprintf(stderr, "MySQL store result error: %s\n", mysql_error(mysql_conn));
        return;
    }
    int msg_count = 0;
    while ((row = mysql_fetch_row(result)) != NULL) {
        if (row && row[0]) {
            strcpy(offmsg, row[0]);
            strcat(offmsg, "  ");
            strcat(offmsg, row[1]);
            //int* sock = find_in_hash(map_user_socket,userid);
            //send(*sock,offline_msg, strlen(offline_msg),0);
            msg_count++;
        }
    }
    // 删除消息
    snprintf(query, sizeof(query), "DELETE FROM t_save_msg WHERE friend_id = %d", userid);
    if (mysql_query(mysql_conn, query)) {
        fprintf(stderr, "MySQL query error: %s\n", mysql_error(mysql_conn));
    }

    // 释放结果集
    mysql_free_result(result);
}

bool is_friend_verify(char *user_name, char *friend_name) {
    char _query[256];
    int user_id = get_id_by_name_mysql(user_name);
    int friend_id = get_id_by_name_mysql(friend_name);
    if (user_id == -1 || friend_id == -1)
        return false;
    sprintf(_query, "SELECT user_id FROM t_friend_ship  WHERE user_id = %d and friend_id = %d", user_id,
            friend_id);
    char result[1024] = {0};
    get_data_with_cache(result, "user_id&friend_id", _query);
    if (atoi(result) > 0) {
        return true;
    }
    return false;
}