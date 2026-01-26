package io.openmobilemaps.mapscore.kmp.feature.map.interop

import android.content.Context
import androidx.lifecycle.Lifecycle
import io.openmobilemaps.gps.GpsLayer
import io.openmobilemaps.gps.GpsProviderType
import io.openmobilemaps.gps.style.GpsStyleInfoFactory
import io.openmobilemaps.mapscore.map.layers.TiledRasterLayer
import io.openmobilemaps.mapscore.map.loader.DataLoader
import io.openmobilemaps.mapscore.map.loader.FontLoader
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerInterface as MapscoreVectorLayer
import kotlinx.coroutines.CoroutineScope
import java.io.File

actual abstract class MapFactory actual constructor(
	platformContext: Any?,
	coroutineScope: CoroutineScope?,
	lifecycle: Any?,
) {
	private val context = platformContext as? Context
	private val coroutineScope = coroutineScope
	private val lifecycle = lifecycle as? Lifecycle

	actual fun _createVectorLayer(
		layerName: String,
		dataProvider: MapDataProviderProtocol,
	): MapVectorLayer? {
		val context = requireNotNull(context) { "MapFactory requires an Android Context" }
		val coroutineScope = requireNotNull(coroutineScope) { "MapFactory requires a CoroutineScope" }
		val cacheDir = File(context.cacheDir, "vector").apply { mkdirs() }
		val provider = MapDataProviderLocalDataProviderImplementation(
			dataProvider = dataProvider,
			coroutineScope = coroutineScope,
		)
		return MapscoreVectorLayer.createExplicitly(
			layerName,
			null,
			false,
			arrayListOf(DataLoader(context, cacheDir, 50L * 1024 * 1024)),
			FontLoader(context, "map/fonts/", context.resources.displayMetrics.density),
			provider,
			null,
			null,
			null,
		)?.let { MapVectorLayerImpl(it) }
	}

	actual fun _createRasterLayer(config: MapTiled2dMapLayerConfig): MapRasterLayer? {
		val context = requireNotNull(context) { "MapFactory requires an Android Context" }
		val cacheDir = File(context.cacheDir, "raster").apply { mkdirs() }
		val loader = DataLoader(context, cacheDir, 25L * 1024 * 1024)
		return TiledRasterLayer(MapTiled2dMapLayerConfigImplementation(config), arrayListOf(loader))
			.let { MapRasterLayerImpl(it) }
	}

	actual fun _createGpsLayer(): MapGpsLayer? {
		val context = requireNotNull(context) { "MapFactory requires an Android Context" }
		val locationProvider = GpsProviderType.GOOGLE_FUSED.getProvider(context)
		val gpsLayer = GpsLayer(
			context = context,
			style = GpsStyleInfoFactory.createDefaultStyle(context),
			initialLocationProvider = locationProvider,
		).apply {
			setHeadingEnabled(false)
			setFollowInitializeZoom(25_000f)
			lifecycle?.let { registerLifecycle(it) }
		}
		return MapGpsLayerImpl(GpsLayerHandle(gpsLayer, locationProvider))
	}

	actual companion object {
		actual fun create(
			platformContext: Any?,
			coroutineScope: CoroutineScope?,
			lifecycle: Any?,
		): MapFactory = MapFactoryImpl(platformContext, coroutineScope, lifecycle)
	}
}

private class MapFactoryImpl(
	platformContext: Any?,
	coroutineScope: CoroutineScope?,
	lifecycle: Any?,
) : MapFactory(platformContext, coroutineScope, lifecycle)
