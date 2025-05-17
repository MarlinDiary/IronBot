package com.example.playground.network

import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.OutputStreamWriter
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.TimeUnit

class AIImageService {
    companion object {
        private const val TAG = "AIImageService"
        private const val BASE_URL = "https://ai.elliottwen.info"
        private const val AUTH_ENDPOINT = "$BASE_URL/auth"
        private const val GENERATE_IMAGE_ENDPOINT = "$BASE_URL/generate_image"
        
        // This would typically be stored in a more secure way
        private const val AUTHORIZATION_HEADER = "c0957e34a11786192e8819a7d4faef725c3a0becf05716823b30e37111196e92ba1953a695dddd761cce8abbffefce40da8059d06aa651a02f9cc3322a7d1e0b"
    }
    
    /**
     * Fetches a signature from the authentication endpoint.
     * @return The signature string or null if the request failed
     */
    private suspend fun getSignature(): String? = withContext(Dispatchers.IO) {
        try {
            Log.d(TAG, "Getting signature from: $AUTH_ENDPOINT")
            val url = URL(AUTH_ENDPOINT)
            val connection = url.openConnection() as HttpURLConnection
            connection.requestMethod = "POST"
            connection.setRequestProperty("Authorization", AUTHORIZATION_HEADER)
            connection.connectTimeout = TimeUnit.SECONDS.toMillis(10).toInt()
            connection.readTimeout = TimeUnit.SECONDS.toMillis(10).toInt()
            
            Log.d(TAG, "Opening connection to auth endpoint...")
            
            val responseCode = connection.responseCode
            Log.d(TAG, "Auth response code: $responseCode")
            
            if (responseCode == HttpURLConnection.HTTP_OK) {
                val response = connection.inputStream.bufferedReader().use { it.readText() }
                Log.d(TAG, "Auth response: $response")
                
                val jsonObject = JSONObject(response)
                val signature = jsonObject.optString("signature", null)
                Log.d(TAG, "Signature obtained: ${signature?.take(10)}...")
                return@withContext signature
            } else {
                val errorResponse = connection.errorStream?.bufferedReader()?.use { it.readText() } ?: "No error response"
                Log.e(TAG, "Auth request failed with response code: $responseCode, Error: $errorResponse")
                return@withContext null
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error during authentication", e)
            return@withContext null
        }
    }
    
    /**
     * Generates an image based on the provided prompt by calling the API.
     * @param prompt The text prompt to generate the image from
     * @return The URL of the generated image, or null if the request failed
     */
    suspend fun generateImage(prompt: String): String? {
        // First get a signature
        Log.d(TAG, "Starting image generation for prompt: $prompt")
        val signature = getSignature()
        
        if (signature == null) {
            Log.e(TAG, "Failed to get signature, aborting image generation")
            return null
        }
        
        return withContext(Dispatchers.IO) {
            try {
                Log.d(TAG, "Calling generate image endpoint with signature")
                val url = URL(GENERATE_IMAGE_ENDPOINT)
                val connection = url.openConnection() as HttpURLConnection
                connection.requestMethod = "POST"
                connection.setRequestProperty("Authorization", AUTHORIZATION_HEADER)
                connection.setRequestProperty("Content-Type", "application/json")
                connection.doOutput = true
                connection.connectTimeout = TimeUnit.SECONDS.toMillis(30).toInt()
                connection.readTimeout = TimeUnit.SECONDS.toMillis(30).toInt()
                
                // Create JSON request body
                val requestBody = JSONObject().apply {
                    put("signature", signature)
                    put("prompt", prompt)
                }.toString()
                
                Log.d(TAG, "Request body: $requestBody")
                
                // Write request body
                OutputStreamWriter(connection.outputStream).use { it.write(requestBody) }
                
                val responseCode = connection.responseCode
                Log.d(TAG, "Generate image response code: $responseCode")
                
                if (responseCode == HttpURLConnection.HTTP_OK) {
                    val responseText = connection.inputStream.bufferedReader().use { it.readText() }
                    Log.d(TAG, "Generate image raw response: $responseText")
                    
                    // 关键修复：移除JSON响应中的引号
                    // 服务器返回的是JSON字符串，例如"images/path.jpg"
                    // 我们需要移除引号以获取实际路径
                    val cleanPath = responseText.trim().removeSurrounding("\"")
                    Log.d(TAG, "Cleaned path: $cleanPath")
                    
                    // Construct the full image URL
                    val fullImageUrl = if (cleanPath.startsWith("http")) {
                        // If the response is already a full URL
                        cleanPath
                    } else if (cleanPath.startsWith("/")) {
                        // If the response is a path starting with /
                        BASE_URL + cleanPath
                    } else {
                        // Otherwise assume it's a relative path
                        "$BASE_URL/$cleanPath"
                    }
                    
                    Log.d(TAG, "Constructed image URL: $fullImageUrl")
                    return@withContext fullImageUrl
                } else {
                    val errorResponse = connection.errorStream?.bufferedReader()?.use { it.readText() } ?: "No error response"
                    Log.e(TAG, "Image generation failed with response code: $responseCode, Error: $errorResponse")
                    return@withContext null
                }
            } catch (e: Exception) {
                Log.e(TAG, "Error during image generation", e)
                return@withContext null
            }
        }
    }
    
    /**
     * Interface for handling image generation callbacks
     */
    interface ImageGenerationCallback {
        fun onImageGenerated(imageUrl: String?)
        fun onError(message: String)
    }
} 