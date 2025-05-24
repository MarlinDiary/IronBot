package com.example.playground.network

/**
 * 该类作为JNI和Kotlin之间的桥梁，用于获取API密钥和发送图像生成请求
 */
class ApiKeyCombiner {
    companion object {
        init {
            try {
                // 我们使用api_key_combiner库，它是在CMakeLists.txt中定义的
                System.loadLibrary("api_key_combiner")
            } catch (e: UnsatisfiedLinkError) {
                e.printStackTrace()
            }
        }
    }

    /**
     * 调用C层实现的函数，该函数会组合API密钥，发送认证请求，获取signature，
     * 然后使用signature和prompt发送图像生成请求，最终返回图像URL
     * 
     * @param prompt 用户提供的文本提示词
     * @return 生成的图像URL
     */
    external fun combineApiKey(prompt: String): String
} 