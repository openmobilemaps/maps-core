package io.openmobilemaps.mapscore.map.util;

import java.awt.image.BufferedImage;
import java.util.ArrayList;

import io.openmobilemaps.mapscore.shared.map.coordinates.Coord;
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateConversionHelperInterface;
import io.openmobilemaps.mapscore.shared.map.coordinates.RectCoord;
import io.openmobilemaps.mapscore.shared.map.layers.tiled.DefaultTiled2dMapLayerConfigs;
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapZoomLevelInfo;

/**
 * Off-screen tile renderer for an openmobilemaps Map.
 */
public class MapTileRenderer {
  private ArrayList<Tiled2dMapZoomLevelInfo> zoomLevelInfos;
  private OffscreenMapRenderer renderer;
  private CoordinateConversionHelperInterface converter;

  public record TileRange(int zoomLevel, int minColumn, int maxColumn, int minRow, int maxRow) {
  }

  /**
   * Create tile renderer for map with default web-mercator (EPSG:3857) tile
   * pyramid.
   */
  public MapTileRenderer(OffscreenMapRenderer renderer) {
    this(renderer,
        DefaultTiled2dMapLayerConfigs.webMercator("unusedLayerName", "unusedTileUrlPattern").getZoomLevelInfos());
  }

  public MapTileRenderer(OffscreenMapRenderer renderer, ArrayList<Tiled2dMapZoomLevelInfo> zoomLevelInfos) {
    this.renderer = renderer;
    this.zoomLevelInfos = zoomLevelInfos;
    this.converter = renderer.getMap().getCoordinateConverterHelper();
      
    this.renderer.setFramebufferSize(2*256, 2*256, 4);
  }

  /**
   * Determine the range of tiles necessary to cover the given bounds.
   */
  public TileRange getTileRange(int zoomLevel, RectCoord bbox) {
    // See H.1 "From BBOX to tile indices" in "OpenGIS® Web Map Tile Service
    // Implementation Standard" (OGC 07-057r7)
    final var m = zoomLevelInfos.get(zoomLevel); // m for tile _m_atrix.
   
    bbox = converter.convertRect(m.getBounds().getTopLeft().getSystemIdentifier(), bbox);

    final double tileSpan = m.getTileWidthLayerSystemUnits(); // == world-size / numTiles
    final double tileMatrixMinX = m.getBounds().getTopLeft().getX();
    final double tileMatrixMaxY = m.getBounds().getTopLeft().getY();

    final double epsilon = 1e-6;
    final int minCol = (int) Math.floor((bbox.getTopLeft().getX() - tileMatrixMinX) / tileSpan + epsilon);
    final int maxCol = (int) Math.floor((bbox.getBottomRight().getX() - tileMatrixMinX) / tileSpan - epsilon);
    final int minRow = (int) Math.floor((tileMatrixMaxY - bbox.getTopLeft().getY()) / tileSpan + epsilon);
    final int maxRow = (int) Math.floor((tileMatrixMaxY - bbox.getBottomRight().getY()) / tileSpan - epsilon);
    
    return new TileRange(zoomLevel, minCol, maxCol, minRow, maxRow);
  }

  public RectCoord getTileBBox(int zoomLevel, int xcol, int yrow) {
    // See H.2 "From tile indices to BBOX" in "OpenGIS® Web Map Tile Service
    // Implementation Standard" (OGC 07-057r7)
    final var m = zoomLevelInfos.get(zoomLevel); // m for tile _m_atrix.
    if (m.getZoomLevelIdentifier() != zoomLevel) {
      throw new AssertionError("zoomLevel inconsistent");
    }
    final double tileSpan = m.getTileWidthLayerSystemUnits(); // == world-size / numTiles
    final double tileMatrixMinX = m.getBounds().getTopLeft().getX();
    final double tileMatrixMaxY = m.getBounds().getTopLeft().getY();

    final double leftX = xcol * tileSpan + tileMatrixMinX;
    final double rightX = (xcol + 1) * tileSpan + tileMatrixMinX;
    final double upperY = tileMatrixMaxY - yrow * tileSpan;
    final double lowerY = tileMatrixMaxY - (yrow + 1) * tileSpan;

    final var csr = m.getBounds().getTopLeft().getSystemIdentifier();
    return new RectCoord(
        new Coord(csr, leftX, upperY, 0.0),
        new Coord(csr, rightX, lowerY, 0.0));
  }

  public BufferedImage renderTile(int zoomLevel, int xcol, int yrow) throws OffscreenMapRenderer.MapLayerException {
    var cam = renderer.getMap().getCamera();
    cam.freeze(false);

    var tile = getTileBBox(zoomLevel, xcol, yrow);
    cam.moveToBoundingBox(tile, 0.0f, false, null, null);
    // TODO: this drawFrame calls update() on camera/layer for each tile separately. Is this safe or will it cause issues at tile borders?
    return renderer.drawFrame();
  }

}
