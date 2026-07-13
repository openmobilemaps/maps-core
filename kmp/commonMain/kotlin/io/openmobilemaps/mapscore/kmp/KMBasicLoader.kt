package io.openmobilemaps.mapscore.kmp

/**
 * Simple fallback loader for validating a basic maps-core KMP setup.
 *
 * Prefer a project-specific [KMLoaderInterface] implementation that integrates with the
 * networking and loading architecture of the surrounding app.
 */
@Deprecated(
    message = "KMBasicLoader is a simple fallback loader intended for setup verification. " +
        "For production use, implement a project-specific KMLoaderInterface that integrates " +
        "with your app's networking and loading architecture.",
    level = DeprecationLevel.WARNING,
)
expect class KMBasicLoader() : KMLoaderInterface {
    override fun loadTexture(url: String, etag: String?): KMTextureLoaderResult

    override fun loadData(url: String, etag: String?): KMDataLoaderResult

    override fun loadTextureAsync(url: String, etag: String?): KMFuture<KMTextureLoaderResult>

    override fun loadDataAsync(url: String, etag: String?): KMFuture<KMDataLoaderResult>

    override fun cancel(url: String)
}
