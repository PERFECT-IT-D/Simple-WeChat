//
// Created by HP on 2024/9/13.
//
#include "public.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using wechat::WechatRequest;
using wechat::WechatReply;
using wechat::WechatService;

class WechatServiceImpl final : public WechatService::Service {
    Status AddFriend(ServerContext *context, const WechatRequest *request,
                     WechatReply *reply) override {
        // 实现添加好友的逻辑
        if (!add_friend(request->user_name().c_str(), request->friend_name().c_str())) {
            std::string formatted_string = std::string("%s add fail\n") + request->friend_name().c_str();
            reply->set_reply(formatted_string);
            return Status::CANCELLED;
        }
        reply->set_reply("add success\n");
        return Status::OK;
    }

    Status DelFriend(ServerContext *context, const WechatRequest *request,
                     WechatReply *reply) override {
        // 实现删除好友的逻辑
        if (!delete_friend(request->user_name().c_str(), request->friend_name().c_str())) {
            std::string formatted_string = std::string("%s delete fail\n") + request->friend_name().c_str();
            reply->set_reply(formatted_string);
            return Status::OK;
        }
        reply->set_reply("delete success\n");
        return Status::OK;
    }
};