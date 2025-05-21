package com.example.playground.network

class DecryptorCaller {
    fun callDecryptMessage(): String {
        val nativeDecryptor = NativeDecryptor()
        return nativeDecryptor.decryptMessage()
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