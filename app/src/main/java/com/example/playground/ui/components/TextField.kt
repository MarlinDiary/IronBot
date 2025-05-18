package com.example.playground.ui.components

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.playground.ui.theme.PlaygroundTheme

@Composable
fun TextField(
    value: String,
    onValueChange: (String) -> Unit,
    modifier: Modifier = Modifier,
    placeholderText: String = "Describe an image",
    textStyle: TextStyle = MaterialTheme.typography.bodyLarge,
    onSend: () -> Unit = {},
    isLoading: Boolean = false,
    onCancel: () -> Unit = {}
) {
    val isButtonActive = isLoading || value.isNotEmpty()
    
    // 计算按钮颜色，使用更醒目的非活跃状态颜色
    val buttonContainerColor = if (isButtonActive) 
        MaterialTheme.colorScheme.primary
    else 
        MaterialTheme.colorScheme.secondary.copy(alpha = 0.9f)
        
    val buttonContentColor = if (isButtonActive)
        MaterialTheme.colorScheme.onPrimary
    else
        MaterialTheme.colorScheme.onSecondary

    Surface(
        modifier = modifier.fillMaxWidth(),
        shape = RoundedCornerShape(percent = 50),
        tonalElevation = 1.dp,
        color = MaterialTheme.colorScheme.surfaceVariant
    ) {
        BasicTextField(
            value = value,
            onValueChange = { newValue ->
                // 过滤掉所有换行符
                onValueChange(newValue.replace("\n", ""))
            },
            textStyle = textStyle.copy(color = MaterialTheme.colorScheme.onSurfaceVariant),
            singleLine = true,
            keyboardOptions = KeyboardOptions(imeAction = ImeAction.Send),
            keyboardActions = KeyboardActions(
                onSend = { if (value.isNotEmpty()) onSend() }
            ),
            cursorBrush = SolidColor(MaterialTheme.colorScheme.primary),
            decorationBox = { innerTextField ->
                Row(
                    modifier = Modifier.padding(horizontal = 16.dp, vertical = 12.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Box(
                        modifier = Modifier.weight(1f)
                    ) {
                        if (value.isEmpty()) {
                            Text(
                                text = placeholderText,
                                style = MaterialTheme.typography.bodyLarge,
                                color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.6f)
                            )
                        }
                        innerTextField()
                    }
                    
                    // 使用不同的颜色而不是透明度，并添加onCancel回调
                    SendButton(
                        onClick = if (isButtonActive && !isLoading) onSend else { {} },
                        modifier = Modifier.padding(start = 8.dp),
                        isLoading = isLoading,
                        containerColor = buttonContainerColor,
                        contentColor = buttonContentColor,
                        onCancel = onCancel
                    )
                }
            }
        )
    }
}

@Preview(showBackground = true)
@Composable
fun TextFieldPreview() {
    PlaygroundTheme {
        var text by remember { mutableStateOf("") }
        TextField(
            value = text,
            onValueChange = { text = it },
            modifier = Modifier.padding(16.dp),
            onSend = {}
        )
    }
}

@Preview(showBackground = true)
@Composable
fun TextFieldWithTextPreview() {
    PlaygroundTheme {
        var text by remember { mutableStateOf("Sample text") }
        TextField(
            value = text,
            onValueChange = { text = it },
            modifier = Modifier.padding(16.dp),
            onSend = {}
        )
    }
}

@Preview(showBackground = true)
@Composable
fun TextFieldLoadingPreview() {
    PlaygroundTheme {
        var text by remember { mutableStateOf("") }
        TextField(
            value = text,
            onValueChange = { text = it },
            modifier = Modifier.padding(16.dp),
            onSend = {},
            isLoading = true,
            onCancel = {}
        )
    }
} 