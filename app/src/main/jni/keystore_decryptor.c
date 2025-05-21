#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <android/log.h>

#define LOG_TAG "KeystoreDecryptor"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Base64 encoding function
char *base64_encode(const unsigned char *input, int length)
{
    BIO *bmem, *b64;
    BUF_MEM *bptr;
    char *buff;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    buff = (char *)malloc(bptr->length + 1);
    memcpy(buff, bptr->data, bptr->length);
    buff[bptr->length] = 0;

    BIO_free_all(b64);
    return buff;
}

// AES decryption function
char *aes_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                  const unsigned char *key, const unsigned char *iv)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    unsigned char *plaintext = (unsigned char *)malloc(ciphertext_len);

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

// Function to get third key's first 16 chars from Java NativeKeyStore
jstring Java_com_example_playground_network_NativeDecryptor_decryptMessage(
    JNIEnv *env, jobject thiz, jstring encryptedMsg)
{

    // Get the NativeKeyStore class
    jclass keyStoreClass = (*env)->FindClass(env, "com/example/playground/network/NativeKeyStore");
    if (keyStoreClass == NULL)
    {
        LOGE("Failed to find NativeKeyStore class");
        return NULL;
    }

    // Get the KEYS field
    jfieldID keysFieldID = (*env)->GetStaticFieldID(env, keyStoreClass, "KEYS", "[Ljava/lang/String;");
    if (keysFieldID == NULL)
    {
        LOGE("Failed to find KEYS field");
        return NULL;
    }

    // Get the KEYS array
    jobjectArray keysArray = (*env)->GetStaticObjectField(env, keyStoreClass, keysFieldID);
    if (keysArray == NULL)
    {
        LOGE("Failed to get KEYS array");
        return NULL;
    }

    // Get the third key (index 2)
    jstring thirdKeyJString = (jstring)(*env)->GetObjectArrayElement(env, keysArray, 2);
    if (thirdKeyJString == NULL)
    {
        LOGE("Failed to get third key");
        return NULL;
    }

    // Convert jstring to C string
    const char *thirdKeyStr = (*env)->GetStringUTFChars(env, thirdKeyJString, NULL);
    if (thirdKeyStr == NULL)
    {
        LOGE("Failed to convert third key to C string");
        return NULL;
    }

    // Log the third key
    LOGI("Third key: %s", thirdKeyStr);

    // Use first 16 characters of the third key as the decryption key
    unsigned char aesKey[17];
    strncpy((char *)aesKey, thirdKeyStr, 16);
    aesKey[16] = '\0';

    LOGI("AES Key (first 16 chars): %s", aesKey);

    // Release the Java string
    (*env)->ReleaseStringUTFChars(env, thirdKeyJString, thirdKeyStr);

    // Setup IV (all zeros)
    unsigned char iv[16];
    memset(iv, '0', 16);

    // Get the encrypted message
    const char *encryptedBase64 = (*env)->GetStringUTFChars(env, encryptedMsg, NULL);
    if (encryptedBase64 == NULL)
    {
        LOGE("Failed to get encrypted message");
        return NULL;
    }

    // Decode from Base64
    BIO *b64, *bmem;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new_mem_buf(encryptedBase64, -1);
    bmem = BIO_push(b64, bmem);

    unsigned char encrypted[1024] = {0};
    int encryptedLen = BIO_read(bmem, encrypted, sizeof(encrypted));
    BIO_free_all(bmem);

    (*env)->ReleaseStringUTFChars(env, encryptedMsg, encryptedBase64);

    if (encryptedLen <= 0)
    {
        LOGE("Failed to decode Base64 string");
        return NULL;
    }

    // Decrypt the message
    char *decrypted = aes_decrypt(encrypted, encryptedLen, aesKey, iv);
    if (decrypted == NULL)
    {
        LOGE("Decryption failed");
        return NULL;
    }

    // Create a Java string with the result
    jstring result = (*env)->NewStringUTF(env, decrypted);

    // Clean up
    free(decrypted);

    return result;
}

// Function to decrypt the second API key fragment
JNIEXPORT jstring JNICALL Java_com_example_playground_network_NativeDecryptor_decryptSecondFragment(
    JNIEnv *env, jobject thiz)
{
    // The key we use for decryption (second key fragment)
    const unsigned char key[] = "aieIIiottweninfo";

    // IV (all ones)
    unsigned char iv[16];
    memset(iv, '1', 16);

    // The encrypted message in Base64
    const char *encryptedBase64 = "qGRv/ZNXAKL8L1XOwBTpI+J/opXZC+WtvRAMvqFb4fs=";

    // Decode from Base64
    BIO *b64, *bmem;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new_mem_buf(encryptedBase64, -1);
    bmem = BIO_push(b64, bmem);

    unsigned char encrypted[1024] = {0};
    int encryptedLen = BIO_read(bmem, encrypted, sizeof(encrypted));
    BIO_free_all(bmem);

    if (encryptedLen <= 0)
    {
        LOGE("Failed to decode Base64 string for second fragment");
        return (*env)->NewStringUTF(env, "Base64 decoding failed");
    }

    // Decrypt the message
    char *decrypted = aes_decrypt(encrypted, encryptedLen, key, iv);
    if (decrypted == NULL)
    {
        LOGE("Decryption failed for second fragment");
        return (*env)->NewStringUTF(env, "Decryption failed");
    }

    // Create a Java string with the result
    jstring result = (*env)->NewStringUTF(env, decrypted);

    // Log the decrypted result
    LOGI("Decrypted second fragment: %s", decrypted);

    // Clean up
    free(decrypted);

    return result;
}