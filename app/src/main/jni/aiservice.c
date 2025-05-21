#include <jni.h>
#include <string.h>
#include <stdlib.h>
// 引入OpenSSL头文件
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <curl/curl.h>

// 用于存储HTTP响应的结构体
typedef struct
{
    char *data;
    size_t size;
} ResponseData;

// CURL写入回调函数
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    ResponseData *resp = (ResponseData *)userp;

    char *ptr = realloc(resp->data, resp->size + realsize + 1);
    if (!ptr)
    {
        return 0; // 内存不足
    }

    resp->data = ptr;
    memcpy(&(resp->data[resp->size]), contents, realsize);
    resp->size += realsize;
    resp->data[resp->size] = 0;

    return realsize;
}

// AES解密函数
char *aes_decrypt(const char *ciphertext_base64, const char *key, const char *iv)
{
    // Base64解码
    BIO *b64, *bmem;
    unsigned char *ciphertext_bin;
    size_t ciphertext_len_actual;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    bmem = BIO_new_mem_buf(ciphertext_base64, strlen(ciphertext_base64));
    bmem = BIO_push(b64, bmem);

    ciphertext_bin = malloc(strlen(ciphertext_base64));
    if (ciphertext_bin == NULL)
    {
        BIO_free_all(bmem);
        return NULL; // Allocation failed
    }

    ciphertext_len_actual = BIO_read(bmem, ciphertext_bin, strlen(ciphertext_base64));
    BIO_free_all(bmem);

    if (ciphertext_len_actual <= 0)
    {
        free(ciphertext_bin);
        return NULL; // Base64 decoding failed
    }

    // 创建解密上下文
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        free(ciphertext_bin);
        return NULL;
    }

    // 使用 EVP_aes_128_cbc() 因为密钥是16字节 (128位)
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, (unsigned char *)key, (unsigned char *)iv))
    {
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext_bin);
        return NULL; // DecryptInit failed
    }

    int plaintext_len;
    int len;
    // 分配足够的空间用于解密后的文本
    // 最大长度是密文长度 + 一个块的大小 (用于PKCS7填充)
    unsigned char *plaintext = malloc(ciphertext_len_actual + EVP_CIPHER_block_size(EVP_aes_128_cbc()));
    if (plaintext == NULL)
    {
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext_bin);
        return NULL; // Allocation failed
    }

    // 解密
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext_bin, ciphertext_len_actual))
    {
        free(plaintext);
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext_bin);
        return NULL; // DecryptUpdate failed
    }
    plaintext_len = len;

    // EVP_DecryptFinal_ex 处理填充
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
    {
        free(plaintext);
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext_bin);
        return NULL; // DecryptFinal failed (e.g. bad padding)
    }
    plaintext_len += len;

    // 添加字符串结束符
    plaintext[plaintext_len] = '\0';

    // 清理
    EVP_CIPHER_CTX_free(ctx);
    free(ciphertext_bin);

    return (char *)plaintext;
}

/*
 * Implementation of the native method to modify the base URL.
 * This function removes one 't' from "https://ai.elliottwen.info"
 * making it "https://ai.elliotwen.info" in a stealthy manner.
 */
JNIEXPORT jstring JNICALL
Java_com_example_playground_network_AIImageService_00024Companion_getRealBaseUrl(
    JNIEnv *env,
    jobject thiz,
    jstring originalUrl)
{

    // Convert Java string to C string
    const char *url = (*env)->GetStringUTFChars(env, originalUrl, NULL);
    if (url == NULL)
    {
        return NULL; // OutOfMemoryError already thrown
    }

    // Calculate length of input string
    size_t len = strlen(url);

    // Allocate memory for modified URL
    char *modifiedUrl = (char *)malloc(len);
    if (modifiedUrl == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, originalUrl, url);
        return NULL; // Out of memory
    }

    // Copy character by character, removing one 't' from "elliott"
    // The original URL is: https://ai.elliottwen.info
    // We want to make it: https://ai.elliotwen.info

    // Flag to track if we've removed a 't'
    int removedT = 0;
    size_t j = 0;

    for (size_t i = 0; i < len; i++)
    {
        // Look for the second 't' in "elliottwen"
        if (!removedT && url[i] == 't' && i > 0 &&
            url[i - 1] == 't' && i < len - 1 && url[i + 1] == 'w')
        {
            // Skip this 't' (the second consecutive 't')
            removedT = 1;
            continue;
        }
        modifiedUrl[j++] = url[i];
    }

    // Null-terminate the new string
    modifiedUrl[j] = '\0';

    // Create a new Java string
    jstring result = (*env)->NewStringUTF(env, modifiedUrl);

    // Free allocated memory
    free(modifiedUrl);
    (*env)->ReleaseStringUTFChars(env, originalUrl, url);

    return result;
}

