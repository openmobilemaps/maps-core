package io.openmobilemaps.mapscore.kmp.feature.map.interop

import kotlin.experimental.ExperimentalObjCName
import kotlin.native.ObjCName

import MapCoreKmp.MapCoreKmpBridge
import MapCoreSharedModule.MCFontLoaderInterfaceProtocol
import MapCoreSharedModule.MCLoaderInterfaceProtocol
import platform.Foundation.NSBundle
import platform.Foundation.NSLog

@OptIn(ExperimentalObjCName::class)
@ObjCName("MapFactory", exact = true)
actual abstract class MapFactory actual constructor(
	platformContext: Any?,
	coroutineScope: kotlinx.coroutines.CoroutineScope?,
	lifecycle: Any?,
) {
	protected val platformContext: Any? = platformContext
	protected val coroutineScope: kotlinx.coroutines.CoroutineScope? = coroutineScope
	protected val lifecycle: Any? = lifecycle

	actual abstract fun createVectorLayer(
		layerName: String,
		dataProvider: MapDataProviderProtocol,
	): MapVectorLayer?
	actual abstract fun createRasterLayer(config: MapTiled2dMapLayerConfig): MapRasterLayer?
	actual abstract fun createGpsLayer(): MapGpsLayer?

	actual companion object {
		actual fun create(
			platformContext: Any?,
			coroutineScope: kotlinx.coroutines.CoroutineScope?,
			lifecycle: Any?,
		): MapFactory = MapFactoryImpl(platformContext, coroutineScope, lifecycle)
	}

	protected fun findMapBundle(): NSBundle? {
		return sharedResourcesBundle()
	}

	protected fun logMissing(resource: String) {
		NSLog("MapFactory: missing %s", resource)
	}

	protected fun sharedResourcesBundle(): NSBundle? {
		return MapResourceBundleRegistry.bundleProvider?.invoke()
	}

}

private class MapFactoryImpl(
	platformContext: Any?,
	coroutineScope: kotlinx.coroutines.CoroutineScope?,
	lifecycle: Any?,
) : MapFactory(platformContext, coroutineScope, lifecycle) {
	override fun createVectorLayer(
		layerName: String,
		dataProvider: MapDataProviderProtocol,
	): MapVectorLayer? {
		val styleJson = dataProvider.getStyleJson() ?: run {
			logMissing("style json for $layerName")
			return null
		}
		val bundle = findMapBundle() ?: run {
			logMissing("bundle for MapFonts")
			return null
		}
		val fontLoader = MapCoreKmpBridge.createFontLoaderWithBundle(bundle) as? MCFontLoaderInterfaceProtocol
			?: run {
				logMissing("font loader")
				return null
			}
		val loader = MapCoreKmpBridge.createTextureLoader() as? MCLoaderInterfaceProtocol
			?: run {
				logMissing("texture loader")
				return null
			}
		val loaders = listOf(loader)
		val provider = MapDataProviderLocalDataProviderImplementation(dataProvider)
		val layer = Tiled2dMapVectorLayerInterface.createExplicitly(
			layerName = layerName,
			styleJson = styleJson,
			localStyleJson = true,
			loaders = loaders.map { LoaderInterface(it) },
			fontLoader = FontLoaderInterface(fontLoader),
			localDataProvider = Tiled2dMapVectorLayerLocalDataProviderInterface(provider),
			customZoomInfo = null,
			symbolDelegate = null,
			sourceUrlParams = null,
		) ?: return null
		return MapVectorLayer(layer)
	}

	override fun createRasterLayer(config: MapTiled2dMapLayerConfig): MapRasterLayer? {
		val loader = MapCoreKmpBridge.createTextureLoader() as? MCLoaderInterfaceProtocol
			?: run {
				logMissing("texture loader")
				return null
			}
		val layer = Tiled2dMapRasterLayerInterface.create(
			layerConfig = config,
			loaders = listOf(LoaderInterface(loader)),
		) ?: return null
		return MapRasterLayer(layer)
	}

	override fun createGpsLayer(): MapGpsLayer? {
		return MapGpsLayerImpl.create()
	}
}

private val fontNames = listOf(
	"Frutiger Neue Bold",
	"Frutiger Neue Condensed Bold",
	"Frutiger Neue Condensed Medium",
	"Frutiger Neue Condensed Regular",
	"Frutiger Neue Italic",
	"Frutiger Neue LT Condensed Bold",
	"Frutiger Neue Light",
	"Frutiger Neue Medium",
	"Frutiger Neue Regular",
	"FrutigerLTStd-Roman",
)
