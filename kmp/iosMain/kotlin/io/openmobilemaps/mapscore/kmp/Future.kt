package io.openmobilemaps.mapscore.kmp

actual class KMFuture<T> {
    internal val native: MapCoreSharedModule.DJFuture?

    constructor() {
        native = null
    }

    constructor(native: MapCoreSharedModule.DJFuture) {
        this.native = native
    }
}

internal fun <T> KMFuture<T>.asPlatform(): MapCoreSharedModule.DJFuture =
    requireNotNull(native)

internal fun <T> MapCoreSharedModule.DJFuture.asKmp(): KMFuture<T> = KMFuture(this)
