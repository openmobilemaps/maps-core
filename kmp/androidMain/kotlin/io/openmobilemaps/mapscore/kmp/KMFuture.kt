package io.openmobilemaps.mapscore.kmp

import com.snapchat.djinni.Promise

actual class KMFuture<T> {
    internal val native: com.snapchat.djinni.Future<*>?

    constructor() {
        native = null
    }

    internal constructor(native: com.snapchat.djinni.Future<*>) {
        this.native = native
    }
}

actual class KMPromise<T> {
    private val promise: Promise<T> = Promise()

    actual fun setValue(value: T) {
        promise.setValue(value)
    }

    actual fun future(): KMFuture<T> = promise.future.asKmp()
}

@Suppress("UNCHECKED_CAST")
internal fun <TKmp, TPlatform> KMFuture<TKmp>.asPlatform(): com.snapchat.djinni.Future<TPlatform> =
    requireNotNull(native) as com.snapchat.djinni.Future<TPlatform>

fun <T> com.snapchat.djinni.Future<*>.asKmp(): KMFuture<T> = KMFuture(this)
