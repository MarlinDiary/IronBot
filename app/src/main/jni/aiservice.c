#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <curl/curl.h>

typedef struct
{
    char *data;
    size_t size;
} ResponseData;

typedef struct {
    const char* hostname;
    const char* expected_fingerprint;
    int cert_verified;
} CertVerifyInfo;

static int ssl_verify_callback(int preverify_ok, X509_STORE_CTX* ctx) {
    SSL* ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    CertVerifyInfo* verify_info = SSL_get_ex_data(ssl, 0);
    
    if (!verify_info) {
        return 0;
    }

    if (preverify_ok == 0) {
        verify_info->cert_verified = 0;
        return 0;
    }

    X509* cert = X509_STORE_CTX_get_current_cert(ctx);
    if (!cert) {
        verify_info->cert_verified = 0;
        return 0;
    }

    int depth = X509_STORE_CTX_get_error_depth(ctx);
    
    if (depth == 1) {
        unsigned char md[EVP_MAX_MD_SIZE];
        unsigned int md_len;
        
        if (!X509_digest(cert, EVP_sha256(), md, &md_len)) {
            verify_info->cert_verified = 0;
            return 0;
        }
        
        BIO *bmem = BIO_new(BIO_s_mem());
        BIO *b64 = BIO_new(BIO_f_base64());
        BIO_push(b64, bmem);
        BIO_write(b64, md, md_len);
        BIO_flush(b64);
        
        char fingerprint[100];
        memset(fingerprint, 0, sizeof(fingerprint));
        
        BUF_MEM *bptr;
        BIO_get_mem_ptr(b64, &bptr);
        
        if (bptr->length > 0 && bptr->length < sizeof(fingerprint)) {
            memcpy(fingerprint, bptr->data, bptr->length - 1);
        }
        
        BIO_free_all(b64);
        
        char full_fingerprint[150];
        snprintf(full_fingerprint, sizeof(full_fingerprint), "sha256/%s", fingerprint);
        
        if (strcmp(full_fingerprint, verify_info->expected_fingerprint) != 0) {
            verify_info->cert_verified = 0;
            return 0;
        }
    }
    
    verify_info->cert_verified = 1;
    return 1;
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    ResponseData *resp = (ResponseData *)userp;

    char *ptr = realloc(resp->data, resp->size + realsize + 1);
    if (!ptr)
    {
        return 0;
    }

    resp->data = ptr;
    memcpy(&(resp->data[resp->size]), contents, realsize);
    resp->size += realsize;
    resp->data[resp->size] = 0;

    return realsize;
}

char *aes_decrypt(const char *ciphertext_base64, const char *key, const char *iv)
{
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
        return NULL;
    }

    ciphertext_len_actual = BIO_read(bmem, ciphertext_bin, strlen(ciphertext_base64));
    BIO_free_all(bmem);

    if (ciphertext_len_actual <= 0)
    {
        free(ciphertext_bin);
        return NULL;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        free(ciphertext_bin);
        return NULL;
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, (unsigned char *)key, (unsigned char *)iv))
    {
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext_bin);
        return NULL;
    }

    int plaintext_len;
    int len;
    unsigned char *plaintext = malloc(ciphertext_len_actual + EVP_CIPHER_block_size(EVP_aes_128_cbc()));
    if (plaintext == NULL)
    {
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext_bin);
        return NULL;
    }

    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext_bin, ciphertext_len_actual))
    {
        free(plaintext);
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext_bin);
        return NULL;
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
    {
        free(plaintext);
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext_bin);
        return NULL;
    }
    plaintext_len += len;

    plaintext[plaintext_len] = '\0';

    EVP_CIPHER_CTX_free(ctx);
    free(ciphertext_bin);

    return (char *)plaintext;
}

