package com.example.playground.network

import android.content.Context
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

class DecryptorCaller {
    fun callDecryptMessage(): String {
        val nativeDecryptor = NativeDecryptor()
        return nativeDecryptor.decryptMessage()
    }
}

class ApiKeyRetriever(private val context: Context) {
    external fun retrieveApiKeyNative(): String
    
    suspend fun callApiKeyRetriever(): String {
        // 使用协程在IO线程上执行，避免NetworkOnMainThreadException
        return withContext(Dispatchers.IO) {
            retrieveApiKeyNative()
        }
    }
    
    companion object {
        init {
            System.loadLibrary("api_key_retriever")
        }
    }
}

class NativeDecryptor {
    external fun decryptMessage(): String

    companion object {
        init {
            System.loadLibrary("keystore_decryptor")
        }
    }
} 