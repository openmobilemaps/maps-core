package io.openmobilemaps.mapscore.kmp

actual class KMFuture<T> {
    internal val native: MapCoreSharedModule.DJFuture?

    constructor() {
        native = null
    }

    internal constructor(native: MapCoreSharedModule.DJFuture) {
        this.native = native
    }
}

actual class KMPromise<T> {
    private val promise = MapCoreSharedModule.DJPromise()

    actual fun setValue(value: T) {
        promise.setValue(value)
    }

    fun setValue(value: KMDataLoaderResult) {
        promise.setValue(value.asPlatform())
    }

    fun setValue(value: KMTextureLoaderResult) {
        promise.setValue(value.asPlatform())
    }

    actual fun setDataLoaderResult(value: KMDataLoaderResult) {
        setValue(value)
    }

    actual fun setTextureLoaderResult(value: KMTextureLoaderResult) {
        setValue(value)
    }

    actual fun future(): KMFuture<T> = promise.getFuture().asKmp()
}

internal fun <T> KMFuture<T>.asPlatform(): MapCoreSharedModule.DJFuture =
    requireNotNull(native)

internal fun <T> MapCoreSharedModule.DJFuture.asKmp(): KMFuture<T> = KMFuture(this)
