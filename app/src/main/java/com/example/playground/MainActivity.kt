package com.example.playground

import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.playground.ui.components.ImageView
import com.example.playground.ui.components.TextField
import com.example.playground.ui.theme.PlaygroundTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            PlaygroundTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    MainScreen(modifier = Modifier.padding(innerPadding))
                }
            }
        }
    }
}

@Composable
fun MainScreen(modifier: Modifier = Modifier) {
    var text by remember { mutableStateOf("") }
    var isLoading by remember { mutableStateOf(false) }
    var imageUrl by remember { mutableStateOf<String?>(null) }
    val context = LocalContext.current
    
    Column(
        modifier = modifier.padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(text = "Image Generator")
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // Image view component
        ImageView(
            imageUrl = imageUrl,
            isLoading = isLoading,
            modifier = Modifier.padding(vertical = 16.dp)
        )
        
        Spacer(modifier = Modifier.height(16.dp))
        
        // Text input field
        TextField(
            value = text,
            onValueChange = { text = it },
            isLoading = isLoading,
            onSend = {
                if (text.isNotEmpty()) {
                    // Start loading
                    isLoading = true
                    
                    // Simulate image generation with a delay
                    // In real app, this would be an API call to generate the image
                    Toast.makeText(context, "Generating image from: $text", Toast.LENGTH_SHORT).show()
                    
                    // Mock image URL - in real app this would come from the API
                    android.os.Handler().postDelayed({
                        // Set a sample image URL (this is just a placeholder)
                        imageUrl = "https://picsum.photos/seed/${text.hashCode()}/400/300"
                        isLoading = false
                    }, 2000) // 2 second delay to simulate API call
                    
                    // Reset text after sending
                    text = ""
                }
            }
        )
    }
}

@Preview(showBackground = true)
@Composable
fun MainScreenPreview() {
    PlaygroundTheme {
        MainScreen()
    }
}