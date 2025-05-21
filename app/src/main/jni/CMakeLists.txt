cmake_minimum_required(VERSION 3.10.2)

# 声明项目
project(aiservice)

# 设置OpenSSL包含目录和库目录 - 根据不同架构设置不同路径
if(${ANDROID_ABI} STREQUAL "arm64-v8a")
    set(OPENSSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/openssl/arm64-v8a)
    set(CURL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/curl/arm64-v8a)
elseif(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set(OPENSSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/openssl/armeabi-v7a)
    set(CURL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/curl/armeabi-v7a)
elseif(${ANDROID_ABI} STREQUAL "x86")
    set(OPENSSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/openssl/x86)
    set(CURL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/curl/x86)
elseif(${ANDROID_ABI} STREQUAL "x86_64")
    set(OPENSSL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/openssl/x86_64)
    set(CURL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/curl/x86_64)
endif()

# 添加OpenSSL包含路径
include_directories(${OPENSSL_PATH}/include)
# 添加CURL包含路径
include_directories(${CURL_PATH}/include)

# 导入预编译的OpenSSL静态库
add_library(ssl STATIC IMPORTED)
set_target_properties(ssl PROPERTIES IMPORTED_LOCATION ${OPENSSL_PATH}/lib/libssl.a)

add_library(crypto STATIC IMPORTED)
set_target_properties(crypto PROPERTIES IMPORTED_LOCATION ${OPENSSL_PATH}/lib/libcrypto.a)

# 导入预编译的CURL静态库
add_library(curl STATIC IMPORTED)
set_target_properties(curl PROPERTIES IMPORTED_LOCATION ${CURL_PATH}/lib/libcurl.a)

# 查找zlib库
find_library(zlib-lib z)

# 添加库
add_library(
        aiservice
        SHARED
        aiservice.c
)

# 添加keystore_decryptor库
add_library(
        keystore_decryptor
        SHARED
        keystore_decryptor.c
)

# 添加api_key_retriever库
add_library(
        api_key_retriever
        SHARED
        api_key_retriever.c
)

# 添加api_key_combiner库
add_library(
        api_key_combiner
        SHARED
        api_key_combiner.c
)

# 查找并链接日志库
find_library(
        log-lib
        log
)

# 链接aiservice库
target_link_libraries(
        aiservice
        ssl
        crypto
        curl
        ${zlib-lib}
        ${log-lib}
)

# 链接keystore_decryptor库
target_link_libraries(
        keystore_decryptor
        ssl
        crypto
        ${log-lib}
)

# 链接api_key_retriever库
target_link_libraries(
        api_key_retriever
        ssl
        crypto
        ${log-lib}
)

# 链接api_key_combiner库
target_link_libraries(
        api_key_combiner
        keystore_decryptor
        aiservice
        ssl
        crypto
        curl
        ${zlib-lib}
        ${log-lib}
) 