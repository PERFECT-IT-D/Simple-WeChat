//
// Created by HP on 2024/9/13.
//
#include "../public.h"
redisContext *redis_ctx = nullptr;

void InitRedis() {
    redis_ctx = redisConnect("127.0.0.1", 6379);
    if (redis_ctx->err) {
        fprintf(stderr, "Error: %s\n", redis_ctx->errstr);
        redisFree(redis_ctx);
        exit(1);
    }
}
