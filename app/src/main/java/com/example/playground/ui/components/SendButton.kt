package com.example.playground.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Icon
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.playground.R
import com.example.playground.ui.theme.PlaygroundTheme

@Composable
fun SendButton(
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    isLoading: Boolean = false
) {
    Box(
        modifier = modifier
            .size(36.dp)
            .clip(CircleShape)
            .background(Color.Black)
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Icon(
            painter = painterResource(
                id = if (isLoading) R.drawable.square else R.drawable.arrow_up
            ),
            contentDescription = if (isLoading) "Loading" else "Send",
            tint = Color.White,
            modifier = Modifier.size(18.dp)
        )
    }
}

@Preview
@Composable
fun SendButtonPreview() {
    PlaygroundTheme {
        SendButton(onClick = {})
    }
}

@Preview
@Composable
fun SendButtonLoadingPreview() {
    PlaygroundTheme {
        SendButton(
            onClick = {},
            isLoading = true
        )
    }
} 