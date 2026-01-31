package io.openmobilemaps.mapscore.kmp.feature.map.interop

import android.graphics.BitmapFactory
import com.snapchat.djinni.Future
import com.snapchat.djinni.Promise
import io.openmobilemaps.mapscore.graphics.BitmapTextureHolder
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerLocalDataProviderInterface as MapscoreLocalDataProvider
import io.openmobilemaps.mapscore.shared.map.loader.DataLoaderResult
import io.openmobilemaps.mapscore.shared.map.loader.LoaderStatus
import io.openmobilemaps.mapscore.shared.map.loader.TextureLoaderResult
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import java.nio.ByteBuffer

internal class MapDataProviderLocalDataProviderImplementation(
	private val dataProvider: MapDataProviderProtocol,
	private val coroutineScope: CoroutineScope,
) : MapscoreLocalDataProvider() {

	override fun getStyleJson(): String? = dataProvider.getStyleJson()

	override fun loadSpriteAsync(scale: Int): Future<TextureLoaderResult> {
		val promise = Promise<TextureLoaderResult>()
		coroutineScope.launch(Dispatchers.IO) {
			val attempt = runCatching { dataProvider.loadSpriteAsync("", "", scale) }
			val texture = attempt.getOrNull()
				?.let { bytes -> BitmapFactory.decodeByteArray(bytes, 0, bytes.size) }
				?.let { BitmapTextureHolder(it) }
			val result = if (attempt.isFailure) {
				TextureLoaderResult(null, null, LoaderStatus.ERROR_NETWORK, null)
			} else {
				texture?.let { TextureLoaderResult(it, null, LoaderStatus.OK, null) }
					?: TextureLoaderResult(null, null, LoaderStatus.ERROR_NETWORK, null)
			}
			promise.setValue(result)
		}
		return promise.future
	}

	override fun loadSpriteJsonAsync(scale: Int): Future<DataLoaderResult> {
		val promise = Promise<DataLoaderResult>()
		coroutineScope.launch(Dispatchers.IO) {
			val attempt = runCatching { dataProvider.loadSpriteJsonAsync("", "", scale) }
			val result = if (attempt.isFailure) {
				DataLoaderResult(null, null, LoaderStatus.ERROR_NETWORK, null)
			} else {
				attempt.getOrNull()
					?.let { bytes -> bytes.asDataLoaderResult() }
					?: DataLoaderResult(null, null, LoaderStatus.OK, null)
			}
			promise.setValue(result)
		}
		return promise.future
	}

	override fun loadGeojson(sourceName: String, url: String): Future<DataLoaderResult> {
		val promise = Promise<DataLoaderResult>()
		coroutineScope.launch(Dispatchers.IO) {
			val attempt = runCatching { dataProvider.loadGeojson(sourceName, url) }
			val result = if (attempt.isFailure) {
				DataLoaderResult(null, null, LoaderStatus.ERROR_NETWORK, null)
			} else {
				attempt.getOrNull()
					?.let { bytes -> bytes.asDataLoaderResult() }
					?: DataLoaderResult(null, null, LoaderStatus.OK, null)
			}
			promise.setValue(result)
		}
		return promise.future
	}

	private fun ByteArray.asDataLoaderResult(): DataLoaderResult {
		val buffer = ByteBuffer.allocateDirect(size).apply {
			put(this@asDataLoaderResult)
			flip()
		}
		return DataLoaderResult(buffer, null, LoaderStatus.OK, null)
	}
}
