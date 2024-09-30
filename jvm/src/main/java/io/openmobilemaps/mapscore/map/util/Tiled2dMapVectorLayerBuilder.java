package io.openmobilemaps.mapscore.map.util;

import io.openmobilemaps.mapscore.map.loader.DataLoader;
import io.openmobilemaps.mapscore.map.loader.FontLoader;
import io.openmobilemaps.mapscore.shared.map.MapInterface;
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerInterface;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface;

import java.util.ArrayList;

public class Tiled2dMapVectorLayerBuilder {
    /** Convenience helper to build a style.json layer with the default loaders. */
    public static Tiled2dMapVectorLayerInterface addFromStyleJson(
            MapInterface map,
            String layerName,
            String styleJsonUrl,
            ClassLoader classLoader,
            String fontDirectory) {
        return addFromStyleJson(map, layerName, styleJsonUrl, classLoader, fontDirectory, null);
    }

    /** Convenience helper to build a style.json layer with the default loaders. */
    public static Tiled2dMapVectorLayerInterface addFromStyleJson(
            MapInterface map,
            String layerName,
            String styleJsonUrl,
            ClassLoader classLoader,
            String fontDirectory,
            String fontFallbackName) {

        var loaders = new ArrayList<LoaderInterface>();
        loaders.add(new DataLoader());

        final float fontScalingDpi = map.getCamera().getScreenDensityPpi();
        var layer =
                Tiled2dMapVectorLayerInterface.createFromStyleJson(
                        layerName,
                        styleJsonUrl,
                        loaders,
                        new FontLoader(fontScalingDpi, classLoader, fontDirectory, fontFallbackName));

        map.addLayer(layer.asLayerInterface());
        return layer;
    }
}
