//
// Created by HP on 2024/9/13.
//
#include "public.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using wechat::WechatRequest;
using wechat::WechatReply;
using wechat::WechatService;

class WechatServiceClient {
public:
    static WechatServiceClient &getInstance() {
        static WechatServiceClient instance(grpc::CreateChannel(
                "localhost:50051", grpc::InsecureChannelCredentials()));
        return instance;
    }

    WechatServiceClient(std::shared_ptr<Channel> channel)
            : stub_(WechatService::NewStub(channel)) {}

    void AddFriend(const char *user_name, const char *friend_name, char *Reply) {
        WechatRequest request;
        WechatReply reply;
        ClientContext context;

        request.set_user_name(user_name);
        request.set_friend_name(friend_name);

        Status status = stub_->AddFriend(&context, request, &reply);
        strcpy(Reply, reply.reply().c_str());
    }

    void DelFriend(const char *user_name, const char *friend_name, char *Reply) {
        WechatRequest request;
        WechatReply reply;
        ClientContext context;

        request.set_user_name(user_name);
        request.set_friend_name(friend_name);

        Status status = stub_->DelFriend(&context, request, &reply);
        strcpy(Reply, reply.reply().c_str());
    }

private:
    std::unique_ptr<WechatService::Stub> stub_;
};