JNIEXPORT jstring JNICALL
Java_com_example_playground_network_AIImageService_00024Companion_getRealBaseUrl(
    JNIEnv *env,
    jobject thiz,
    jstring originalUrl)
{
    const char *url = (*env)->GetStringUTFChars(env, originalUrl, NULL);
    if (url == NULL)
    {
        return NULL;
    }

    size_t len = strlen(url);

    char *modifiedUrl = (char *)malloc(len);
    if (modifiedUrl == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, originalUrl, url);
        return NULL;
    }

    int removedT = 0;
    size_t j = 0;

    for (size_t i = 0; i < len; i++)
    {
        if (!removedT && url[i] == 't' && i > 0 &&
            url[i - 1] == 't' && i < len - 1 && url[i + 1] == 'w')
        {
            removedT = 1;
            continue;
        }
        modifiedUrl[j++] = url[i];
    }

    modifiedUrl[j] = '\0';

    jstring result = (*env)->NewStringUTF(env, modifiedUrl);

    free(modifiedUrl);
    (*env)->ReleaseStringUTFChars(env, originalUrl, url);

    return result;
}

char *getThirdApiKeyPart()
{
    const char *encrypted_apikey = "lmyL2liG91r65tQGgv9Hr5XdNtNtg1WnwmCSf2HlcO978fHbmB6MyXFqOiQrPXUlaUIkIrYOKsAaIUu7ytUAm/N9fcrFZdsnBSO0UZojdswwUdnmBDHdD18X3tbHOnAtAGX5FcTjlUYXGPO0PzcH9yeQGgdsrk68ElnvEbKOC4/iyV2sBFjqCz45KPUlv511";
    const char *key1 = "1996090520020120";
    const char *iv1 = "1996090520020120";

    char *apikey = aes_decrypt(encrypted_apikey, key1, iv1);
    if (!apikey)
    {
        return strdup("Error: Failed to decrypt apikey");
    }

    CURL *curl;
    CURLcode res;
    ResponseData response;
    response.data = malloc(1);
    response.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    char *signature = NULL;
    char *final_key = NULL;
    char *result = NULL;

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "https://ai.elliotwen.info/auth");

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        struct curl_slist *headers = NULL;
        char auth_header[1024];
        snprintf(auth_header, sizeof(auth_header), "Authorization: %s", apikey);
        headers = curl_slist_append(headers, auth_header);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK)
        {
            char *sig_start = strstr(response.data, "\"signature\":");
            if (sig_start)
            {
                sig_start += 13;
                char *sig_end = strchr(sig_start, '"');
                if (sig_end)
                {
                    size_t sig_len = sig_end - sig_start;
                    signature = malloc(sig_len + 1);
                    strncpy(signature, sig_start, sig_len);
                    signature[sig_len] = '\0';

                    if (strlen(signature) >= 26)
                    {
                        char *key2 = malloc(17);
                        strncpy(key2, signature + 9, 16);
                        key2[16] = '\0';

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

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    free(apikey);
    if (signature)
        free(signature);
    free(response.data);

    if (!result)
    {
        result = strdup("");
    }

    return result;
}

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

JNIEXPORT jboolean JNICALL
Java_com_example_playground_network_AIImageService_00024Companion_verifyCertificate(
    JNIEnv *env,
    jobject thiz,
    jstring hostname_jstr,
    jstring expected_fingerprint_jstr)
{
    const char *hostname = (*env)->GetStringUTFChars(env, hostname_jstr, NULL);
    const char *expected_fingerprint = (*env)->GetStringUTFChars(env, expected_fingerprint_jstr, NULL);
    
    CURL *curl;
    CURLcode res;
    int certificate_problem = 1;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if (curl) {
        char url[256];
        snprintf(url, sizeof(url), "https://%s", hostname);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        ResponseData response;
        response.data = malloc(1);
        response.size = 0;
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        
        res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            
            struct curl_slist *slist;
            res = curl_easy_getinfo(curl, CURLINFO_CERTINFO, &slist);
            
            if (res == CURLE_OK && slist) {
                certificate_problem = 0;
            }
            
            free(response.data);
        }
        
        curl_easy_cleanup(curl);
    }
    
    curl_global_cleanup();
    (*env)->ReleaseStringUTFChars(env, hostname_jstr, hostname);
    (*env)->ReleaseStringUTFChars(env, expected_fingerprint_jstr, expected_fingerprint);
    
    return certificate_problem ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    SSL_library_init();
    return JNI_VERSION_1_6;
}