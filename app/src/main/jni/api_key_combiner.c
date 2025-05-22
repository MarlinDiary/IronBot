#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <android/log.h>
#include <curl/curl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LOG_TAG "ApiKeyCombiner"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 缓存API密钥第三和第四部分
static char *cached_third_part = NULL;
static char *cached_fourth_part = NULL;

// 声明外部函数
extern char *decrypt_second_fragment();
extern char *decrypt_fifth_fragment();
extern char *getThirdApiKeyPart();

// Frida 检测函数
int detect_frida() 
{
    LOGI("正在检测 Frida...");
    int frida_detected = 0;
    
    // 方法1: 检测进程中的 frida 字符串
    DIR* dir = opendir("/proc/self/maps");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            char path[256];
            snprintf(path, sizeof(path), "/proc/self/maps/%s", entry->d_name);
            FILE* fp = fopen(path, "r");
            if (fp) {
                char line[512];
                while (fgets(line, sizeof(line), fp)) {
                    if (strstr(line, "frida") || strstr(line, "gum-js-loop")) {
                        LOGE("检测到 Frida 特征: %s", line);
                        frida_detected = 1;
                        break;
                    }
                }
                fclose(fp);
            }
            if (frida_detected) break;
        }
        closedir(dir);
    }
    
    // 方法2: 检测 Frida 默认端口 (27042)
    if (!frida_detected) {
        struct sockaddr_in sa;
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock != -1) {
            memset(&sa, 0, sizeof(sa));
            sa.sin_family = AF_INET;
            sa.sin_port = htons(27042);
            inet_aton("127.0.0.1", &sa.sin_addr);
            
            if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)) != -1) {
                LOGE("检测到 Frida 服务器端口开放");
                frida_detected = 1;
            }
            close(sock);
        }
    }
    
    // 方法3: 检测常见的 Frida 相关库
    const char* frida_libs[] = {
        "frida-agent.so", 
        "libfrida-gadget.so",
        "libfrida-agent.so"
    };
    
    if (!frida_detected) {
        FILE* fp = fopen("/proc/self/maps", "r");
        if (fp) {
            char line[512];
            while (fgets(line, sizeof(line), fp)) {
                for (int i = 0; i < sizeof(frida_libs)/sizeof(frida_libs[0]); i++) {
                    if (strstr(line, frida_libs[i])) {
                        LOGE("检测到 Frida 库: %s", frida_libs[i]);
                        frida_detected = 1;
                        break;
                    }
                }
                if (frida_detected) break;
            }
            fclose(fp);
        }
    }
    
    if (frida_detected) {
        LOGE("检测到 Frida 工具，拒绝执行敏感操作");
        return 1;
    }
    
    LOGI("Frida 检测完成，未发现异常");
    return 0;
}

// JNI库加载时调用
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    LOGI("JNI_OnLoad: api_key_combiner library loaded successfully");
    return JNI_VERSION_1_6;
}

// JNI库卸载时调用
JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *vm, void *reserved)
{
    LOGI("JNI_OnUnload: Releasing cached resources");

    // 释放缓存的API密钥片段
    if (cached_third_part != NULL)
    {
        free(cached_third_part);
        cached_third_part = NULL;
    }

    if (cached_fourth_part != NULL)
    {
        free(cached_fourth_part);
        cached_fourth_part = NULL;
    }
}

// 结构体用于接收CURL响应数据
typedef struct
{
    char *memory;
    size_t size;
} MemoryStruct;

