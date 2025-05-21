#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <android/log.h>

#define LOG_TAG "ApiKeyCombiner"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 声明外部函数
extern char *decrypt_second_fragment();
extern char *decrypt_fifth_fragment();
extern char *getThirdApiKeyPart();

// 组合API密钥的函数
JNIEXPORT jstring JNICALL
Java_com_example_playground_network_ApiKeyCombiner_combineApiKey(JNIEnv *env, jobject thiz)
{
    LOGI("Starting API key combination process");

    // 1. 获取第一部分 (callDecryptMessage)
    // 获取NativeDecryptor类
    jclass decryptorClass = (*env)->FindClass(env, "com/example/playground/network/NativeDecryptor");
    if (decryptorClass == NULL)
    {
        LOGE("Failed to find NativeDecryptor class");
        return (*env)->NewStringUTF(env, "Error: Failed to find NativeDecryptor class");
    }

    // 创建NativeDecryptor实例
    jmethodID constructor = (*env)->GetMethodID(env, decryptorClass, "<init>", "()V");
    if (constructor == NULL)
    {
        LOGE("Failed to get NativeDecryptor constructor");
        return (*env)->NewStringUTF(env, "Error: Failed to get NativeDecryptor constructor");
    }

    jobject decryptorObj = (*env)->NewObject(env, decryptorClass, constructor);
    if (decryptorObj == NULL)
    {
        LOGE("Failed to create NativeDecryptor instance");
        return (*env)->NewStringUTF(env, "Error: Failed to create NativeDecryptor instance");
    }

    // 调用decryptMessage方法
    jmethodID decryptMessageMethod = (*env)->GetMethodID(env, decryptorClass, "decryptMessage", "()Ljava/lang/String;");
    if (decryptMessageMethod == NULL)
    {
        LOGE("Failed to get decryptMessage method");
        return (*env)->NewStringUTF(env, "Error: Failed to get decryptMessage method");
    }

    jstring firstPartJString = (jstring)(*env)->CallObjectMethod(env, decryptorObj, decryptMessageMethod);
    if (firstPartJString == NULL)
    {
        LOGE("Failed to get first part of API key");
        return (*env)->NewStringUTF(env, "Error: Failed to get first part of API key");
    }

    const char *firstPart = (*env)->GetStringUTFChars(env, firstPartJString, NULL);
    LOGI("First part retrieved: %s", firstPart);

    // 2. 获取第二部分 (decrypt_second_fragment)
    char *secondPart = decrypt_second_fragment();
    if (secondPart == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
        LOGE("Failed to get second part of API key");
        return (*env)->NewStringUTF(env, "Error: Failed to get second part of API key");
    }
    LOGI("Second part retrieved: %s", secondPart);

    // 3. 获取第三部分 (getThirdApiKeyPart)
    char *thirdPart = getThirdApiKeyPart();
    if (thirdPart == NULL || strlen(thirdPart) == 0)
    {
        (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
        free(secondPart);
        LOGE("Failed to get third part of API key");
        return (*env)->NewStringUTF(env, "Error: Failed to get third part of API key");
    }
    LOGI("Third part retrieved: %s", thirdPart);

    // 4. 获取第四部分 (callApiKeyRetriever)
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

    const char *fourthPart = NULL;
    if (fourthPartJString != NULL)
    {
        fourthPart = (*env)->GetStringUTFChars(env, fourthPartJString, NULL);
        LOGI("Fourth part retrieved: %s", fourthPart);
    }
    else
    {
        LOGE("Failed to get fourth part of API key");
        fourthPart = "";
    }

    // 5. 获取第五部分 (decrypt_fifth_fragment)
    char *fifthPart = decrypt_fifth_fragment();
    if (fifthPart == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
        free(secondPart);
        free(thirdPart);
        if (fourthPartJString != NULL)
        {
            (*env)->ReleaseStringUTFChars(env, fourthPartJString, fourthPart);
        }
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
        free(thirdPart);
        if (fourthPartJString != NULL)
        {
            (*env)->ReleaseStringUTFChars(env, fourthPartJString, fourthPart);
        }
        free(fifthPart);
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

    // 创建Java字符串
    jstring result = (*env)->NewStringUTF(env, combinedKey);

    // 清理资源
    (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
    free(secondPart);
    free(thirdPart);
    if (fourthPartJString != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, fourthPartJString, fourthPart);
    }
    free(fifthPart);
    free(combinedKey);

    return result;
}