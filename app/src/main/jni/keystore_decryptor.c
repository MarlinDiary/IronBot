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
    JNIEnv *env, jobject thiz)
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

    // Hardcoded encrypted message
    const char *encryptedBase64 = "8jPsLCgYQ26vNAXtBTEj3Q==";

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

// C function to decrypt the second fragment
char *decrypt_second_fragment()
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
        return NULL;
    }

    // Decrypt the message
    char *decrypted = aes_decrypt(encrypted, encryptedLen, key, iv);
    if (decrypted == NULL)
    {
        LOGE("Decryption failed for second fragment");
        return NULL;
    }

    // Log the decrypted result
    LOGI("Decrypted second fragment: %s", decrypted);

    return decrypted;
}

// Function to decrypt the second API key fragment
JNIEXPORT jstring JNICALL Java_com_example_playground_network_NativeDecryptor_decryptSecondFragment(
    JNIEnv *env, jobject thiz)
{
    // Call the C function to do the decryption
    char *decrypted = decrypt_second_fragment();

    if (decrypted == NULL)
    {
        return (*env)->NewStringUTF(env, "Decryption failed");
    }

    // Create a Java string with the result
    jstring result = (*env)->NewStringUTF(env, decrypted);

    // Clean up
    free(decrypted);

    return result;
}

// Function to decrypt the fifth API key fragment using triple XOR
JNIEXPORT jstring JNICALL Java_com_example_playground_network_NativeDecryptor_decryptFifthFragment(
    JNIEnv *env, jobject thiz)
{
    LOGI("Decrypting fifth API key fragment using triple XOR");

    // The encrypted data (in hex format)
    const char *encrypted_hex = "545C03585254045D520C5306070D565800535B565059060405075457000050535B025D0B5106015E";

    // The three keys for decryption
    const char *key1 = "4a17f315edc7aa28b1938eaf32d569da85ce14ab"; // First key
    const char *key2 = "f6c8d74b78bd12c5a14df0b4dff7a79b271cc215"; // Second key
    const char *key3 = "b35fe102c4da1fb12e749830d5cbe79a4494f2e0"; // Third key

    // Calculate the length of the encrypted data
    size_t encrypted_len = strlen(encrypted_hex) / 2;

    // Allocate memory for the binary encrypted data
    unsigned char *encrypted_data = (unsigned char *)malloc(encrypted_len);
    if (!encrypted_data)
    {
        LOGE("Memory allocation failed for encrypted data");
        return NULL;
    }

    // Convert hex string to binary
    for (size_t i = 0; i < encrypted_len; i++)
    {
        sscanf(&encrypted_hex[i * 2], "%2hhx", &encrypted_data[i]);
    }

    // Allocate memory for the decryption result
    unsigned char *result = (unsigned char *)malloc(encrypted_len + 1);
    if (!result)
    {
        LOGE("Memory allocation failed for result buffer");
        free(encrypted_data);
        return NULL;
    }

    // Copy encrypted data to result buffer for in-place decryption
    memcpy(result, encrypted_data, encrypted_len);

    // Triple XOR decryption (reverse order of encryption)
    // First, XOR with key3
    for (size_t i = 0; i < encrypted_len; i++)
    {
        result[i] ^= key3[i % strlen(key3)];
    }

    // Second, XOR with key2
    for (size_t i = 0; i < encrypted_len; i++)
    {
        result[i] ^= key2[i % strlen(key2)];
    }

    // Third, XOR with key1
    for (size_t i = 0; i < encrypted_len; i++)
    {
        result[i] ^= key1[i % strlen(key1)];
    }

    // Null-terminate the result
    result[encrypted_len] = '\0';

    LOGI("Decrypted fifth fragment: %s", result);

    // Convert binary result to Java string
    jstring jresult = (*env)->NewStringUTF(env, (char *)result);

    // Clean up
    free(encrypted_data);
    free(result);

    return jresult;
}

// Test function to run decrypt_second_fragment locally
#ifdef LOCAL_TEST
int main()
{
    // Initialize OpenSSL library
    OpenSSL_add_all_algorithms();

    printf("Running test for decrypt_second_fragment...\n");

    char *decrypted = decrypt_second_fragment();
    if (decrypted != NULL)
    {
        printf("Decryption successful. Result: %s\n", decrypted);
        free(decrypted); // Don't forget to free the memory
    }
    else
    {
        printf("Decryption failed\n");
    }

    // Clean up OpenSSL
    EVP_cleanup();

    return 0;
}
#endif