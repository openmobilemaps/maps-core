package io.openmobilemaps.mapscore.kmp

actual class KMFuture<T> {
    internal val native: com.snapchat.djinni.Future<*>?

    constructor() {
        native = null
    }

    internal constructor(native: com.snapchat.djinni.Future<*>) {
        this.native = native
    }
}

@Suppress("UNCHECKED_CAST")
internal fun <TKmp, TPlatform> KMFuture<TKmp>.asPlatform(): com.snapchat.djinni.Future<TPlatform> =
    requireNotNull(native) as com.snapchat.djinni.Future<TPlatform>

fun <T> com.snapchat.djinni.Future<*>.asKmp(): KMFuture<T> = KMFuture(this)
