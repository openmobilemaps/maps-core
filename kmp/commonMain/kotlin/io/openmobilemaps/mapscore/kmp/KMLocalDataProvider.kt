package io.openmobilemaps.mapscore.kmp

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.IO
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking


open class KMLocalVectorLayer(
	open val styleFolderName: String,
	open val fileNameStyleJson: String = "style.json",
	open val spriteFolderName: String = "generated-sprites",
)

class KMLocalDataProvider(
	private val kmResources: KMResources,
	private val coroutineScope: CoroutineScope,
	private val layer: KMLocalVectorLayer,
	private val styleFolderRootPath: String = "files/map/layers/",
	private val ioDispatcher: CoroutineDispatcher = Dispatchers.IO,
) : KMTiled2dMapVectorLayerLocalDataProviderInterface {
	override fun getStyleJson(): String? {
		return runBlocking {
			// Currently enforced synchronous due to maps-core KMTiled2dMapVectorLayerLocalDataProviderInterface
			kmResources.readBytes("${styleFolderRootPath}${layer.styleFolderName}/${layer.fileNameStyleJson}").getOrNull()?.decodeToString()
		}
	}

	override fun loadSpriteAsync(spriteId: String, url: String, scale: Int): KMFuture<KMTextureLoaderResult> {
		val promise = KMPromise<KMTextureLoaderResult>()
		coroutineScope.launch(Dispatchers.Default) {
			val texture = kmResources.readBytes(
				getSpritePath(styleFolderRootPath, layer.spriteFolderName, spriteId, scale, ".png")
			).map { decodeTextureHolder(it) }.getOrNull()
			val result = texture?.let {
				KMTextureLoaderResult(data = it, etag = null, status = KMLoaderStatus.OK, errorCode = null)
			} ?: KMTextureLoaderResult(data = null, etag = null, status = KMLoaderStatus.ERROR_OTHER, errorCode = null)
			promise.setTextureLoaderResult(result)
		}
		return promise.future()
	}

	override fun loadSpriteJsonAsync(spriteId: String, url: String, scale: Int): KMFuture<KMDataLoaderResult> {
		val promise = KMPromise<KMDataLoaderResult>()
		coroutineScope.launch(Dispatchers.Default) {
			val dataRef = kmResources.readBytes(
				getSpritePath(styleFolderRootPath, layer.spriteFolderName, spriteId, scale, ".json")
			).getOrNull()?.toKMDataRef()
			val result = dataRef?.let {
				KMDataLoaderResult(data = it, etag = null, status = KMLoaderStatus.OK, errorCode = null)
			} ?: KMDataLoaderResult(data = null, etag = null, status = KMLoaderStatus.ERROR_OTHER, errorCode = null)
			promise.setDataLoaderResult(result)
		}
		return promise.future()
	}

	private fun getSpritePath(basePath: String, textureFolderName: String, spriteId: String, scale: Int, fileSuffix: String) =
		"${basePath}${textureFolderName}/${spriteId}@${scaleSuffix(scale)}$fileSuffix"

	private fun scaleSuffix(scale: Int): String = when (scale) {
		in 2..3 -> "@${scale}x"
		else -> ""
	}

	override fun loadGeojson(sourceName: String, url: String): KMFuture<KMDataLoaderResult> {
		val promise = KMPromise<KMDataLoaderResult>()
		coroutineScope.launch(Dispatchers.Default) {
			val dataRef = kmResources.readBytes("$styleFolderRootPath$url").getOrNull()?.toKMDataRef()
			val result = dataRef?.let {
				KMDataLoaderResult(data = it, etag = null, status = KMLoaderStatus.OK, errorCode = null)
			} ?: KMDataLoaderResult(data = null, etag = null, status = KMLoaderStatus.ERROR_404, errorCode = null)
			promise.setDataLoaderResult(result)
		}
		return promise.future()
	}
}