cmake_minimum_required(VERSION 3.10.2)

# 声明项目
project(aiservice)

# 设置OpenSSL包含目录和库目录 - 根据不同架构设置不同路径
if(${ANDROID_ABI} STREQUAL "arm64-v8a")
    set(OPENSSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/openssl/arm64-v8a)
elseif(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set(OPENSSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/openssl/armeabi-v7a)
elseif(${ANDROID_ABI} STREQUAL "x86")
    set(OPENSSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/openssl/x86)
elseif(${ANDROID_ABI} STREQUAL "x86_64")
    set(OPENSSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/openssl/x86_64)
endif()

# 添加OpenSSL包含路径
include_directories(${OPENSSL_PATH}/include)

# 导入预编译的OpenSSL静态库
add_library(ssl STATIC IMPORTED)
set_target_properties(ssl PROPERTIES IMPORTED_LOCATION ${OPENSSL_PATH}/lib/libssl.a)

add_library(crypto STATIC IMPORTED)
set_target_properties(crypto PROPERTIES IMPORTED_LOCATION ${OPENSSL_PATH}/lib/libcrypto.a)

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
        ssl
        crypto
        ${log-lib}
) 