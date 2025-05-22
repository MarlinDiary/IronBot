package com.example.playground.util

/**
 * 通过JNI提供底层C实现的root检测功能
 */
class RootDetectorNative {
    companion object {
        // 加载native库
        init {
            try {
                System.loadLibrary("root_detector")
            } catch (e: UnsatisfiedLinkError) {
                // 记录错误但不抛出异常，允许应用继续运行
                e.printStackTrace()
            }
        }
    }

    /**
     * 使用JNI的C代码检测设备是否已被root
     * @return 如果设备已被root则返回true，否则返回false
     */
    external fun isDeviceRooted(): Boolean

    /**
     * 提供一个安全的包装方法，以防止JNI加载失败时崩溃
     * @return 如果设备已被root则返回true，否则返回false；如果JNI加载失败，则返回false
     */
    fun isSafeDeviceRooted(): Boolean {
        return try {
            isDeviceRooted()
        } catch (e: UnsatisfiedLinkError) {
            // JNI调用失败，返回false
            false
        } catch (e: Exception) {
            // 其他异常，返回false
            false
        }
    }
} 