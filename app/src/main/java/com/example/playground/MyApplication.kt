package com.example.playground

import android.app.Application
import com.example.playground.network.AIImageService

class MyApplication : Application() {
    
    override fun onCreate() {
        super.onCreate()
        
        // 在应用启动时进行证书验证，如果有问题则使应用纯色显示且不可交互
        AIImageService.checkCertificateAndSecure(this)
    }
} 