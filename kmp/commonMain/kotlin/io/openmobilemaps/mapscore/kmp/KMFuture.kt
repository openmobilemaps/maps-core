package io.openmobilemaps.mapscore.kmp

expect class KMFuture<T>

expect class KMPromise<T>() {
    fun setValue(value: T)
    fun setDataLoaderResult(value: KMDataLoaderResult)
    fun setTextureLoaderResult(value: KMTextureLoaderResult)
    fun future(): KMFuture<T>
}
