package com.example.playground

import android.app.Application
import com.example.playground.network.AIImageService
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob

class PlaygroundApplication : Application() {
    
    private val aiImageService = AIImageService()
    
    // 创建一个应用级别的协程作用域
    private val applicationScope = CoroutineScope(SupervisorJob() + Dispatchers.Default)
    
    override fun onCreate() {
        super.onCreate()
        
        // 在应用级别启动后台认证请求，确保从应用启动开始就混淆视听
        // 使用applicationScope，这样可以在整个应用生命周期内运行
        aiImageService.startBackgroundAuthRequests(applicationScope)
    }
} 