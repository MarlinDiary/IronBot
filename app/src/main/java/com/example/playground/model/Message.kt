package com.example.playground.model

data class Message(
    val id: String = System.currentTimeMillis().toString(),
    val content: String,
    val isUser: Boolean,
    val imageUrl: String? = null,
    val isLoading: Boolean = false
) 