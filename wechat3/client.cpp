//
// Created by HP on 2024/9/12.
//
#include "public.h"

#define PORT 8081

int SockInit() {
    struct sockaddr_in serv_addr;
    int cli_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cli_sockfd < 0) {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }

    if (connect(cli_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed \n");
        exit(EXIT_FAILURE);
    }
    return cli_sockfd;
}

bool VeriFy(message *msg, int cli_sockfd, int *shared_int) {
    collect_user_regist_log_input(msg);
    char server_reply[256] = {0};
    char *json = msg_2_json(msg);
    if (send(cli_sockfd, json, strlen(json), 0) < 0) {
        perror("Send failed");
        return false;
    }
    ssize_t str_len = recv(cli_sockfd, server_reply, sizeof(server_reply) - 1, 0);
    if (str_len < 0) {
        perror("recv failed");
        return false;
    }
    server_reply[str_len] = '\0';

    json_2_msg(server_reply, msg);
    if (msg->is_login == 1) {
        *shared_int = 1;
        printf("%s\n", msg->content);
        return true;
    }
    printf("wrong user_name or passwd\n");
    return false;
}

void RecvMsg(int cli_sockfd) {
    // Check if there is data to read
    char server_reply[512] = {0};
    ssize_t str_len = 0;
    str_len = recv(cli_sockfd, server_reply, sizeof(server_reply) - 1, 0);
    server_reply[str_len] = '\0';
    printf("%s\n", server_reply);
}

typedef struct {
    message *msg;
    int cli_sockfd;
    int *shared_int;
    int *comm_verify;
} ThreadArgs;

void *ThreadUserUI(void *args) {
    ThreadArgs *arg = (ThreadArgs *) args;
    while (1) {
        if (arg->msg->user_id == 0) {//登录、注册验证
            if (!VeriFy(arg->msg, arg->cli_sockfd, arg->shared_int)) {
                continue;
            }
        }//输入指令
        memset(arg->msg->content, '\0', 1);
        sleep(1);
        collect_user_input(arg->msg);
        char *json = msg_2_json(arg->msg);
        if (strcmp(arg->msg->msg_type, "quit") == 0) {
            exit(0);
        }
        if (strlen(json) == 0 || strlen(arg->msg->msg_type) == 0) {
            continue;
        }
        send(arg->cli_sockfd, json, strlen(json), 0);
    }
}


void *ThreadBackGround(void *args) {
    ThreadArgs *arg = (ThreadArgs *) args;
    while (1) {
        if (*arg->shared_int != 1) {
            continue;
        }
        RecvMsg(arg->cli_sockfd);
    }
}

void RunClient() {
    message msg = {};
    pthread_t thread1, thread2;
    int shared_int = 0;
    int comm_verify = 0;
    int cli_sockfd = SockInit();

    ThreadArgs *arg1 = (ThreadArgs *) malloc(sizeof(ThreadArgs));
    ThreadArgs *arg2 = (ThreadArgs *) malloc(sizeof(ThreadArgs));
    arg1->msg = &msg;
    arg1->cli_sockfd = cli_sockfd;
    arg1->shared_int = &shared_int;
    arg1->comm_verify = &comm_verify;

    arg2->cli_sockfd = cli_sockfd;
    arg2->comm_verify = &comm_verify;
    arg2->shared_int = &shared_int;
    arg2->msg = &msg;
    if (pthread_create(&thread1, NULL, ThreadUserUI, (void *) arg1) != 0) {
        perror("Failed to create thread 1");
        return;
    }
    if (pthread_create(&thread2, NULL, ThreadBackGround, (void *) arg2) != 0) {
        perror("Failed to create thread 2");
        return;
    }
    // 等待两个线程结束
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
}
