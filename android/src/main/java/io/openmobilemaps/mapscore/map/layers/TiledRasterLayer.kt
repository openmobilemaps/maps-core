package io.openmobilemaps.mapscore.map.layers

import android.content.Context
import io.openmobilemaps.mapscore.map.loader.DataLoader
import io.openmobilemaps.mapscore.shared.map.layers.tiled.DefaultTiled2dMapLayerConfigs
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapLayerConfig
import io.openmobilemaps.mapscore.shared.map.layers.tiled.raster.Tiled2dMapRasterLayerInterface
import io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface
import java.util.UUID

class TiledRasterLayer(layerConfig: Tiled2dMapLayerConfig, loaders: ArrayList<LoaderInterface>) {

	/**
	 * Creates a default web-mercator (EPSG:3857) tiled raster layer with a custom LoaderInterface (e.g. a custom DataLoader)
	 *
	 * @param tileUrl Url to the tiles, formatted with placeholders. E.g. https://www.sample.org/{z}/{x}/{y}.png
	 * @param loader A loader interface for loading the layer tiles. E.g an instance of a DataLoader
	 * @param layerName Name of the created layer
	 */
	constructor(tileUrl: String, loader: LoaderInterface, layerName: String = UUID.randomUUID().toString()) : this(
		DefaultTiled2dMapLayerConfigs.webMercator(layerName, tileUrl), arrayListOf(loader)
	)

	/**
	 * Creates a default web-mercator (EPSG:3857) tiled raster layer with a default DataLoader.
	 *
	 * @param context Android contex used for the default DataLoader
	 * @param tileUrl Url to the tiles, formatted with placeholders. E.g. https://www.sample.org/{z}/{x}/{y}.png
	 * @param layerName Name of the created layer
	 */
	constructor(context: Context, tileUrl: String, layerName: String = UUID.randomUUID().toString()) : this(
		tileUrl, DataLoader(context, context.cacheDir, 50 * 1024 * 1024L), layerName
	)

	private val layer: Tiled2dMapRasterLayerInterface = Tiled2dMapRasterLayerInterface.create(layerConfig, loaders)

	fun layerInterface() = layer.asLayerInterface()
	fun rasterLayerInterface() = layer
}