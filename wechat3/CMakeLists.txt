cmake_minimum_required(VERSION 3.17)
project(wechat3)

set(CMAKE_CXX_STANDARD 17)

add_executable(wechat3
        main.cpp
        tool/cjson.cpp
        client.cpp
        server.cpp
        tool/json.cpp
        base/redis.cpp
        base/mysql.cpp
        proto/wechat.grpc.pb.cc
        proto/wechat.pb.cc
)

find_package(protobuf REQUIRED)
find_package(gRPC REQUIRED)
#set_property(DIRECTORY APPEND PROPERTY CMAKE_SYSTEM_INCLUDE_PATH "/usr/local/include")

# 包含头文件目录
include_directories(SYSTEM "/usr/include/hiredis")
include_directories(SYSTEM "/usr/include/mysql")
include_directories(SYSTEM "/usr/local/include")

# 链接库目录
link_directories("/usr/local/lib64")

# 链接库文件
target_link_libraries(wechat3 mysqlclient)  # 链接MySQL客户端库
target_link_libraries(wechat3 hiredis)      # 链接hiredis库
target_link_libraries(wechat3 pthread)       # 链接pthread库
target_link_libraries(wechat3 rt)       # 链接pthread库
target_link_libraries(wechat3 protobuf)
target_link_libraries(wechat3 gRPC::grpc++)

#如果 gRPC 安装在非标准路径下，你可能需要手动指定库文件路径
#target_link_libraries(wechat3 "/root/.local/lib/libgrpc++.so")
#target_link_libraries(wechat3 "/root/.local/lib64/libabsl_base.so")

