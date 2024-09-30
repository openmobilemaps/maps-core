package io.openmobilemaps.mapscore.map.util;

import io.openmobilemaps.mapscore.shared.map.LayerReadyState;
import io.openmobilemaps.mapscore.shared.map.coordinates.Coord;
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateConversionHelperInterface;
import io.openmobilemaps.mapscore.shared.map.coordinates.RectCoord;
import io.openmobilemaps.mapscore.shared.map.layers.tiled.DefaultTiled2dMapLayerConfigs;
import io.openmobilemaps.mapscore.shared.map.layers.tiled.Tiled2dMapZoomLevelInfo;

import java.awt.image.BufferedImage;
import java.time.Duration;
import java.util.ArrayList;

/** Off-screen tile renderer for an openmobilemaps Map. */
public class MapTileRenderer {
    private final ArrayList<Tiled2dMapZoomLevelInfo> zoomLevelInfos;
    private final OffscreenMapRenderer renderer;
    private final CoordinateConversionHelperInterface converter;

    /** Create tile renderer for map with default web-mercator (EPSG:3857) tile pyramid. */
    public MapTileRenderer(OffscreenMapRenderer renderer) {
        this(
                renderer,
                DefaultTiled2dMapLayerConfigs.webMercator("unusedLayerName", "unusedTileUrlPattern")
                        .getZoomLevelInfos());
    }

    public MapTileRenderer(
            OffscreenMapRenderer renderer, ArrayList<Tiled2dMapZoomLevelInfo> zoomLevelInfos) {
        this.renderer = renderer;
        this.zoomLevelInfos = zoomLevelInfos;
        this.converter = renderer.getMap().getCoordinateConverterHelper();

        this.renderer.setFramebufferSize(2 * 256, 2 * 256, 4);
    }

    /** Determine the range of tiles necessary to cover the given bounds. */
    public TileRange getTileRange(int zoomLevel, RectCoord bbox) {
        // See H.1 "From BBOX to tile indices" in "OpenGIS® Web Map Tile Service
        // Implementation Standard" (OGC 07-057r7)
        final var m = zoomLevelInfos.get(zoomLevel); // m for tile _m_atrix.

        final var bboxC =
                converter.convertRect(m.getBounds().getTopLeft().getSystemIdentifier(), bbox);
        final double bboxMinX = bboxC.getTopLeft().getX();
        final double bboxMaxX = bboxC.getBottomRight().getX();
        final double bboxMinY = bboxC.getBottomRight().getY();
        final double bboxMaxY = bboxC.getTopLeft().getY();

        final double tileSpan = m.getTileWidthLayerSystemUnits(); // == world-size / numTiles
        final double tileMatrixMinX = m.getBounds().getTopLeft().getX();
        final double tileMatrixMaxY = m.getBounds().getTopLeft().getY();

        final double epsilon = 1e-6;
        final int minCol = (int) Math.floor((bboxMinX - tileMatrixMinX) / tileSpan + epsilon);
        final int maxCol = (int) Math.floor((bboxMaxX - tileMatrixMinX) / tileSpan - epsilon);
        // Note: tiles indexed from top row (tileMatrixMaxY corresponds to row 0).
        final int minRow = (int) Math.floor((tileMatrixMaxY - bboxMaxY) / tileSpan + epsilon);
        final int maxRow = (int) Math.floor((tileMatrixMaxY - bboxMinY) / tileSpan - epsilon);

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
                new Coord(csr, leftX, upperY, 0.0), new Coord(csr, rightX, lowerY, 0.0));
    }

    public BufferedImage renderTile(int zoomLevel, int xcol, int yrow)
            throws OffscreenMapRenderer.MapLayerException {
        return renderTile(zoomLevel, xcol, yrow, null);
    }

    public BufferedImage renderTile(int zoomLevel, int xcol, int yrow, Duration timeout)
            throws OffscreenMapRenderer.MapLayerException {
        var cam = renderer.getMap().getCamera();
        cam.freeze(false);

        var tile = getTileBBox(zoomLevel, xcol, yrow);
        cam.moveToBoundingBox(tile, 0.0f, false, null, null);
        // TODO: this drawFrame calls update() on camera/layer for each tile separately. Is this
        // safe or will it cause issues at tile borders?
        return renderer.drawFrame(timeout);
    }

    /**
     * Attempt to force rendering a tile even if a map layer keeps saying it's not ready. See
     * OffscreenMapRenderer.forceDrawFrame.
     */
    public BufferedImage forceRenderTile(int zoomLevel, int xcol, int yrow) throws OffscreenMapRenderer.MapLayerException {
        try {
            return renderTile(zoomLevel, xcol, yrow, Duration.ofMillis(50));
        } catch (OffscreenMapRenderer.MapLayerException e) {
            if (e.getState() == LayerReadyState.NOT_READY) {
                return renderer.forceDrawFrame();
            } else {
                throw e;
            }
        }
    }

    public record TileRange(int zoomLevel, int minColumn, int maxColumn, int minRow, int maxRow) {}
}
