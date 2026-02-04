package io.openmobilemaps.mapscore.kmp

actual typealias KMFuture<T> = com.snapchat.djinni.Future<T>

internal fun <T> KMFuture<T>.asPlatform(): com.snapchat.djinni.Future<T> = this
internal fun <T> com.snapchat.djinni.Future<T>.asKmp(): KMFuture<T> = this
