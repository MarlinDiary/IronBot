package com.example.playground

import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.ime
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material3.CenterAlignedTopAppBar
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.playground.model.Message
import com.example.playground.network.AIImageService
import com.example.playground.ui.components.ChatBubble
import com.example.playground.ui.components.EmptyChat
import com.example.playground.ui.components.TextField
import com.example.playground.ui.theme.PlaygroundTheme
import com.example.playground.utils.ImageDownloader
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import java.util.UUID

class MainActivity : ComponentActivity() {
    
    private val PERMISSION_REQUEST_CODE = 100
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            PlaygroundTheme {
                ChatApp()
            }
        }
    }
    
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this, "Storage permission granted. You can now download images", Toast.LENGTH_SHORT).show()
            } else {
                Toast.makeText(this, "Storage permission denied. Cannot download images", Toast.LENGTH_SHORT).show()
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ChatApp() {
    Scaffold(
        modifier = Modifier.fillMaxSize(),
        topBar = {
            CenterAlignedTopAppBar(
                title = {
                    Text(
                        text = "Playground",
                        style = MaterialTheme.typography.titleLarge,
                        textAlign = TextAlign.Center
                    )
                },
                colors = TopAppBarDefaults.centerAlignedTopAppBarColors(
                    containerColor = MaterialTheme.colorScheme.surface,
                    titleContentColor = MaterialTheme.colorScheme.onSurface
                )
            )
        },
        containerColor = MaterialTheme.colorScheme.background
    ) { innerPadding ->
        ChatScreen(modifier = Modifier.padding(innerPadding))
    }
}

@Composable
fun ChatScreen(modifier: Modifier = Modifier) {
    var text by remember { mutableStateOf("") }
    var isLoading by remember { mutableStateOf(false) }
    val messages = remember { mutableStateListOf<Message>() }
    val context = LocalContext.current
    val coroutineScope = rememberCoroutineScope()
    val imageService = remember { AIImageService() }
    val listState = rememberLazyListState()
    
    // 保存当前生成图片的Job引用，以便取消
    var currentGenerationJob by remember { mutableStateOf<Job?>(null) }
    // 保存当前加载消息的引用，以便在取消时移除
    var loadingMessage by remember { mutableStateOf<Message?>(null) }
    
    // 自动滚动到底部
    LaunchedEffect(messages.size) {
        if (messages.isNotEmpty()) {
            listState.animateScrollToItem(messages.size - 1)
        }
    }
    
    // 监听消息中的图片URL变化，确保图片加载后也滚动到底部
    LaunchedEffect(messages.map { it.imageUrl }) {
        if (messages.isNotEmpty()) {
            listState.animateScrollToItem(messages.size - 1)
        }
    }
    
    Column(
        modifier = modifier.fillMaxSize()
    ) {
        // 消息列表区域 - 添加weight使其填充可用空间
        if (messages.isEmpty()) {
            // 显示空屏占位
            EmptyChat(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxWidth()
            )
        } else {
            // 显示消息列表
            LazyColumn(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxWidth(),
                state = listState,
                contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp)
            ) {
                items(messages, key = { it.id }) { message ->
                    ChatBubble(
                        message = message.content,
                        isUser = message.isUser,
                        imageUrl = message.imageUrl,
                        modifier = Modifier.padding(vertical = 4.dp)
                    )
                }
            }
        }
        
        // 输入框区域 - 固定在底部并确保在键盘上方
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .background(MaterialTheme.colorScheme.surface)
                .imePadding() // 确保内容在键盘上方
        ) {
            TextField(
                value = text,
                onValueChange = { text = it },
                isLoading = isLoading,
                modifier = Modifier.padding(horizontal = 16.dp).padding(top = 8.dp),
                onSend = {
                    if (text.isNotEmpty()) {
                        // 添加用户消息
                        val userMessage = Message(
                            id = UUID.randomUUID().toString(),
                            content = text,
                            isUser = true
                        )
                        messages.add(userMessage)
                        
                        // 添加一个加载中的AI消息
                        val newLoadingMessage = Message(
                            id = UUID.randomUUID().toString(),
                            content = "Generating image...",
                            isUser = false,
                            isLoading = true
                        )
                        messages.add(newLoadingMessage)
                        loadingMessage = newLoadingMessage
                        
                        // 开始生成图片
                        isLoading = true
                        val prompt = text.trim()
                        text = "" // 重置输入框
                        
                        currentGenerationJob = coroutineScope.launch {
                            Log.d("ChatScreen", "Generating image from prompt: $prompt")
                            
                            try {
                                val generatedImageUrl = imageService.generateImage(prompt)
                                Log.d("ChatScreen", "Generated URL: $generatedImageUrl")
                                
                                // 移除加载消息
                                loadingMessage?.let { messages.remove(it) }
                                loadingMessage = null
                                
                                if (generatedImageUrl != null) {
                                    // 添加带图片的AI回复
                                    messages.add(
                                        Message(
                                            id = UUID.randomUUID().toString(),
                                            content = "Here is the image based on your description",
                                            isUser = false,
                                            imageUrl = generatedImageUrl
                                        )
                                    )
                                } else {
                                    // 添加错误消息
                                    messages.add(
                                        Message(
                                            id = UUID.randomUUID().toString(),
                                            content = "Sorry, image generation failed",
                                            isUser = false
                                        )
                                    )
                                    Log.e("ChatScreen", "Failed to generate image - URL is null")
                                    Toast.makeText(context, "Failed to generate image", Toast.LENGTH_SHORT).show()
                                }
                            } catch (e: CancellationException) {
                                // 处理取消操作 - 这里不需要做任何事情，因为UI已经在点击取消按钮时更新了
                                Log.d("ChatScreen", "Image generation was cancelled")
                            } catch (e: Exception) {
                                // 移除加载消息
                                loadingMessage?.let { messages.remove(it) }
                                loadingMessage = null
                                
                                // 添加错误消息
                                messages.add(
                                    Message(
                                        id = UUID.randomUUID().toString(),
                                        content = "Error: ${e.message}",
                                        isUser = false
                                    )
                                )
                                Log.e("ChatScreen", "Error generating image", e)
                                Toast.makeText(context, "Error: ${e.message}", Toast.LENGTH_SHORT).show()
                            } finally {
                                isLoading = false
                                currentGenerationJob = null
                            }
                        }
                    }
                },
                onCancel = {
                    // 乐观更新UI
                    // 1. 立即移除加载消息
                    loadingMessage?.let { messages.remove(it) }
                    
                    // 2. 立即添加取消消息
                    messages.add(
                        Message(
                            id = UUID.randomUUID().toString(),
                            content = "Image generation cancelled",
                            isUser = false
                        )
                    )
                    
                    // 3. 立即更新加载状态
                    isLoading = false
                    
                    // 4. 取消当前任务
                    currentGenerationJob?.cancel()
                    currentGenerationJob = null
                    loadingMessage = null
                    
                    Log.d("ChatScreen", "Image generation cancelled by user (optimistic update)")
                }
            )
        }
    }
}

@Preview(showBackground = true)
@Composable
fun ChatScreenPreview() {
    PlaygroundTheme {
        ChatApp()
    }
}