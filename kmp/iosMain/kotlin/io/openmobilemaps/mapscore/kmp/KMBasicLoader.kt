package io.openmobilemaps.mapscore.kmp

import swiftPMImport.io.openmobilemaps.mapscore.kmp.MCTextureLoader

actual class KMBasicLoader actual constructor() : KMLoaderInterface {
    private val delegate = MCTextureLoader()

    actual override fun loadTexture(url: String, etag: String?): KMTextureLoaderResult =
        delegate.loadTexture(url, etag).asKmp()

    actual override fun loadData(url: String, etag: String?): KMDataLoaderResult =
        delegate.loadData(url, etag).asKmp()

    actual override fun loadTextureAsync(url: String, etag: String?): KMFuture<KMTextureLoaderResult> =
        delegate.loadTextureAsync(url, etag).asKmp()

    actual override fun loadDataAsync(url: String, etag: String?): KMFuture<KMDataLoaderResult> =
        delegate.loadDataAsync(url, etag).asKmp()

    actual override fun cancel(url: String) {
        delegate.cancel(url)
    }
}
