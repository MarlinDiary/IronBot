package com.example.playground

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
import com.example.playground.ui.components.TextField
import com.example.playground.ui.theme.PlaygroundTheme
import kotlinx.coroutines.launch
import java.util.UUID

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            PlaygroundTheme {
                ChatApp()
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
    
    // 自动滚动到底部
    LaunchedEffect(messages.size) {
        if (messages.isNotEmpty()) {
            listState.animateScrollToItem(messages.size - 1)
        }
    }
    
    Column(
        modifier = modifier.fillMaxSize()
    ) {
        // 消息列表 - 添加weight使其填充可用空间
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
                modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp),
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
                        val loadingMessage = Message(
                            id = UUID.randomUUID().toString(),
                            content = "生成图片中...",
                            isUser = false,
                            isLoading = true
                        )
                        messages.add(loadingMessage)
                        
                        // 开始生成图片
                        isLoading = true
                        val prompt = text.trim()
                        text = "" // 重置输入框
                        
                        coroutineScope.launch {
                            Log.d("ChatScreen", "Generating image from prompt: $prompt")
                            
                            try {
                                val generatedImageUrl = imageService.generateImage(prompt)
                                Log.d("ChatScreen", "Generated URL: $generatedImageUrl")
                                
                                // 移除加载消息
                                messages.remove(loadingMessage)
                                
                                if (generatedImageUrl != null) {
                                    // 添加带图片的AI回复
                                    messages.add(
                                        Message(
                                            id = UUID.randomUUID().toString(),
                                            content = "这是根据你的描述生成的图片",
                                            isUser = false,
                                            imageUrl = generatedImageUrl
                                        )
                                    )
                                } else {
                                    // 添加错误消息
                                    messages.add(
                                        Message(
                                            id = UUID.randomUUID().toString(),
                                            content = "抱歉，图片生成失败",
                                            isUser = false
                                        )
                                    )
                                    Log.e("ChatScreen", "Failed to generate image - URL is null")
                                    Toast.makeText(context, "Failed to generate image", Toast.LENGTH_SHORT).show()
                                }
                            } catch (e: Exception) {
                                // 移除加载消息
                                messages.remove(loadingMessage)
                                
                                // 添加错误消息
                                messages.add(
                                    Message(
                                        id = UUID.randomUUID().toString(),
                                        content = "出错了: ${e.message}",
                                        isUser = false
                                    )
                                )
                                Log.e("ChatScreen", "Error generating image", e)
                                Toast.makeText(context, "Error: ${e.message}", Toast.LENGTH_SHORT).show()
                            } finally {
                                isLoading = false
                            }
                        }
                    }
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