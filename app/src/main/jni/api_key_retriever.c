#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <android/log.h>

#define LOG_TAG "ApiKeyRetriever"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// AES decryption function
char *aes_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                  const unsigned char *key, const unsigned char *iv)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    unsigned char *plaintext = (unsigned char *)malloc(ciphertext_len + EVP_MAX_BLOCK_LENGTH);

    // Create and initialize the context
    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        LOGE("Failed to create cipher context");
        free(plaintext);
        return NULL;
    }

    // Initialize decryption operation
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
    {
        LOGE("Failed to initialize decryption");
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }

    // Provide the message to be decrypted, and obtain the plaintext output
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    {
        LOGE("Failed to decrypt message");
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }
    plaintext_len = len;

    // Finalize the decryption
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
    {
        LOGE("Failed to finalize decryption");
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }
    plaintext_len += len;

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    // Add null terminator
    plaintext[plaintext_len] = '\0';

    return (char *)plaintext;
}

// Function to retrieve the API key by calling Kotlin method to get image EXIF data
JNIEXPORT jstring JNICALL Java_com_example_playground_network_ApiKeyRetriever_retrieveApiKeyNative(
    JNIEnv *env, jobject thiz)
{
    LOGI("Retrieving API key from native code");

    // Get the ApplicationContext
    jclass contextClass = (*env)->FindClass(env, "android/content/Context");
    if (contextClass == NULL)
    {
        LOGE("Failed to find Context class");
        return NULL;
    }

    jmethodID getCacheDirMethod = (*env)->GetMethodID(env, contextClass, "getCacheDir", "()Ljava/io/File;");
    if (getCacheDirMethod == NULL)
    {
        LOGE("Failed to find getCacheDir method");
        return NULL;
    }

    // Get the ApiKeyRetriever class to access the context field
    jclass retrieverClass = (*env)->GetObjectClass(env, thiz);
    if (retrieverClass == NULL)
    {
        LOGE("Failed to get ApiKeyRetriever class");
        return NULL;
    }

    // Get the context field from ApiKeyRetriever
    jfieldID contextFieldID = (*env)->GetFieldID(env, retrieverClass, "context", "Landroid/content/Context;");
    if (contextFieldID == NULL)
    {
        LOGE("Failed to find context field");
        return NULL;
    }

    // Get the context object
    jobject contextObj = (*env)->GetObjectField(env, thiz, contextFieldID);
    if (contextObj == NULL)
    {
        LOGE("Failed to get context object");
        return NULL;
    }

    // Get the ApiKeyHelper class
    jclass helperClass = (*env)->FindClass(env, "com/example/playground/utils/ApiKeyHelper");
    if (helperClass == NULL)
    {
        LOGE("Failed to find ApiKeyHelper class");
        return NULL;
    }

    // Create an instance of ApiKeyHelper
    jmethodID helperConstructor = (*env)->GetMethodID(env, helperClass, "<init>", "()V");
    if (helperConstructor == NULL)
    {
        LOGE("Failed to find ApiKeyHelper constructor");
        return NULL;
    }

    jobject helperInstance = (*env)->NewObject(env, helperClass, helperConstructor);
    if (helperInstance == NULL)
    {
        LOGE("Failed to create ApiKeyHelper instance");
        return NULL;
    }

    // Get the retrieveApiKey method
    jmethodID retrieveMethod = (*env)->GetMethodID(env, helperClass, "retrieveApiKey", "(Ljava/io/File;)Ljava/lang/String;");
    if (retrieveMethod == NULL)
    {
        LOGE("Failed to find retrieveApiKey method");
        return NULL;
    }

    // Get the cache directory
    jobject cacheDir = (*env)->CallObjectMethod(env, contextObj, getCacheDirMethod);
    if (cacheDir == NULL)
    {
        LOGE("Failed to get cache directory");
        return NULL;
    }

    // Call the retrieveApiKey method
    jstring encryptedKey = (jstring)(*env)->CallObjectMethod(env, helperInstance, retrieveMethod, cacheDir);
    if (encryptedKey == NULL)
    {
        LOGE("Failed to retrieve encrypted key from Kotlin");
        return NULL;
    }

    // Convert Java string to C string
    const char *encryptedKeyStr = (*env)->GetStringUTFChars(env, encryptedKey, NULL);
    if (encryptedKeyStr == NULL)
    {
        LOGE("Failed to convert encrypted key to C string");
        return NULL;
    }

    LOGI("Encrypted key from EXIF: %s", encryptedKeyStr);

    // Define decryption key (2002012020020120)
    const unsigned char key[] = "2002012020020120";

    // Set up IV (all zeros - using character '0', not numeric 0)
    unsigned char iv[16];
    memset(iv, '0', 16);

    // Decode the Base64 encoded string
    BIO *b64, *bmem;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new_mem_buf(encryptedKeyStr, -1);
    bmem = BIO_push(b64, bmem);

    unsigned char encrypted[1024] = {0};
    int encryptedLen = BIO_read(bmem, encrypted, sizeof(encrypted));
    BIO_free_all(bmem);

    // Release the Java string
    (*env)->ReleaseStringUTFChars(env, encryptedKey, encryptedKeyStr);

    if (encryptedLen <= 0)
    {
        LOGE("Failed to decode Base64 string, length: %d", encryptedLen);
        return NULL;
    }

    LOGI("Decoded Base64 data length: %d", encryptedLen);

    // Decrypt the message
    char *decryptedKey = aes_decrypt(encrypted, encryptedLen, key, iv);
    if (decryptedKey == NULL)
    {
        LOGE("Decryption failed");
        return NULL;
    }

    LOGI("Successfully decrypted: %s", decryptedKey);

    // Create a Java string with the result
    jstring result = (*env)->NewStringUTF(env, decryptedKey);

    // Clean up
    free(decryptedKey);

    return result;
}