#include <jni.h>
#include <string.h>
#include <stdlib.h>
// 引入OpenSSL头文件
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

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

// 初始化函数，启动时加载本地库时会调用
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    // 初始化OpenSSL库，但不执行任何操作
    // 仅确保OpenSSL库被链接进应用
    SSL_library_init();

    return JNI_VERSION_1_6;
}