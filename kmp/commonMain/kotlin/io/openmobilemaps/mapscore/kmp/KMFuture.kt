package io.openmobilemaps.mapscore.kmp

expect class KMFuture<T>

expect class KMPromise<T>() {
    fun setValue(value: T)
    fun future(): KMFuture<T>
}
