package com.example.playground.utils

import android.Manifest
import android.app.Activity
import android.content.ContentValues
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.MediaStore
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream
import java.io.OutputStream
import java.net.URL
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

object ImageDownloader {
    private const val PERMISSION_REQUEST_CODE = 100
    
    /**
     * Check storage permission
     */
    fun checkStoragePermission(context: Context): Boolean {
        return when {
            Build.VERSION.SDK_INT >= 34 -> {
                // Android 14+: Check for READ_MEDIA_VISUAL_USER_SELECTED permission
                ContextCompat.checkSelfPermission(
                    context,
                    Manifest.permission.READ_MEDIA_VISUAL_USER_SELECTED
                ) == PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(
                    context,
                    Manifest.permission.READ_MEDIA_IMAGES
                ) == PackageManager.PERMISSION_GRANTED
            }
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU -> {
                // Android 13: Check for READ_MEDIA_IMAGES permission
                ContextCompat.checkSelfPermission(
                    context,
                    Manifest.permission.READ_MEDIA_IMAGES
                ) == PackageManager.PERMISSION_GRANTED
            }
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.R -> {
                // Android 11-12: Doesn't require storage permission for MediaStore
                true
            }
            else -> {
                // Android 10 and below: Check for WRITE_EXTERNAL_STORAGE
                ContextCompat.checkSelfPermission(
                    context,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
                ) == PackageManager.PERMISSION_GRANTED
            }
        }
    }
    
    /**
     * Request storage permission
     */
    fun requestStoragePermission(activity: Activity) {
        when {
            Build.VERSION.SDK_INT >= 34 -> {
                // Android 14+: Request READ_MEDIA_VISUAL_USER_SELECTED permission
                ActivityCompat.requestPermissions(
                    activity,
                    arrayOf(
                        Manifest.permission.READ_MEDIA_VISUAL_USER_SELECTED,
                        Manifest.permission.READ_MEDIA_IMAGES
                    ),
                    PERMISSION_REQUEST_CODE
                )
            }
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU -> {
                // Android 13: Request READ_MEDIA_IMAGES permission
                ActivityCompat.requestPermissions(
                    activity,
                    arrayOf(Manifest.permission.READ_MEDIA_IMAGES),
                    PERMISSION_REQUEST_CODE
                )
            }
            Build.VERSION.SDK_INT < Build.VERSION_CODES.Q -> {
                // Android 9 and below: Request WRITE_EXTERNAL_STORAGE
                ActivityCompat.requestPermissions(
                    activity,
                    arrayOf(Manifest.permission.WRITE_EXTERNAL_STORAGE),
                    PERMISSION_REQUEST_CODE
                )
            }
        }
    }
    
    /**
     * Download and save image
     */
    suspend fun downloadImage(context: Context, imageUrl: String): Boolean {
        return try {
            withContext(Dispatchers.IO) {
                val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.getDefault()).format(Date())
                val filename = "IMG_$timestamp.jpg"
                
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    saveImageWithMediaStore(context, imageUrl, filename)
                } else {
                    saveImageLegacy(context, imageUrl, filename)
                }
            }
        } catch (e: Exception) {
            withContext(Dispatchers.Main) {
                Toast.makeText(context, "Failed to download image: ${e.message}", Toast.LENGTH_SHORT).show()
            }
            false
        }
    }
    
    /**
     * Save image using MediaStore (Android 10+)
     */
    private suspend fun saveImageWithMediaStore(context: Context, imageUrl: String, filename: String): Boolean {
        val contentValues = ContentValues().apply {
            put(MediaStore.Images.Media.DISPLAY_NAME, filename)
            put(MediaStore.Images.Media.MIME_TYPE, "image/jpeg")
            put(MediaStore.Images.Media.RELATIVE_PATH, Environment.DIRECTORY_PICTURES + "/Playground")
        }
        
        val contentResolver = context.contentResolver
        val uri = contentResolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, contentValues)
        
        return uri?.let { imageUri ->
            contentResolver.openOutputStream(imageUri)?.use { outputStream ->
                val bitmap = downloadBitmap(imageUrl)
                bitmap.compress(Bitmap.CompressFormat.JPEG, 100, outputStream)
                
                withContext(Dispatchers.Main) {
                    Toast.makeText(context, "Image saved to gallery", Toast.LENGTH_SHORT).show()
                }
                true
            } ?: false
        } ?: false
    }
    
    /**
     * Save image using legacy method (Android 9 and below)
     */
    private suspend fun saveImageLegacy(context: Context, imageUrl: String, filename: String): Boolean {
        val directory = File(
            Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES),
            "Playground"
        )
        
        if (!directory.exists()) {
            directory.mkdirs()
        }
        
        val file = File(directory, filename)
        var outputStream: OutputStream? = null
        
        try {
            outputStream = FileOutputStream(file)
            val bitmap = downloadBitmap(imageUrl)
            bitmap.compress(Bitmap.CompressFormat.JPEG, 100, outputStream)
            
            // Notify media library to update
            MediaStore.Images.Media.insertImage(
                context.contentResolver,
                file.absolutePath,
                file.name,
                null
            )
            
            withContext(Dispatchers.Main) {
                Toast.makeText(context, "Image saved to gallery", Toast.LENGTH_SHORT).show()
            }
            return true
        } finally {
            outputStream?.close()
        }
    }
    
    /**
     * Download bitmap from URL
     */
    private suspend fun downloadBitmap(imageUrl: String): Bitmap {
        return withContext(Dispatchers.IO) {
            val url = URL(imageUrl)
            val connection = url.openConnection()
            connection.connect()
            val inputStream = connection.getInputStream()
            android.graphics.BitmapFactory.decodeStream(inputStream)
        }
    }
} 