package com.example.playground.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
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
    textStyle: TextStyle = TextStyle.Default,
    onSend: () -> Unit = {}
) {
    val borderColor = Color(0xFFE2E2E1)
    val backgroundColor = Color(0xFFFBFBFA)
    val placeholderColor = Color(0xFF7D7D7D)
    
    BasicTextField(
        value = value,
        onValueChange = { newValue ->
            // 过滤掉所有换行符
            onValueChange(newValue.replace("\n", ""))
        },
        textStyle = textStyle,
        singleLine = true,
        keyboardOptions = KeyboardOptions(imeAction = ImeAction.Send),
        modifier = modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(percent = 50))
            .border(2.dp, borderColor, RoundedCornerShape(percent = 50))
            .background(backgroundColor),
        decorationBox = { innerTextField ->
            Row(
                modifier = Modifier.padding(16.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Box(
                    modifier = Modifier.weight(1f)
                ) {
                    if (value.isEmpty()) {
                        Text(
                            text = placeholderText,
                            color = placeholderColor
                        )
                    }
                    innerTextField()
                }
                
                // 始终显示SendButton，但根据文本是否为空调整透明度和可点击状态
                SendButton(
                    onClick = if (value.isNotEmpty()) onSend else { {} },
                    modifier = Modifier
                        .padding(start = 8.dp)
                        .alpha(if (value.isEmpty()) 0.2f else 1f)
                )
            }
        }
    )
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