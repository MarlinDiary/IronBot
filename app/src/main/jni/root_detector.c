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

// 检查常见的 su 文件路径
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
            return 1; // 文件存在
        }
    }
    
    return 0; // 所有文件都不存在
}

// 检查 /proc/mounts 中的挂载点，寻找可写的系统分区
static int check_mount_points() {
    FILE* fp = fopen("/proc/mounts", "r");
    if (fp == NULL) {
        return 0; // 无法打开文件
    }
    
    char line[512];
    int result = 0;
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        // 检查 /system 分区是否以 rw（可读写）方式挂载
        if (strstr(line, "/system") && strstr(line, " rw,") != NULL) {
            result = 1;
            break;
        }
    }
    
    fclose(fp);
    return result;
}

// 检查是否安装了常见的 root 应用
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
            return 1; // 目录存在
        }
    }
    
    return 0; // 所有目录都不存在
}

// 检查是否存在 Magisk 隐藏目录
static int check_magisk_hide() {
    if (access("/sbin/.magisk", F_OK) == 0 ||
        access("/dev/.magisk", F_OK) == 0 ||
        access("/.magisk", F_OK) == 0) {
        return 1;
    }
    return 0;
}

// 检查是否可以执行 su 命令
static int check_su_execution() {
    int result = 0;
    FILE* pipe = popen("su -c id", "r");
    
    if (pipe != NULL) {
        char buffer[128];
        
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            if (strstr(buffer, "uid=0") != NULL) {
                result = 1; // 可以执行 su 命令且获取到 root 权限
            }
        }
        
        pclose(pipe);
    }
    
    return result;
}

// 检查文件测试 - 尝试在受保护的目录创建文件
static int check_system_writable() {
    const char* test_path = "/system/test_root_access";
    FILE* file = fopen(test_path, "w");
    
    if (file != NULL) {
        fclose(file);
        unlink(test_path); // 删除测试文件
        return 1; // 能够在 /system 目录下创建文件，说明有 root 权限
    }
    
    return 0; // 无法在 /system 目录下创建文件
}

// 综合检测函数
static int is_device_rooted() {
    // 检查 su 路径
    if (check_su_paths()) {
        return 1;
    }
    
    // 检查挂载点
    if (check_mount_points()) {
        return 1;
    }
    
    // 检查 root 应用包
    if (check_root_packages_dir()) {
        return 1;
    }
    
    // 检查 Magisk 隐藏
    if (check_magisk_hide()) {
        return 1;
    }
    
    // 检查 su 命令执行
    if (check_su_execution()) {
        return 1;
    }
    
    // 检查系统分区可写性
    if (check_system_writable()) {
        return 1;
    }
    
    return 0; // 所有检测都通过，设备可能没有 root
}

// JNI 接口函数
JNIEXPORT jboolean JNICALL
Java_com_example_playground_util_RootDetectorNative_isDeviceRooted(JNIEnv *env, jobject thiz) {
    return (jboolean)(is_device_rooted() ? JNI_TRUE : JNI_FALSE);
} 