package io.openmobilemaps.mapscore.kmp

actual typealias KMDataRef = platform.Foundation.NSData

internal fun KMDataRef.asPlatform(): platform.Foundation.NSData = this
internal fun platform.Foundation.NSData.asKmp(): KMDataRef = this
