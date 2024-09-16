package io.openmobilemaps.mapscore.map.util;

import java.util.ArrayList;

import io.openmobilemaps.mapscore.map.loader.DataLoader;
import io.openmobilemaps.mapscore.map.loader.FontLoader;
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerInterface;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface;

public class Tiled2dMapVectorLayerBuilder {
  /**
   * Convenience helper to build a style.json layer with the default loaders.
   */
  public static Tiled2dMapVectorLayerInterface createFromStyleJson(
      String layerName,
      String styleJsonUrl,
      float fontScalingDpi) {
    var loaders = new ArrayList<LoaderInterface>();
    loaders.add(new DataLoader());
    return Tiled2dMapVectorLayerInterface.createFromStyleJson(layerName,
        styleJsonUrl,
        loaders,
        new FontLoader(fontScalingDpi));
  }
}
