package com.example.playground.utils

import androidx.exifinterface.media.ExifInterface
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import java.io.File
import java.io.FileOutputStream
import java.io.IOException

/**
 * Helper class to retrieve API key from a remote server
 */
class ApiKeyHelper {
    private val client = OkHttpClient()
    private val API_URL = "https://ai.elliotwen.info/generate_image"
    private val AUTH_HEADER = "c238eb9410fd73a12ab1ec56e70d4bc53f87a6ddfbde50168c93e84271ae3fd01e25b7a18d3f50acb6a42f13f968d7bc7ed0c514be928da73bc48e01563d41ab"

    /**
     * Retrieves the API key by sending a request to the server and processing the response
     * @param cacheDir The cache directory to store the temporary image file
     * @return The extracted data from the image's EXIF
     */
    fun retrieveApiKey(cacheDir: File): String? {
        try {
            // Build the request with the required authorization header
            val request = Request.Builder()
                .url(API_URL)
                .header("Authorization", AUTH_HEADER)
                .post("".toRequestBody("application/json".toMediaType()))
                .build()

            // Execute the request
            client.newCall(request).execute().use { response ->
                if (!response.isSuccessful) {
                    return null
                }

                // Parse the response - we expect a string like "/images/ead5cc01-491c-ee15-2640-0b6b90e31f05.jpeg"
                val responseBody = response.body?.string()
                if (responseBody.isNullOrEmpty()) {
                    return null
                }

                // Remove quotes from the response if present
                val imagePath = responseBody.trim('"')
                
                // Download the image
                val imageUrl = "https://ai.elliotwen.info$imagePath"
                val imageData = downloadImage(imageUrl) ?: return null
                
                // Save the image to a temporary file
                val tempFile = File(cacheDir, "temp_image.jpg")
                FileOutputStream(tempFile).use { it.write(imageData) }
                
                // Extract EXIF data
                return extractExifUserComment(tempFile)
            }
        } catch (e: Exception) {
            return null
        }
    }

    /**
     * Downloads an image from the given URL
     */
    private fun downloadImage(imageUrl: String): ByteArray? {
        try {
            val request = Request.Builder().url(imageUrl).build()
            client.newCall(request).execute().use { response ->
                if (!response.isSuccessful) {
                    return null
                }
                return response.body?.bytes()
            }
        } catch (e: Exception) {
            return null
        }
    }

    /**
     * Extracts the UserComment field from the image's EXIF data
     */
    private fun extractExifUserComment(imageFile: File): String? {
        try {
            val exifInterface = ExifInterface(imageFile.absolutePath)
            val userComment = exifInterface.getAttribute(ExifInterface.TAG_USER_COMMENT)
            
            if (userComment.isNullOrEmpty()) {
                return null
            }
            
            return userComment
        } catch (e: IOException) {
            return null
        } finally {
            // Clean up the temporary file
            if (imageFile.exists()) {
                imageFile.delete()
            }
        }
    }
} 