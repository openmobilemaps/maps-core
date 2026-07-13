package io.openmobilemaps.mapscore.kmp

actual class KMFuture<T> {
    internal val native: swiftPMImport.io.openmobilemaps.mapscore.kmp.DJFuture?

    constructor() {
        native = null
    }

    internal constructor(native: swiftPMImport.io.openmobilemaps.mapscore.kmp.DJFuture) {
        this.native = native
    }
}

actual class KMPromise<T> {
    private val promise = swiftPMImport.io.openmobilemaps.mapscore.kmp.DJPromise()

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

public fun <T> KMFuture<T>.asPlatform(): swiftPMImport.io.openmobilemaps.mapscore.kmp.DJFuture =
    requireNotNull(native)

public fun <T> swiftPMImport.io.openmobilemaps.mapscore.kmp.DJFuture.asKmp(): KMFuture<T> = KMFuture(this)
