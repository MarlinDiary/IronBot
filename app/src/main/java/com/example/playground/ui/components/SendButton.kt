package com.example.playground.ui.components

import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.FilledIconButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
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
    isLoading: Boolean = false,
    containerColor: Color = MaterialTheme.colorScheme.primary,
    contentColor: Color = MaterialTheme.colorScheme.onPrimary,
    onCancel: (() -> Unit)? = null
) {
    FilledIconButton(
        onClick = {
            if (isLoading && onCancel != null) {
                onCancel()
            } else if (!isLoading) {
                onClick()
            }
        },
        modifier = modifier.size(36.dp),
        shape = CircleShape,
        colors = IconButtonDefaults.filledIconButtonColors(
            containerColor = containerColor,
            contentColor = contentColor
        )
    ) {
        Icon(
            painter = painterResource(
                id = if (isLoading) R.drawable.square else R.drawable.arrow_up
            ),
            contentDescription = if (isLoading) "Cancel" else "Send",
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
            isLoading = true,
            onCancel = {}
        )
    }
}

@Preview
@Composable
fun SendButtonDisabledPreview() {
    PlaygroundTheme {
        SendButton(
            onClick = {},
            containerColor = MaterialTheme.colorScheme.surfaceVariant,
            contentColor = MaterialTheme.colorScheme.onSurfaceVariant
        )
    }
} 