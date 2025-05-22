package com.example.playground.util

import java.io.File
import java.io.BufferedReader
import java.io.InputStreamReader
import android.os.Build
import android.content.Context
import android.content.pm.PackageManager

/**
 * 工具类用于检测设备是否已被 root
 */
class RootChecker(private val context: Context) {

    // JNI根检测实例
    private val nativeRootDetector = RootDetectorNative()

    /**
     * 检查设备是否已被 root
     * 同时使用Java实现和Native实现的检测方法
     * @return 如果设备已被 root 则返回 true，否则返回 false
     */
    fun isDeviceRooted(): Boolean {
        // 先使用Java实现的方法检测
        val javaResult = checkRootMethod1() || checkRootMethod2() || checkRootMethod3() || checkRootMethod4() || checkRootMethod5()
        
        // 再使用JNI实现的方法检测
        val nativeResult = nativeRootDetector.isSafeDeviceRooted()
        
        // 任一方法检测到root，就返回true
        return javaResult || nativeResult
    }

    /**
     * 方法1：检查常见的 root 管理应用包名
     */
    private fun checkRootMethod1(): Boolean {
        val rootApps = arrayOf(
            "com.noshufou.android.su",
            "com.noshufou.android.su.elite",
            "eu.chainfire.supersu",
            "com.koushikdutta.superuser",
            "com.thirdparty.superuser",
            "com.yellowes.su",
            "com.topjohnwu.magisk",
            "com.kingroot.kinguser",
            "com.kingo.root",
            "com.smedialink.oneclickroot",
            "com.zhiqupk.root.global",
            "com.alephzain.framaroot"
        )

        val pm = context.packageManager
        for (app in rootApps) {
            try {
                pm.getPackageInfo(app, PackageManager.GET_ACTIVITIES)
                return true
            } catch (e: Exception) {
                // 包名不存在，继续检查下一个
            }
        }
        return false
    }

    /**
     * 方法2：检查是否存在常见的 su 二进制文件
     */
    private fun checkRootMethod2(): Boolean {
        val paths = arrayOf(
            "/system/bin/su",
            "/system/xbin/su",
            "/sbin/su",
            "/system/su",
            "/system/bin/.ext/.su",
            "/system/usr/we-need-root/su",
            "/system/app/Superuser.apk",
            "/system/app/SuperSU.apk",
            "/data/local/xbin/su",
            "/data/local/bin/su",
            "/system/sd/xbin/su"
        )

        for (path in paths) {
            if (File(path).exists()) {
                return true
            }
        }
        return false
    }

    /**
     * 方法3：尝试执行 su 命令
     */
    private fun checkRootMethod3(): Boolean {
        var process: Process? = null
        return try {
            process = Runtime.getRuntime().exec(arrayOf("su", "-c", "id"))
            val reader = BufferedReader(InputStreamReader(process.inputStream))
            val line = reader.readLine()
            line != null && line.contains("uid=0")
        } catch (e: Exception) {
            false
        } finally {
            process?.destroy()
        }
    }
    
    /**
     * 方法4：检查 build 标签是否包含可疑关键字
     */
    private fun checkRootMethod4(): Boolean {
        val buildTags = Build.TAGS
        return buildTags != null && buildTags.contains("test-keys")
    }
    
    /**
     * 方法5：检查系统属性
     */
    private fun checkRootMethod5(): Boolean {
        try {
            val process = Runtime.getRuntime().exec("getprop")
            val reader = BufferedReader(InputStreamReader(process.inputStream))
            var line: String?
            
            while (reader.readLine().also { line = it } != null) {
                line?.let {
                    if ((it.contains("ro.debuggable=1") || it.contains("ro.secure=0"))) {
                        return true
                    }
                }
            }
            
            reader.close()
            process.destroy()
        } catch (e: Exception) {
            return false
        }
        
        return false
    }
} 