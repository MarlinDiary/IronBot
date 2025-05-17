package com.example.playground

import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
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
    val context = LocalContext.current
    
    Column(
        modifier = modifier.padding(16.dp)
    ) {
        Text(text = "Image Generator")
        TextField(
            value = text,
            onValueChange = { text = it },
            modifier = Modifier.padding(top = 16.dp),
            onSend = {
                // TODO: Generate image from text
                Toast.makeText(context, "Generating image from: $text", Toast.LENGTH_SHORT).show()
                // Reset text after sending
                text = ""
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