package com.example.playground.ui.components

import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.playground.ui.theme.PlaygroundTheme
import kotlinx.coroutines.delay
import androidx.compose.foundation.Image
import androidx.compose.ui.res.painterResource
import coil.compose.AsyncImage
import coil.request.ImageRequest
import com.example.playground.R

@Composable
fun ImageView(
    imageUrl: String?,
    modifier: Modifier = Modifier,
    isLoading: Boolean = false,
    placeholderText: String = "Describe an image"
) {
    val borderColor = Color(0xFFE2E2E1)
    val cornerRadius = 12.dp
    val loadingEmojis = remember { listOf("🔍", "🖼️", "✨", "🎨", "🧩") }
    var currentEmojiIndex by remember { mutableIntStateOf(0) }
    
    // Emoji animation effect
    LaunchedEffect(isLoading) {
        if (isLoading) {
            while (true) {
                delay(500) // 500ms delay between emoji changes
                currentEmojiIndex = (currentEmojiIndex + 1) % loadingEmojis.size
            }
        }
    }

    Box(
        modifier = modifier
            .fillMaxWidth()
            .aspectRatio(1.33f) // 4:3 aspect ratio
            .clip(RoundedCornerShape(cornerRadius)),
        contentAlignment = Alignment.Center
    ) {
        when {
            isLoading -> {
                // Loading state with animated emojis
                Text(
                    text = loadingEmojis[currentEmojiIndex],
                    fontSize = 48.sp,
                    textAlign = TextAlign.Center
                )
            }
            imageUrl != null -> {
                // Image loaded state
                AsyncImage(
                    model = ImageRequest.Builder(LocalContext.current)
                        .data(imageUrl)
                        .crossfade(true)
                        .build(),
                    contentDescription = "Generated image",
                    contentScale = ContentScale.Crop,
                    modifier = Modifier
                        .fillMaxWidth()
                        .aspectRatio(1.33f)
                        .border(
                            width = 1.dp,
                            color = borderColor,
                            shape = RoundedCornerShape(cornerRadius)
                        )
                )
            }
            else -> {
                // Placeholder state
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .aspectRatio(1.33f),
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = placeholderText,
                        color = Color(0xFF7D7D7D),
                        textAlign = TextAlign.Center
                    )
                }
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun ImageViewPlaceholderPreview() {
    PlaygroundTheme {
        ImageView(
            imageUrl = null,
            modifier = Modifier.padding(16.dp)
        )
    }
}

@Preview(showBackground = true)
@Composable
fun ImageViewLoadingPreview() {
    PlaygroundTheme {
        ImageView(
            imageUrl = null,
            isLoading = true,
            modifier = Modifier.padding(16.dp)
        )
    }
}

@Preview(showBackground = true)
@Composable
fun ImageViewLoadedPreview() {
    PlaygroundTheme {
        // Local image for preview
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp)
        ) {
            Image(
                painter = painterResource(id = R.drawable.ic_launcher_background),
                contentDescription = "Sample image",
                contentScale = ContentScale.Crop,
                modifier = Modifier
                    .fillMaxWidth()
                    .aspectRatio(1.33f)
                    .clip(RoundedCornerShape(12.dp))
                    .border(
                        width = 1.dp,
                        color = Color(0xFFE2E2E1),
                        shape = RoundedCornerShape(12.dp)
                    )
            )
        }
    }
} 