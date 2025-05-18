package com.example.playground.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectVerticalDragGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Download
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import coil.compose.AsyncImage
import coil.request.ImageRequest
import com.example.playground.utils.ImageDownloader
import kotlinx.coroutines.launch
import android.widget.Toast
import android.app.Activity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.runtime.DisposableEffect
import android.Manifest
import android.os.Build
import kotlin.math.abs

@Composable
fun FullScreenImageViewer(
    imageUrl: String,
    onDismiss: () -> Unit
) {
    val context = LocalContext.current
    val coroutineScope = rememberCoroutineScope()
    
    // 用于处理上下滑动关闭的状态
    var offsetY by remember { mutableFloatStateOf(0f) }
    var isDragging by remember { mutableStateOf(false) }
    
    // 定义关闭阈值
    val dismissThreshold = 300f // 超过这个距离时关闭查看器
    
    // 背景颜色 - 使用Material 3的scrim颜色
    val backgroundColor = MaterialTheme.colorScheme.scrim.copy(alpha = 0.95f)
    
    // 使用Activity Result API处理权限请求
    val permissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        if (isGranted) {
            // 权限已授予，下载图片
            coroutineScope.launch {
                ImageDownloader.downloadImage(context, imageUrl)
            }
        } else {
            // 权限被拒绝
            Toast.makeText(
                context,
                "Storage permission is required to save images",
                Toast.LENGTH_SHORT
            ).show()
        }
    }
    
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(backgroundColor)
            .pointerInput(Unit) {
                detectVerticalDragGestures(
                    onDragStart = { isDragging = true },
                    onDragEnd = {
                        isDragging = false
                        // 如果拖动距离超过阈值，关闭查看器
                        if (abs(offsetY) > dismissThreshold) {
                            onDismiss()
                        }
                        offsetY = 0f
                    },
                    onDragCancel = {
                        isDragging = false
                        offsetY = 0f
                    },
                    onVerticalDrag = { _, dragAmount ->
                        offsetY += dragAmount
                    }
                )
            }
    ) {
        // 图片 - 不再应用透明度变化
        AsyncImage(
            model = ImageRequest.Builder(LocalContext.current)
                .data(imageUrl)
                .crossfade(true)
                .build(),
            contentDescription = "Full screen image",
            contentScale = ContentScale.Fit,
            modifier = Modifier.fillMaxSize()
        )
        
        // 顶部控制栏
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .statusBarsPadding()
                .padding(16.dp),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            // 左上角关闭按钮 - 使用Material 3风格
            IconButton(
                onClick = onDismiss,
                modifier = Modifier
                    .size(40.dp)
                    .clip(CircleShape)
                    .background(MaterialTheme.colorScheme.surfaceContainerHigh)
            ) {
                Icon(
                    imageVector = Icons.Default.Close,
                    contentDescription = "Close",
                    tint = MaterialTheme.colorScheme.onSurface
                )
            }
            
            // 右上角保存按钮 - 使用Material 3风格
            IconButton(
                onClick = {
                    // 检查并请求权限
                    if (ImageDownloader.checkStoragePermission(context)) {
                        // 已有权限，直接下载
                        coroutineScope.launch {
                            ImageDownloader.downloadImage(context, imageUrl)
                        }
                    } else {
                        // 根据API级别请求相应的权限
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                            permissionLauncher.launch(Manifest.permission.READ_MEDIA_IMAGES)
                        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                            // Android 11+, 使用Manifest.permission.MANAGE_EXTERNAL_STORAGE需要特殊处理
                            // 这里我们只使用Activity Result API处理基本权限
                            Toast.makeText(
                                context,
                                "Please grant storage permissions in settings",
                                Toast.LENGTH_SHORT
                            ).show()
                        } else {
                            permissionLauncher.launch(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                        }
                    }
                },
                modifier = Modifier
                    .size(40.dp)
                    .clip(CircleShape)
                    .background(MaterialTheme.colorScheme.surfaceContainerHigh)
            ) {
                Icon(
                    imageVector = Icons.Default.Download,
                    contentDescription = "Save",
                    tint = MaterialTheme.colorScheme.onSurface
                )
            }
        }
    }
} 