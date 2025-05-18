package com.example.playground.ui.components

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.playground.ui.theme.PlaygroundTheme

@Composable
fun ChatBubble(
    message: String,
    isUser: Boolean,
    modifier: Modifier = Modifier,
    imageUrl: String? = null
) {
    val alignment = if (isUser) Alignment.End else Alignment.Start
    
    Column(
        modifier = modifier.fillMaxWidth(),
        horizontalAlignment = alignment
    ) {
        // Text bubble using Material 3 Card
        Card(
            modifier = Modifier
                .widthIn(max = 280.dp),
            shape = RoundedCornerShape(
                topStart = if (isUser) 16.dp else 4.dp,
                topEnd = if (isUser) 4.dp else 16.dp,
                bottomStart = 16.dp,
                bottomEnd = 16.dp
            ),
            colors = CardDefaults.cardColors(
                containerColor = if (isUser) 
                    MaterialTheme.colorScheme.primary 
                else 
                    MaterialTheme.colorScheme.surfaceVariant
            )
        ) {
            Text(
                text = message,
                color = if (isUser) 
                    MaterialTheme.colorScheme.onPrimary 
                else 
                    MaterialTheme.colorScheme.onSurfaceVariant,
                style = MaterialTheme.typography.bodyLarge,
                modifier = Modifier.padding(12.dp)
            )
        }
        
        // If there's an image URL and it's not a user message, show the image
        if (!isUser && imageUrl != null) {
            Surface(
                modifier = Modifier
                    .padding(top = 8.dp)
                    .widthIn(max = 280.dp),
                shape = MaterialTheme.shapes.medium,
                shadowElevation = 0.dp
            ) {
                ImageView(
                    imageUrl = imageUrl,
                    modifier = Modifier
                )
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun UserChatBubblePreview() {
    PlaygroundTheme {
        ChatBubble(
            message = "Can you generate an image of a sunset over mountains?",
            isUser = true,
            modifier = Modifier.padding(8.dp)
        )
    }
}

@Preview(showBackground = true)
@Composable
fun AIChatBubblePreview() {
    PlaygroundTheme {
        ChatBubble(
            message = "Here's your image of a sunset over mountains",
            isUser = false,
            modifier = Modifier.padding(8.dp),
            imageUrl = null
        )
    }
}

@Preview(showBackground = true)
@Composable
fun AIChatBubbleWithImagePreview() {
    PlaygroundTheme {
        ChatBubble(
            message = "Here's your image of a sunset over mountains",
            isUser = false,
            modifier = Modifier.padding(8.dp),
            imageUrl = "https://example.com/image.jpg" // Dummy URL for preview
        )
    }
} 