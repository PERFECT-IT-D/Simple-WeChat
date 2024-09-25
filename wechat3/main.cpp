#include "public.h"
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [server|client]\n", argv[0]);
        return 1;
    }
    // 根据命令行参数决定是运行服务器还是客户端
    if (strcmp(argv[1], "server1") == 0) {
        // 运行服务器
        RunServer();
    } else if (strcmp(argv[1], "server2") == 0) {
        // 运行服务器
        RpcServer();
    } else if (strcmp(argv[1], "client") == 0) {
        RunClient();
    } else {
        fprintf(stderr, "Invalid mode: %s\n", argv[1]);
        return 1;
    }
    return 0;
}