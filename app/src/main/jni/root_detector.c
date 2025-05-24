#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

static int check_su_paths() {
    const char* su_paths[] = {
        "/system/bin/su",
        "/system/xbin/su",
        "/sbin/su",
        "/system/su",
        "/system/bin/.ext/.su",
        "/system/usr/we-need-root/su",
        "/data/local/xbin/su",
        "/data/local/bin/su",
        "/system/sd/xbin/su"
    };
    
    size_t num_paths = sizeof(su_paths) / sizeof(su_paths[0]);
    
    for (size_t i = 0; i < num_paths; ++i) {
        if (access(su_paths[i], F_OK) == 0) {
            return 1;
        }
    }
    
    return 0;
}

static int check_mount_points() {
    FILE* fp = fopen("/proc/mounts", "r");
    if (fp == NULL) {
        return 0;
    }
    
    char line[512];
    int result = 0;
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "/system") && strstr(line, " rw,") != NULL) {
            result = 1;
            break;
        }
    }
    
    fclose(fp);
    return result;
}

static int check_root_packages_dir() {
    const char* root_packages[] = {
        "/data/data/com.noshufou.android.su",
        "/data/data/com.koushikdutta.superuser",
        "/data/data/eu.chainfire.supersu",
        "/data/data/com.topjohnwu.magisk"
    };
    
    size_t num_packages = sizeof(root_packages) / sizeof(root_packages[0]);
    
    for (size_t i = 0; i < num_packages; ++i) {
        if (access(root_packages[i], F_OK) == 0) {
            return 1;
        }
    }
    
    return 0;
}

static int check_magisk_hide() {
    if (access("/sbin/.magisk", F_OK) == 0 ||
        access("/dev/.magisk", F_OK) == 0 ||
        access("/.magisk", F_OK) == 0) {
        return 1;
    }
    return 0;
}

static int check_su_execution() {
    int result = 0;
    FILE* pipe = popen("su -c id", "r");
    
    if (pipe != NULL) {
        char buffer[128];
        
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            if (strstr(buffer, "uid=0") != NULL) {
                result = 1;
            }
        }
        
        pclose(pipe);
    }
    
    return result;
}

static int check_system_writable() {
    const char* test_path = "/system/test_root_access";
    FILE* file = fopen(test_path, "w");
    
    if (file != NULL) {
        fclose(file);
        unlink(test_path);
        return 1;
    }
    
    return 0;
}

static int is_device_rooted() {
    if (check_su_paths()) {
        return 1;
    }
    
    if (check_mount_points()) {
        return 1;
    }
    
    if (check_root_packages_dir()) {
        return 1;
    }
    
    if (check_magisk_hide()) {
        return 1;
    }
    
    if (check_su_execution()) {
        return 1;
    }
    
    if (check_system_writable()) {
        return 1;
    }
    
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_com_example_playground_util_RootDetectorNative_isDeviceRooted(JNIEnv *env, jobject thiz) {
    return (jboolean)(is_device_rooted() ? JNI_TRUE : JNI_FALSE);
} 