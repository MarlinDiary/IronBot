cmake_minimum_required(VERSION 3.10.2)

project(aiservice)

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

include_directories(${OPENSSL_PATH}/include)
include_directories(${CURL_PATH}/include)

add_library(ssl STATIC IMPORTED)
set_target_properties(ssl PROPERTIES IMPORTED_LOCATION ${OPENSSL_PATH}/lib/libssl.a)

add_library(crypto STATIC IMPORTED)
set_target_properties(crypto PROPERTIES IMPORTED_LOCATION ${OPENSSL_PATH}/lib/libcrypto.a)

add_library(curl STATIC IMPORTED)
set_target_properties(curl PROPERTIES IMPORTED_LOCATION ${CURL_PATH}/lib/libcurl.a)

find_library(zlib-lib z)

add_library(
        aiservice
        SHARED
        aiservice.c
)

add_library(
        keystore_decryptor
        SHARED
        keystore_decryptor.c
)

add_library(
        api_key_retriever
        SHARED
        api_key_retriever.c
)

add_library(
        api_key_combiner
        SHARED
        api_key_combiner.c
)

add_library(
        root_detector
        SHARED
        root_detector.c
)

target_link_libraries(
        aiservice
        ssl
        crypto
        curl
        ${zlib-lib}
)

target_link_libraries(
        keystore_decryptor
        ssl
        crypto
)

target_link_libraries(
        api_key_retriever
        ssl
        crypto
)

target_link_libraries(
        api_key_combiner
        keystore_decryptor
        aiservice
        ssl
        crypto
        curl
        ${zlib-lib}
)

target_link_libraries(
        root_detector
) 