#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

char *aes_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                  const unsigned char *key, const unsigned char *iv)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    unsigned char *plaintext = (unsigned char *)malloc(ciphertext_len + EVP_MAX_BLOCK_LENGTH);

    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        free(plaintext);
        return NULL;
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
    {
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }

    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    {
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
    {
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    plaintext[plaintext_len] = '\0';

    return (char *)plaintext;
}

JNIEXPORT jstring JNICALL Java_com_example_playground_network_ApiKeyRetriever_retrieveApiKeyNative(
    JNIEnv *env, jobject thiz)
{
    jclass contextClass = (*env)->FindClass(env, "android/content/Context");
    if (contextClass == NULL)
    {
        return NULL;
    }

    jmethodID getCacheDirMethod = (*env)->GetMethodID(env, contextClass, "getCacheDir", "()Ljava/io/File;");
    if (getCacheDirMethod == NULL)
    {
        return NULL;
    }

    jclass retrieverClass = (*env)->GetObjectClass(env, thiz);
    if (retrieverClass == NULL)
    {
        return NULL;
    }

    jfieldID contextFieldID = (*env)->GetFieldID(env, retrieverClass, "context", "Landroid/content/Context;");
    if (contextFieldID == NULL)
    {
        return NULL;
    }

    jobject contextObj = (*env)->GetObjectField(env, thiz, contextFieldID);
    if (contextObj == NULL)
    {
        return NULL;
    }

    jclass helperClass = (*env)->FindClass(env, "com/example/playground/utils/ApiKeyHelper");
    if (helperClass == NULL)
    {
        return NULL;
    }

    jmethodID helperConstructor = (*env)->GetMethodID(env, helperClass, "<init>", "()V");
    if (helperConstructor == NULL)
    {
        return NULL;
    }

    jobject helperInstance = (*env)->NewObject(env, helperClass, helperConstructor);
    if (helperInstance == NULL)
    {
        return NULL;
    }

    jmethodID retrieveMethod = (*env)->GetMethodID(env, helperClass, "retrieveApiKey", "(Ljava/io/File;)Ljava/lang/String;");
    if (retrieveMethod == NULL)
    {
        return NULL;
    }

    jobject cacheDir = (*env)->CallObjectMethod(env, contextObj, getCacheDirMethod);
    if (cacheDir == NULL)
    {
        return NULL;
    }

    jstring encryptedKey = (jstring)(*env)->CallObjectMethod(env, helperInstance, retrieveMethod, cacheDir);
    if (encryptedKey == NULL)
    {
        return NULL;
    }

    const char *encryptedKeyStr = (*env)->GetStringUTFChars(env, encryptedKey, NULL);
    if (encryptedKeyStr == NULL)
    {
        return NULL;
    }

    const unsigned char key[] = "2002012020020120";

    unsigned char iv[16];
    memset(iv, '0', 16);

    BIO *b64, *bmem;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new_mem_buf(encryptedKeyStr, -1);
    bmem = BIO_push(b64, bmem);

    unsigned char encrypted[1024] = {0};
    int encryptedLen = BIO_read(bmem, encrypted, sizeof(encrypted));
    BIO_free_all(bmem);

    (*env)->ReleaseStringUTFChars(env, encryptedKey, encryptedKeyStr);

    if (encryptedLen <= 0)
    {
        return NULL;
    }

    char *decryptedKey = aes_decrypt(encrypted, encryptedLen, key, iv);
    if (decryptedKey == NULL)
    {
        return NULL;
    }

    jstring result = (*env)->NewStringUTF(env, decryptedKey);

    free(decryptedKey);

    return result;
}