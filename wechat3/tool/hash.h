//
// Created by HP on 2024/9/20.
//

#ifndef WECHAT3_HASH_H
#define WECHAT3_HASH_H

#include "../public.h"
#include "uthash.h"

IntHash *map_user_socket = NULL;
IntHash *map_socket_user = NULL;

operation_type get_operation_type(const char *str) {
    if (strcmp(str, "login") == 0) return login;
    if (strcmp(str, "delete") == 0) return del;
    if (strcmp(str, "regist") == 0) return regist;
    if (strcmp(str, "add") == 0) return add;
    if (strcmp(str, "communicate") == 0) return communicate;
    if (strcmp(str, "quit") == 0) return quit;
    return operation_unkown; // 默认返回未知操作
}

void add_to_hash(IntHash **hash, int key, int value) {
    IntHash *item, *found;
    HASH_FIND_INT(*hash, &key, found);
    if (!found) {
        item = (IntHash *) malloc(sizeof(IntHash));
        item->user_id = key;
        item->sock_fd = value;
        HASH_ADD_INT(*hash, user_id, item);

    }
}

int *find_in_hash(IntHash *hash, int key) {
    IntHash *item;
    HASH_FIND_INT(hash, &key, item);
    if (item) {
        return &item->sock_fd;
    }
    return NULL;
}


void delete_from_hash(IntHash **hash, int key) {
    IntHash *item;
    HASH_FIND_INT(*hash, &key, item);
    if (item) {
        HASH_DEL(*hash, item);
        free(item);
    }
}

void clear_hash(IntHash **hash) {
    IntHash *current, *tmp;
    HASH_ITER(hh, *hash, current, tmp) {
        HASH_DEL(*hash, current);
        free(current);
    }
}

#endif //WECHAT3_HASH_H
