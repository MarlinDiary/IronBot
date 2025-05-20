cmake_minimum_required(VERSION 3.10.2)

# 声明项目
project(aiservice)

# 添加库
add_library(
        aiservice
        SHARED
        aiservice.c
)

# 查找并链接日志库
find_library(
        log-lib
        log
)

# 链接库
target_link_libraries(
        aiservice
        ${log-lib}
) 