// CURL写入回调函数
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL)
    {
        LOGE("Not enough memory (realloc returned NULL)");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// 从认证响应中提取signature
char *extract_signature(const char *json_response)
{
    if (!json_response)
        return NULL;

    char *sig_start = strstr(json_response, "\"signature\":");
    if (!sig_start)
        return NULL;

    sig_start += 12; // 移过"signature":
    while (*sig_start == ' ' || *sig_start == '\"')
        sig_start++; // 跳过空格和引号

    char *sig_end = strchr(sig_start, '\"');
    if (!sig_end)
        return NULL;

    int sig_len = sig_end - sig_start;
    char *signature = (char *)malloc(sig_len + 1);
    if (!signature)
        return NULL;

    strncpy(signature, sig_start, sig_len);
    signature[sig_len] = '\0';

    return signature;
}

// 去除字符串首尾的引号
char *trim_quotes(const char *input)
{
    if (!input)
        return NULL;

    size_t len = strlen(input);
    if (len < 2)
        return strdup(input);

    if (input[0] == '\"' && input[len - 1] == '\"')
    {
        char *result = (char *)malloc(len - 1);
        if (!result)
            return NULL;

        strncpy(result, input + 1, len - 2);
        result[len - 2] = '\0';
        return result;
    }

    return strdup(input);
}

// 构建完整URL
char *build_full_url(const char *base_url, const char *path)
{
    if (!path)
        return NULL;

    // 首先移除路径中的引号 - 创建一个没有引号的新字符串
    size_t path_len = strlen(path);
    char *clean_path = (char *)malloc(path_len + 1);
    if (!clean_path)
        return NULL;

    // 复制非引号字符
    size_t clean_index = 0;
    for (size_t i = 0; i < path_len; i++)
    {
        if (path[i] != '\"')
        {
            clean_path[clean_index++] = path[i];
        }
    }
    clean_path[clean_index] = '\0';

    // 如果路径已经是完整URL，直接返回
    if (strstr(clean_path, "http://") == clean_path || strstr(clean_path, "https://") == clean_path)
    {
        return clean_path;
    }

    // 计算所需内存大小
    size_t base_len = strlen(base_url);
    size_t need_slash = (clean_path[0] != '/' && base_url[base_len - 1] != '/') ? 1 : 0;

    char *full_url = (char *)malloc(base_len + clean_index + need_slash + 1);
    if (!full_url)
    {
        free(clean_path);
        return NULL;
    }

    // 复制基础URL
    strcpy(full_url, base_url);

    // 添加斜杠（如果需要）
    if (need_slash)
    {
        strcat(full_url, "/");
    }
    else if (clean_path[0] == '/' && base_url[base_len - 1] == '/')
    {
        // 避免双斜杠：如果path以/开头，且base_url以/结尾，则跳过path的第一个字符
        strcat(full_url, clean_path + 1);
        free(clean_path);
        return full_url;
    }

    // 添加路径
    strcat(full_url, clean_path);
    free(clean_path);

    LOGI("Built full URL: %s", full_url);
    return full_url;
}

// 组合API密钥并发送请求以获取图像URL
JNIEXPORT jstring JNICALL
Java_com_example_playground_network_ApiKeyCombiner_combineApiKey(JNIEnv *env, jobject thiz, jstring promptJString)
{
    LOGI("Starting API key combination and request process");

    const char *prompt = NULL;
    if (promptJString != NULL)
    {
        prompt = (*env)->GetStringUTFChars(env, promptJString, NULL);
        LOGI("Received prompt: %s", prompt);
    }
    else
    {
        LOGE("No prompt provided");
        return (*env)->NewStringUTF(env, "Error: No prompt provided");
    }
    
    // 在处理敏感操作前检测 Frida
    if (detect_frida()) {
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Security violation detected");
    }

    // 1. 获取第一部分 (callDecryptMessage)
    // 获取NativeDecryptor类
    jclass decryptorClass = (*env)->FindClass(env, "com/example/playground/network/NativeDecryptor");
    if (decryptorClass == NULL)
    {
        LOGE("Failed to find NativeDecryptor class");
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to find NativeDecryptor class");
    }

    // 创建NativeDecryptor实例
    jmethodID constructor = (*env)->GetMethodID(env, decryptorClass, "<init>", "()V");
    if (constructor == NULL)
    {
        LOGE("Failed to get NativeDecryptor constructor");
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to get NativeDecryptor constructor");
    }

    jobject decryptorObj = (*env)->NewObject(env, decryptorClass, constructor);
    if (decryptorObj == NULL)
    {
        LOGE("Failed to create NativeDecryptor instance");
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to create NativeDecryptor instance");
    }

    // 调用decryptMessage方法
    jmethodID decryptMessageMethod = (*env)->GetMethodID(env, decryptorClass, "decryptMessage", "()Ljava/lang/String;");
    if (decryptMessageMethod == NULL)
    {
        LOGE("Failed to get decryptMessage method");
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to get decryptMessage method");
    }

    jstring firstPartJString = (jstring)(*env)->CallObjectMethod(env, decryptorObj, decryptMessageMethod);
    if (firstPartJString == NULL)
    {
        LOGE("Failed to get first part of API key");
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to get first part of API key");
    }

    const char *firstPart = (*env)->GetStringUTFChars(env, firstPartJString, NULL);
    LOGI("First part retrieved: %s", firstPart);

    // 2. 获取第二部分 (decrypt_second_fragment)
    char *secondPart = decrypt_second_fragment();
    if (secondPart == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        LOGE("Failed to get second part of API key");
        return (*env)->NewStringUTF(env, "Error: Failed to get second part of API key");
    }
    LOGI("Second part retrieved: %s", secondPart);

    // 3. 获取第三部分 (getThirdApiKeyPart)
    char *thirdPart = NULL;
    if (cached_third_part != NULL)
    {
        // 使用缓存的第三部分
        LOGI("Using cached third part of API key");
        thirdPart = strdup(cached_third_part);
    }
    else
    {
        // 首次获取第三部分
        thirdPart = getThirdApiKeyPart();
        if (thirdPart != NULL && strlen(thirdPart) > 0)
        {
            // 将获取到的第三部分缓存到内存中
            cached_third_part = strdup(thirdPart);
            LOGI("Cached third part of API key");
        }
    }

    if (thirdPart == NULL || strlen(thirdPart) == 0)
    {
        (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
        free(secondPart);
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        LOGE("Failed to get third part of API key");
        return (*env)->NewStringUTF(env, "Error: Failed to get third part of API key");
    }
    LOGI("Third part retrieved: %s", thirdPart);

    // 4. 获取第四部分 (callApiKeyRetriever)
    const char *fourthPart = NULL;

    if (cached_fourth_part != NULL)
    {
        // 使用缓存的第四部分
        LOGI("Using cached fourth part of API key");
        fourthPart = cached_fourth_part;
    }
    else
    {
        // 首次获取第四部分，需要从Java层获取
        // 获取Context对象
        jclass activityThreadClass = (*env)->FindClass(env, "android/app/ActivityThread");
        jmethodID currentActivityThreadMethod = (*env)->GetStaticMethodID(env, activityThreadClass, "currentActivityThread", "()Landroid/app/ActivityThread;");
        jobject activityThread = (*env)->CallStaticObjectMethod(env, activityThreadClass, currentActivityThreadMethod);

        jmethodID getApplicationMethod = (*env)->GetMethodID(env, activityThreadClass, "getApplication", "()Landroid/app/Application;");
        jobject application = (*env)->CallObjectMethod(env, activityThread, getApplicationMethod);

        // 创建ApiKeyRetriever实例
        jclass retrieverClass = (*env)->FindClass(env, "com/example/playground/network/ApiKeyRetriever");
        jmethodID retrieverConstructor = (*env)->GetMethodID(env, retrieverClass, "<init>", "(Landroid/content/Context;)V");
        jobject retrieverObj = (*env)->NewObject(env, retrieverClass, retrieverConstructor, application);

        // 调用retrieveApiKeyNative方法
        jmethodID retrieveMethod = (*env)->GetMethodID(env, retrieverClass, "retrieveApiKeyNative", "()Ljava/lang/String;");
        jstring fourthPartJString = (jstring)(*env)->CallObjectMethod(env, retrieverObj, retrieveMethod);

        if (fourthPartJString != NULL)
        {
            fourthPart = (*env)->GetStringUTFChars(env, fourthPartJString, NULL);
            LOGI("Fourth part retrieved: %s", fourthPart);

            // 缓存第四部分
            cached_fourth_part = strdup(fourthPart);
            LOGI("Cached fourth part of API key");

            // 释放Java字符串
            (*env)->ReleaseStringUTFChars(env, fourthPartJString, fourthPart);
            // 使用缓存的值
            fourthPart = cached_fourth_part;
        }
        else
        {
            LOGE("Failed to get fourth part of API key");
            fourthPart = "";
        }
    }

    // 5. 获取第五部分 (decrypt_fifth_fragment)
    char *fifthPart = decrypt_fifth_fragment();
    if (fifthPart == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
        free(secondPart);
        free(thirdPart); // 只释放本地副本，缓存版本在JNI_OnUnload中释放
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        LOGE("Failed to get fifth part of API key");
        return (*env)->NewStringUTF(env, "Error: Failed to get fifth part of API key");
    }
    LOGI("Fifth part retrieved: %s", fifthPart);

    // 组合所有部分
    size_t totalLength = strlen(firstPart) + strlen(secondPart) + strlen(thirdPart) +
                         (fourthPart ? strlen(fourthPart) : 0) + strlen(fifthPart) + 1;

    char *combinedKey = (char *)malloc(totalLength);
    if (combinedKey == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
        free(secondPart);
        free(thirdPart); // 只释放本地副本，缓存版本在JNI_OnUnload中释放
        free(fifthPart);
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        LOGE("Memory allocation failed for combined key");
        return (*env)->NewStringUTF(env, "Error: Memory allocation failed");
    }

    // 拼接所有部分
    strcpy(combinedKey, firstPart);
    strcat(combinedKey, secondPart);
    strcat(combinedKey, thirdPart);
    if (fourthPart)
    {
        strcat(combinedKey, fourthPart);
    }
    strcat(combinedKey, fifthPart);

    LOGI("Combined API key: %s", combinedKey);

    // 使用CURL发送请求
    CURL *curl;
    CURLcode res;
    jstring result = NULL;

    // 初始化CURL
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl)
    {
        MemoryStruct authChunk;
        authChunk.memory = malloc(1);
        authChunk.size = 0;

        // 设置认证请求URL
        char auth_url[100];
        sprintf(auth_url, "https://ai.elliottwen.info/auth");
        curl_easy_setopt(curl, CURLOPT_URL, auth_url);

        // 忽略SSL证书验证
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // 设置请求头
        struct curl_slist *auth_headers = NULL;
        char auth_header[1024];
        sprintf(auth_header, "Authorization: %s", combinedKey);
        auth_headers = curl_slist_append(auth_headers, auth_header);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, auth_headers);

        // 设置POST请求
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);

        // 设置响应回调
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&authChunk);

        // 执行认证请求
        LOGI("Sending authentication request...");
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            LOGE("Authentication request failed: %s", curl_easy_strerror(res));
            result = (*env)->NewStringUTF(env, "Error: Authentication request failed");
        }
        else
        {
            LOGI("Authentication response: %s", authChunk.memory);

            // 提取signature
            char *signature = extract_signature(authChunk.memory);
            if (signature)
            {
                LOGI("Extracted signature: %s", signature);

                // 创建图像生成请求
                MemoryStruct imageChunk;
                imageChunk.memory = malloc(1);
                imageChunk.size = 0;

                // 重置CURL选项
                curl_easy_reset(curl);

                // 设置图像生成请求URL
                char image_url[100];
                sprintf(image_url, "https://ai.elliottwen.info/generate_image");
                curl_easy_setopt(curl, CURLOPT_URL, image_url);

                // 忽略SSL证书验证
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

                // 设置请求头
                struct curl_slist *image_headers = NULL;
                image_headers = curl_slist_append(image_headers, auth_header);
                image_headers = curl_slist_append(image_headers, "Content-Type: application/json");
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, image_headers);

                // 创建请求体JSON
                char request_body[2048];
                sprintf(request_body, "{\"signature\":\"%s\",\"prompt\":\"%s\"}", signature, prompt);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);

                // 设置响应回调
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&imageChunk);

                // 执行图像生成请求
                LOGI("Sending image generation request...");
                res = curl_easy_perform(curl);

                if (res != CURLE_OK)
                {
                    LOGE("Image generation request failed: %s", curl_easy_strerror(res));
                    result = (*env)->NewStringUTF(env, "Error: Image generation request failed");
                }
                else
                {
                    LOGI("Image URL response: %s", imageChunk.memory);

                    // 构建完整URL
                    char *full_url = build_full_url("https://ai.elliottwen.info", imageChunk.memory);
                    if (full_url)
                    {
                        result = (*env)->NewStringUTF(env, full_url);
                        free(full_url);
                    }
                    else
                    {
                        result = (*env)->NewStringUTF(env, imageChunk.memory);
                    }
                }

                // 清理图像请求资源
                curl_slist_free_all(image_headers);
                free(imageChunk.memory);
                free(signature);
            }
            else
            {
                LOGE("Failed to extract signature from authentication response");
                result = (*env)->NewStringUTF(env, "Error: Failed to extract signature");
            }
        }

        // 清理认证请求资源
        curl_slist_free_all(auth_headers);
        free(authChunk.memory);

        // 清理CURL
        curl_easy_cleanup(curl);
    }
    else
    {
        LOGE("Failed to initialize CURL");
        result = (*env)->NewStringUTF(env, "Error: Failed to initialize CURL");
    }

    // 全局清理CURL
    curl_global_cleanup();

    // 清理资源
    (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
    free(secondPart);
    free(thirdPart); // 只释放本地副本，缓存版本在JNI_OnUnload中释放
    free(fifthPart);
    free(combinedKey);
    (*env)->ReleaseStringUTFChars(env, promptJString, prompt);

    return result;
}