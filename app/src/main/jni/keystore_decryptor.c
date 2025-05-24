#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

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

char *aes_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                  const unsigned char *key, const unsigned char *iv)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    unsigned char *plaintext = (unsigned char *)malloc(ciphertext_len);

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

jstring Java_com_example_playground_network_NativeDecryptor_decryptMessage(
    JNIEnv *env, jobject thiz)
{
    jclass keyStoreClass = (*env)->FindClass(env, "com/example/playground/network/NativeKeyStore");
    if (keyStoreClass == NULL)
    {
        return NULL;
    }

    jfieldID keysFieldID = (*env)->GetStaticFieldID(env, keyStoreClass, "KEYS", "[Ljava/lang/String;");
    if (keysFieldID == NULL)
    {
        return NULL;
    }

    jobjectArray keysArray = (*env)->GetStaticObjectField(env, keyStoreClass, keysFieldID);
    if (keysArray == NULL)
    {
        return NULL;
    }

    jstring thirdKeyJString = (jstring)(*env)->GetObjectArrayElement(env, keysArray, 2);
    if (thirdKeyJString == NULL)
    {
        return NULL;
    }

    const char *thirdKeyStr = (*env)->GetStringUTFChars(env, thirdKeyJString, NULL);
    if (thirdKeyStr == NULL)
    {
        return NULL;
    }

    unsigned char aesKey[17];
    strncpy((char *)aesKey, thirdKeyStr, 16);
    aesKey[16] = '\0';

    (*env)->ReleaseStringUTFChars(env, thirdKeyJString, thirdKeyStr);

    unsigned char iv[16];
    memset(iv, '0', 16);

    const char *encryptedBase64 = "8jPsLCgYQ26vNAXtBTEj3Q==";

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
        return NULL;
    }

    char *decrypted = aes_decrypt(encrypted, encryptedLen, aesKey, iv);
    if (decrypted == NULL)
    {
        return NULL;
    }

    jstring result = (*env)->NewStringUTF(env, decrypted);

    free(decrypted);

    return result;
}

char *decrypt_second_fragment()
{
    const unsigned char key[] = "aieIIiottweninfo";

    unsigned char iv[16];
    memset(iv, '1', 16);

    const char *encryptedBase64 = "qGRv/ZNXAKL8L1XOwBTpI+J/opXZC+WtvRAMvqFb4fs=";

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
        return NULL;
    }

    char *decrypted = aes_decrypt(encrypted, encryptedLen, key, iv);
    if (decrypted == NULL)
    {
        return NULL;
    }

    return decrypted;
}

JNIEXPORT jstring JNICALL Java_com_example_playground_network_NativeDecryptor_decryptSecondFragment(
    JNIEnv *env, jobject thiz)
{
    char *decrypted = decrypt_second_fragment();

    if (decrypted == NULL)
    {
        return (*env)->NewStringUTF(env, "Decryption failed");
    }

    jstring result = (*env)->NewStringUTF(env, decrypted);

    free(decrypted);

    return result;
}

char *decrypt_fifth_fragment()
{
    const char *encrypted_hex = "545C03585254045D520C5306070D565800535B565059060405075457000050535B025D0B5106015E";

    const char *key1 = "4a17f315edc7aa28b1938eaf32d569da85ce14ab";
    const char *key2 = "f6c8d74b78bd12c5a14df0b4dff7a79b271cc215";
    const char *key3 = "b35fe102c4da1fb12e749830d5cbe79a4494f2e0";

    size_t encrypted_len = strlen(encrypted_hex) / 2;

    unsigned char *encrypted_data = (unsigned char *)malloc(encrypted_len);
    if (!encrypted_data)
    {
        return NULL;
    }

    for (size_t i = 0; i < encrypted_len; i++)
    {
        sscanf(&encrypted_hex[i * 2], "%2hhx", &encrypted_data[i]);
    }

    unsigned char *result = (unsigned char *)malloc(encrypted_len + 1);
    if (!result)
    {
        free(encrypted_data);
        return NULL;
    }

    memcpy(result, encrypted_data, encrypted_len);

    for (size_t i = 0; i < encrypted_len; i++)
    {
        result[i] ^= key3[i % strlen(key3)];
    }

    for (size_t i = 0; i < encrypted_len; i++)
    {
        result[i] ^= key2[i % strlen(key2)];
    }

    for (size_t i = 0; i < encrypted_len; i++)
    {
        result[i] ^= key1[i % strlen(key1)];
    }

    result[encrypted_len] = '\0';

    free(encrypted_data);

    return (char *)result;
}