package io.openmobilemaps.mapscore.kmp

class KMPromise<T> {
    private val promise = MapCoreSharedModule.DJPromise()

    fun setValue(value: Any?) {
        promise.setValue(value)
    }

    fun getFuture(): KMFuture<T> = promise.getFuture().asKmp()
}
