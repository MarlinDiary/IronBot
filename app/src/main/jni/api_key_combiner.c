#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static char *cached_third_part = NULL;
static char *cached_fourth_part = NULL;

extern char *decrypt_second_fragment();
extern char *decrypt_fifth_fragment();
extern char *getThirdApiKeyPart();

int detect_frida() 
{
    int frida_detected = 0;
    
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
    
    if (!frida_detected) {
        struct sockaddr_in sa;
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock != -1) {
            memset(&sa, 0, sizeof(sa));
            sa.sin_family = AF_INET;
            sa.sin_port = htons(27042);
            inet_aton("127.0.0.1", &sa.sin_addr);
            
            if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)) != -1) {
                frida_detected = 1;
            }
            close(sock);
        }
    }
    
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
        return 1;
    }
    
    return 0;
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *vm, void *reserved)
{
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

typedef struct
{
    char *memory;
    size_t size;
} MemoryStruct;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL)
    {
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char *extract_signature(const char *json_response)
{
    if (!json_response)
        return NULL;

    char *sig_start = strstr(json_response, "\"signature\":");
    if (!sig_start)
        return NULL;

    sig_start += 12;
    while (*sig_start == ' ' || *sig_start == '\"')
        sig_start++;

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

char *build_full_url(const char *base_url, const char *path)
{
    if (!path)
        return NULL;

    size_t path_len = strlen(path);
    char *clean_path = (char *)malloc(path_len + 1);
    if (!clean_path)
        return NULL;

    size_t clean_index = 0;
    for (size_t i = 0; i < path_len; i++)
    {
        if (path[i] != '\"')
        {
            clean_path[clean_index++] = path[i];
        }
    }
    clean_path[clean_index] = '\0';

    if (strstr(clean_path, "http://") == clean_path || strstr(clean_path, "https://") == clean_path)
    {
        return clean_path;
    }

    size_t base_len = strlen(base_url);
    size_t need_slash = (clean_path[0] != '/' && base_url[base_len - 1] != '/') ? 1 : 0;

    char *full_url = (char *)malloc(base_len + clean_index + need_slash + 1);
    if (!full_url)
    {
        free(clean_path);
        return NULL;
    }

    strcpy(full_url, base_url);

    if (need_slash)
    {
        strcat(full_url, "/");
    }
    else if (clean_path[0] == '/' && base_url[base_len - 1] == '/')
    {
        strcat(full_url, clean_path + 1);
        free(clean_path);
        return full_url;
    }

    strcat(full_url, clean_path);
    free(clean_path);

    return full_url;
}

JNIEXPORT jstring JNICALL
Java_com_example_playground_network_ApiKeyCombiner_combineApiKey(JNIEnv *env, jobject thiz, jstring promptJString)
{
    const char *prompt = NULL;
    if (promptJString != NULL)
    {
        prompt = (*env)->GetStringUTFChars(env, promptJString, NULL);
    }
    else
    {
        return (*env)->NewStringUTF(env, "Error: No prompt provided");
    }
    
    if (detect_frida()) {
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Security violation detected");
    }

    jclass decryptorClass = (*env)->FindClass(env, "com/example/playground/network/NativeDecryptor");
    if (decryptorClass == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to find NativeDecryptor class");
    }

    jmethodID constructor = (*env)->GetMethodID(env, decryptorClass, "<init>", "()V");
    if (constructor == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to get NativeDecryptor constructor");
    }

    jobject decryptorObj = (*env)->NewObject(env, decryptorClass, constructor);
    if (decryptorObj == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to create NativeDecryptor instance");
    }

    jmethodID decryptMessageMethod = (*env)->GetMethodID(env, decryptorClass, "decryptMessage", "()Ljava/lang/String;");
    if (decryptMessageMethod == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to get decryptMessage method");
    }

    jstring firstPartJString = (jstring)(*env)->CallObjectMethod(env, decryptorObj, decryptMessageMethod);
    if (firstPartJString == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to get first part of API key");
    }

    const char *firstPart = (*env)->GetStringUTFChars(env, firstPartJString, NULL);

    char *secondPart = decrypt_second_fragment();
    if (secondPart == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to get second part of API key");
    }

    char *thirdPart = NULL;
    if (cached_third_part != NULL)
    {
        thirdPart = strdup(cached_third_part);
    }
    else
    {
        thirdPart = getThirdApiKeyPart();
        if (thirdPart != NULL && strlen(thirdPart) > 0)
        {
            cached_third_part = strdup(thirdPart);
        }
    }

    if (thirdPart == NULL || strlen(thirdPart) == 0)
    {
        (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
        free(secondPart);
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to get third part of API key");
    }

    const char *fourthPart = NULL;

    if (cached_fourth_part != NULL)
    {
        fourthPart = cached_fourth_part;
    }
    else
    {
        jclass activityThreadClass = (*env)->FindClass(env, "android/app/ActivityThread");
        jmethodID currentActivityThreadMethod = (*env)->GetStaticMethodID(env, activityThreadClass, "currentActivityThread", "()Landroid/app/ActivityThread;");
        jobject activityThread = (*env)->CallStaticObjectMethod(env, activityThreadClass, currentActivityThreadMethod);

        jmethodID getApplicationMethod = (*env)->GetMethodID(env, activityThreadClass, "getApplication", "()Landroid/app/Application;");
        jobject application = (*env)->CallObjectMethod(env, activityThread, getApplicationMethod);

        jclass retrieverClass = (*env)->FindClass(env, "com/example/playground/network/ApiKeyRetriever");
        jmethodID retrieverConstructor = (*env)->GetMethodID(env, retrieverClass, "<init>", "(Landroid/content/Context;)V");
        jobject retrieverObj = (*env)->NewObject(env, retrieverClass, retrieverConstructor, application);

        jmethodID retrieveMethod = (*env)->GetMethodID(env, retrieverClass, "retrieveApiKeyNative", "()Ljava/lang/String;");
        jstring fourthPartJString = (jstring)(*env)->CallObjectMethod(env, retrieverObj, retrieveMethod);

        if (fourthPartJString != NULL)
        {
            fourthPart = (*env)->GetStringUTFChars(env, fourthPartJString, NULL);

            cached_fourth_part = strdup(fourthPart);

            (*env)->ReleaseStringUTFChars(env, fourthPartJString, fourthPart);
            fourthPart = cached_fourth_part;
        }
        else
        {
            fourthPart = "";
        }
    }

    char *fifthPart = decrypt_fifth_fragment();
    if (fifthPart == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
        free(secondPart);
        free(thirdPart);
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Failed to get fifth part of API key");
    }

    size_t totalLength = strlen(firstPart) + strlen(secondPart) + strlen(thirdPart) +
                         (fourthPart ? strlen(fourthPart) : 0) + strlen(fifthPart) + 1;

    char *combinedKey = (char *)malloc(totalLength);
    if (combinedKey == NULL)
    {
        (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
        free(secondPart);
        free(thirdPart);
        free(fifthPart);
        (*env)->ReleaseStringUTFChars(env, promptJString, prompt);
        return (*env)->NewStringUTF(env, "Error: Memory allocation failed");
    }

    strcpy(combinedKey, firstPart);
    strcat(combinedKey, secondPart);
    strcat(combinedKey, thirdPart);
    if (fourthPart)
    {
        strcat(combinedKey, fourthPart);
    }
    strcat(combinedKey, fifthPart);

    CURL *curl;
    CURLcode res;
    jstring result = NULL;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl)
    {
        MemoryStruct authChunk;
        authChunk.memory = malloc(1);
        authChunk.size = 0;

        char auth_url[100];
        sprintf(auth_url, "https://ai.elliottwen.info/auth");
        curl_easy_setopt(curl, CURLOPT_URL, auth_url);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        struct curl_slist *auth_headers = NULL;
        char auth_header[1024];
        sprintf(auth_header, "Authorization: %s", combinedKey);
        auth_headers = curl_slist_append(auth_headers, auth_header);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, auth_headers);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&authChunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            result = (*env)->NewStringUTF(env, "Error: Authentication request failed");
        }
        else
        {
            char *signature = extract_signature(authChunk.memory);
            if (signature)
            {
                MemoryStruct imageChunk;
                imageChunk.memory = malloc(1);
                imageChunk.size = 0;

                curl_easy_reset(curl);

                char image_url[100];
                sprintf(image_url, "https://ai.elliottwen.info/generate_image");
                curl_easy_setopt(curl, CURLOPT_URL, image_url);

                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

                struct curl_slist *image_headers = NULL;
                image_headers = curl_slist_append(image_headers, auth_header);
                image_headers = curl_slist_append(image_headers, "Content-Type: application/json");
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, image_headers);

                char request_body[2048];
                sprintf(request_body, "{\"signature\":\"%s\",\"prompt\":\"%s\"}", signature, prompt);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);

                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&imageChunk);

                res = curl_easy_perform(curl);

                if (res != CURLE_OK)
                {
                    result = (*env)->NewStringUTF(env, "Error: Image generation request failed");
                }
                else
                {
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

                curl_slist_free_all(image_headers);
                free(imageChunk.memory);
                free(signature);
            }
            else
            {
                result = (*env)->NewStringUTF(env, "Error: Failed to extract signature");
            }
        }

        curl_slist_free_all(auth_headers);
        free(authChunk.memory);

        curl_easy_cleanup(curl);
    }
    else
    {
        result = (*env)->NewStringUTF(env, "Error: Failed to initialize CURL");
    }

    curl_global_cleanup();

    (*env)->ReleaseStringUTFChars(env, firstPartJString, firstPart);
    free(secondPart);
    free(thirdPart);
    free(fifthPart);
    free(combinedKey);
    (*env)->ReleaseStringUTFChars(env, promptJString, prompt);

    return result;
}