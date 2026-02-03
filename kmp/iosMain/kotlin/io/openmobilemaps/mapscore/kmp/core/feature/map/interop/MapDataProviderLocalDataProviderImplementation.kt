package io.openmobilemaps.mapscore.kmp.feature.map.interop

import MapCoreSharedModule.MCDataLoaderResult
import MapCoreSharedModule.DJFuture
import MapCoreSharedModule.DJPromise
import MapCoreSharedModule.MCLoaderStatusOK
import MapCoreSharedModule.MCTextureLoaderResult
import MapCoreSharedModule.MCTextureHolderInterfaceProtocol
import MapCoreSharedModule.MCTiled2dMapVectorLayerLocalDataProviderInterfaceProtocol
import MapCoreKmp.MapCoreKmpBridge
import kotlinx.cinterop.addressOf
import kotlinx.cinterop.usePinned
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import platform.Foundation.NSData
import platform.Foundation.create
import platform.darwin.NSObject

internal class MapDataProviderLocalDataProviderImplementation(
	private val dataProvider: MapDataProviderProtocol,
) : NSObject(), MCTiled2dMapVectorLayerLocalDataProviderInterfaceProtocol {
	private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Default)

	override fun getStyleJson(): String? = dataProvider.getStyleJson()

	override fun loadGeojson(sourceName: String, url: String): DJFuture {
		val promise = DJPromise()
		scope.launch {
			val result = runCatching { dataProvider.loadGeojson(sourceName, url) }.getOrNull()
			val data = result?.toNSData()
			promise.setValue(MCDataLoaderResult(data = data, etag = null, status = MCLoaderStatusOK, errorCode = null))
		}
		return promise.getFuture()
	}

	override fun loadSpriteAsync(spriteId: String, url: String, scale: Int): DJFuture {
		val promise = DJPromise()
		scope.launch {
			val result = runCatching { dataProvider.loadSpriteAsync(spriteId, url, scale) }.getOrNull()
			val holder = result?.toNSData()
				?.let { MapCoreKmpBridge.createTextureHolderWithData(it) as? MCTextureHolderInterfaceProtocol }
			promise.setValue(MCTextureLoaderResult(data = holder, etag = null, status = MCLoaderStatusOK, errorCode = null))
		}
		return promise.getFuture()
	}

	override fun loadSpriteJsonAsync(spriteId: String, url: String, scale: Int): DJFuture {
		val promise = DJPromise()
		scope.launch {
			val result = runCatching { dataProvider.loadSpriteJsonAsync(spriteId, url, scale) }.getOrNull()
			val data = result?.toNSData()
			promise.setValue(MCDataLoaderResult(data = data, etag = null, status = MCLoaderStatusOK, errorCode = null))
		}
		return promise.getFuture()
	}
}

@OptIn(kotlinx.cinterop.BetaInteropApi::class)
private fun ByteArray.toNSData(): NSData {
	if (isEmpty()) return NSData()
	return usePinned { pinned ->
		NSData.create(bytes = pinned.addressOf(0), length = size.toULong())
	}
}