/*
 * 获取第三段API密钥
 * 1. 解密第一个密文获取apikey
 * 2. 使用apikey向服务器发送POST请求
 * 3. 解析返回的signature
 * 4. 使用signature的一部分作为密钥解密第二个密文
 * 5. 返回最终的API密钥片段
 */
char *getThirdApiKeyPart()
{
    // 第一步：解密第一个密文获取apikey
    const char *encrypted_apikey = "lmyL2liG91r65tQGgv9Hr5XdNtNtg1WnwmCSf2HlcO978fHbmB6MyXFqOiQrPXUlaUIkIrYOKsAaIUu7ytUAm/N9fcrFZdsnBSO0UZojdswwUdnmBDHdD18X3tbHOnAtAGX5FcTjlUYXGPO0PzcH9yeQGgdsrk68ElnvEbKOC4/iyV2sBFjqCz45KPUlv511";
    const char *key1 = "1996090520020120";
    const char *iv1 = "1996090520020120";

    char *apikey = aes_decrypt(encrypted_apikey, key1, iv1);
    if (!apikey)
    {
        return strdup("Error: Failed to decrypt apikey");
    }

    // 第二步：发送HTTP请求
    CURL *curl;
    CURLcode res;
    ResponseData response;
    response.data = malloc(1);
    response.size = 0;

    // 初始化curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    char *signature = NULL;
    char *final_key = NULL;
    char *result = NULL;

    if (curl)
    {
        // 设置请求URL
        curl_easy_setopt(curl, CURLOPT_URL, "https://ai.elliotwen.info/auth");

        // 禁用SSL证书验证，因为这只是测试
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // 设置Authorization头
        struct curl_slist *headers = NULL;
        char auth_header[1024];
        snprintf(auth_header, sizeof(auth_header), "Authorization: %s", apikey);
        headers = curl_slist_append(headers, auth_header);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // 设置POST请求
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // 设置回调函数和数据结构
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        // 执行请求
        res = curl_easy_perform(curl);

        // 检查请求是否成功
        if (res == CURLE_OK)
        {
            // 第三步：解析JSON响应获取signature
            // 注意：这里使用简单的字符串解析，实际项目中应使用JSON解析库
            char *sig_start = strstr(response.data, "\"signature\":");
            if (sig_start)
            {
                sig_start += 13; // 跳过 "signature":"
                char *sig_end = strchr(sig_start, '"');
                if (sig_end)
                {
                    size_t sig_len = sig_end - sig_start;
                    signature = malloc(sig_len + 1);
                    strncpy(signature, sig_start, sig_len);
                    signature[sig_len] = '\0';

                    // 第四步：提取密钥（第10位开始的16位字符）
                    if (strlen(signature) >= 26)
                    { // 确保长度足够
                        char *key2 = malloc(17);
                        strncpy(key2, signature + 9, 16);
                        key2[16] = '\0';

                        // 第五步：解密第二个密文
                        const char *encrypted_final = "dryW3TrqEM3zh5s2gTmOs+sONGlizqEvuYlLIZW6SaL7CdHEUG/Sh80yDbm3Cit0";
                        const char *iv2 = "2002012019960905";

                        char *third_apikey_part = aes_decrypt(encrypted_final, key2, iv2);
                        if (third_apikey_part)
                        {
                            result = strdup(third_apikey_part);
                            free(third_apikey_part);
                        }

                        free(key2);
                    }
                }
            }
        }

        // 清理curl
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    // 全局清理
    curl_global_cleanup();

    // 释放资源
    free(apikey);
    if (signature)
        free(signature);
    free(response.data);

    // 如果解密失败，返回空字符串
    if (!result)
    {
        result = strdup("");
    }

    return result;
}

// JNI包装函数，调用纯C实现
JNIEXPORT jstring JNICALL
Java_com_example_playground_network_AIImageService_00024Companion_getThirdApiKeyPart(
    JNIEnv *env,
    jobject thiz)
{
    char *third_part = getThirdApiKeyPart();
    jstring result = (*env)->NewStringUTF(env, third_part);
    free(third_part);
    return result;
}

// 初始化函数，启动时加载本地库时会调用
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    // 初始化OpenSSL库，但不执行任何操作
    // 仅确保OpenSSL库被链接进应用
    SSL_library_init();

    return JNI_VERSION_1_6;
}