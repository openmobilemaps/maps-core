package io.openmobilemaps.mapscore.kmp

actual abstract class KMDataRef internal constructor() {
    internal abstract fun asNSData(): platform.Foundation.NSData
}

private class KMDataRefPlatformWrapper(
    private val nativeHandle: platform.Foundation.NSData,
) : KMDataRef() {
    override fun asNSData(): platform.Foundation.NSData = nativeHandle
}

internal fun KMDataRef.asPlatform(): platform.Foundation.NSData = asNSData()
internal fun platform.Foundation.NSData.asKmp(): KMDataRef = KMDataRefPlatformWrapper(this)
fun platform.Foundation.NSData.toKMDataRef(): KMDataRef = KMDataRefPlatformWrapper(this)
