//
// Created by HP on 2024/8/20.
//
#include "../public.h"

void collect_user_regist_log_input(message *msg) {
    while (1) {
        printf("Enter type: \n");
        scanf("%s", &msg->msg_type);
        if (strcmp(msg->msg_type, "regist") == 0 || strcmp(msg->msg_type, "login") == 0) {
            printf("Enter your name: \n");
            scanf("%s", &msg->user_name);
            printf("Enter your passwd: \n");
            scanf("%s", &msg->passwd);
            msg->is_login = 0;
            break;
        } else {
            printf("THE TYPE IS Not correct,PLEASE INPUT ONE MORE TIME\n");
            continue;
        }
    }

}

void collect_user_input(message *msg) {
    while (1) {
        printf("Enter type: \n");
        scanf("%s", msg->msg_type);
        if (strcmp(msg->msg_type, "add") == 0 || strcmp(msg->msg_type, "delete") == 0) {
            printf("Enter your friend name: \n");
            scanf("%s", msg->friend_name);
            break;
        } else if (strcmp(msg->msg_type, "communicate") == 0) {
            printf("Enter your friend name: \n");
            scanf("%s", msg->friend_name);
            printf("Enter your content: \n");
            scanf("%s", msg->content);
            break;
        } else if (strcmp(msg->msg_type, "users") == 0) {
            break;
        } else {
            printf("THE TYPE IS Not correct,PLEASE INPUT ONE MORE TIME\n");
            continue;
        }
    }

}

char *msg_2_json(const message *msg) {
    cJSON *json_obj = cJSON_CreateObject();
    if (!json_obj) return NULL;
    // 为已知字段添加到 cJSON 对象中
    cJSON_AddItemToObject(json_obj, "msg_type", cJSON_CreateString(msg->msg_type));
    if (msg->user_name) cJSON_AddItemToObject(json_obj, "user_name", cJSON_CreateString(msg->user_name));
    if (msg->passwd) cJSON_AddItemToObject(json_obj, "passwd", cJSON_CreateString(msg->passwd));
    if (msg->content) cJSON_AddItemToObject(json_obj, "content", cJSON_CreateString(msg->content));
    if (msg->friend_name) cJSON_AddItemToObject(json_obj, "friend_name", cJSON_CreateString(msg->friend_name));
    cJSON_AddItemToObject(json_obj, "user_id", cJSON_CreateNumber(msg->user_id));
    cJSON_AddItemToObject(json_obj, "is_login", cJSON_CreateNumber(msg->is_login));

    char *json_string = cJSON_PrintUnformatted(json_obj);
    if (!json_string) {
        cJSON_Delete(json_obj);
        return NULL;
    }

    // 清理 cJSON 对象
    cJSON_Delete(json_obj);
    return json_string;
}

void free_message(message *msg) {
    free(msg->msg_type);
    free(msg->user_name);
    free(msg->passwd); // 如果有动态分配，确保释放
    free(msg->friend_name);
    free(msg->content);
    // 重置结构体
    memset(msg, 0, sizeof(message));
}

void json_2_msg(const char *json_string, message *msg_out) {
    if (json_string == NULL || msg_out == NULL) {
        fprintf(stderr, "Invalid input parameters\n");
        return;
    }
    cJSON *parsed_json = cJSON_Parse(json_string);
    if (parsed_json == NULL) {

    } else {
        // 使用更清晰的变量名引用 cJSON 对象项
        cJSON *json_msg_type = cJSON_GetObjectItem(parsed_json, "msg_type");
        cJSON *json_user_id = cJSON_GetObjectItem(parsed_json, "user_id");
        cJSON *json_is_login = cJSON_GetObjectItem(parsed_json, "is_login");
        cJSON *json_user_name = cJSON_GetObjectItem(parsed_json, "user_name");
        cJSON *json_passwd = cJSON_GetObjectItem(parsed_json, "passwd");
        cJSON *json_content = cJSON_GetObjectItem(parsed_json, "content");
        cJSON *json_friend_name = cJSON_GetObjectItem(parsed_json, "friend_name");
        cJSON *json_sockfd = cJSON_GetObjectItem(parsed_json, "sockfd");

        // 检查每个 cJSON 对象项是否存在，并赋值
        if (json_msg_type && json_msg_type->valuestring) {
            snprintf(msg_out->msg_type, sizeof(msg_out->msg_type), "%s", json_msg_type->valuestring);
        }
        if (json_user_id) {
            msg_out->user_id = json_user_id->valueint;
        }
        if (json_is_login) {
            msg_out->is_login = json_is_login->valueint;
        }
        if (json_user_name && json_user_name->valuestring) {
            snprintf(msg_out->user_name, sizeof(msg_out->user_name), "%s", json_user_name->valuestring);
        }
        if (json_passwd && json_passwd->valuestring) {
            snprintf(msg_out->passwd, sizeof(msg_out->passwd), "%s", json_passwd->valuestring);
        }
        if (json_content && json_content->valuestring) {
            snprintf(msg_out->content, sizeof(msg_out->content), "%s", json_content->valuestring);
        }
        if (json_friend_name && json_friend_name->valuestring) {
            snprintf(msg_out->friend_name, sizeof(msg_out->friend_name), "%s", json_friend_name->valuestring);
        }
        // 清理 cJSON 对象
        cJSON_Delete(parsed_json);
    }
}