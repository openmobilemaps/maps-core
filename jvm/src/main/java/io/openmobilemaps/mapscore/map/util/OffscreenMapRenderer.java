package io.openmobilemaps.mapscore.map.util;

import io.openmobilemaps.mapscore.graphics.util.OSMesa;
import io.openmobilemaps.mapscore.shared.graphics.common.Color;
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I;
import io.openmobilemaps.mapscore.shared.map.LayerInterface;
import io.openmobilemaps.mapscore.shared.map.LayerReadyState;
import io.openmobilemaps.mapscore.shared.map.MapCallbackInterface;
import io.openmobilemaps.mapscore.shared.map.MapConfig;
import io.openmobilemaps.mapscore.shared.map.MapInterface;
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemFactory;

import org.jetbrains.annotations.Nullable;

import java.awt.image.BufferedImage;
import java.time.Duration;

public class OffscreenMapRenderer {
    private final MapInterface map;
    private final OSMesa ctx;
    private long threadId;

    public OffscreenMapRenderer(int width, int height, int numSamples) {
        // Screen DPI: 1 inch / 0.28mm (as defined in Annex E "Well-known scale sets" in "OpenGIS®
        // Web Map Tile Service Implementation Standard").
        this(
                width,
                height,
                numSamples,
                new MapConfig(CoordinateSystemFactory.getEpsg3857System()),
                2 * 90.714286f);
    }

    public OffscreenMapRenderer(
            int width, int height, int numSamples, MapConfig mapConfig, float dpi) {
        threadId = Thread.currentThread().threadId();

        map = MapInterface.createWithOpenGl(mapConfig, dpi);
        // Set dummy callbacks, we don't use these here.
        map.setCallbackHandler(
                new MapCallbackInterface() {
                    @Override
                    public void invalidate() {}

                    @Override
                    public void onMapResumed() {}
                });
        map.resume();

        ctx = new OSMesa(width, height, numSamples);
        map.getRenderingContext().onSurfaceCreated();
        map.setViewportSize(new Vec2I(width, height));
        map.setBackgroundColor(new Color(0.0f, 0.0f, 0.0f, 0.0f));
    }

    public void destroy() {
        map.destroy();
        ctx.destroy();
    }

    public void setFramebufferSize(int width, int height, int numSamples) {
        threadId = Thread.currentThread().threadId();
        ctx.makeCurrent(width, height, numSamples);
        map.setViewportSize(new Vec2I(width, height));
    }

    public MapInterface getMap() {
        return map;
    }

    public BufferedImage drawFrame() throws MapLayerException {
        return drawFrame(null);
    }

    public BufferedImage drawFrame(@Nullable Duration timeout) throws MapLayerException {
        // Text icon fade in workaround:
        // - getVpMatrix must be called before first update.
        // - need two calls to update before first actual draw.
        //    -> first update runs symbol collision detection
        //    -> snd update to show non-colliding symbols.
        //    (and another one that is implicit in drawFrame?)
        map.getCamera().asCameraInterface().getVpMatrix();
        for (var layer : map.getLayers()) {
            layer.enableAnimations(false);
        }

        awaitReady(timeout);

        for (var layer : map.getLayers()) {
            layer.update();
            layer.update();
        }

        map.prepare();
        map.drawFrame();
        return ctx.getImage();
    }

    /**
     * Occasionally map layers may get stuck and return an unready state, but might in fact be ready to render.
     * This method simply draws the map anyway, but all bets are off if the output is correct.
     */
    public BufferedImage forceDrawFrame() {
        map.prepare();
        map.drawFrame();
        return ctx.getImage();
    }

    private void awaitReady(@Nullable Duration timeout) throws MapLayerException {
        // Note: this sort of duplicates `map.drawReadyFrame`.
        // The key difference is that this implementation assumes that we're already on the render
        // thread so we don't have to juggle with invalidate and callbacks etc.

        if (timeout == null) {
            timeout = Duration.ofMinutes(1); // basically an eternity.
        }
        final long hopeTime = Math.clamp(0, 10, timeout.toMillis() - 10);

        boolean allReady = false;
        final long deadline = System.currentTimeMillis() + timeout.toMillis();
        while (!allReady && System.currentTimeMillis() < (deadline - hopeTime)) {
            runGraphicsTasks();
            allReady = true;
            for (var layer : map.getLayers()) {
                var readyState = layer.isReadyToRenderOffscreen();
                if (readyState == LayerReadyState.NOT_READY) {
                    // not ready yet, stay in the wait loop
                    allReady = false;
                    break;
                } else if (readyState != LayerReadyState.READY) {
                    throw new MapLayerException(layer, readyState);
                }
            }
            Thread.yield();
        }

        // timeout
        if (!allReady) {
            // The layer might have gotten stuck somehow. Try to kick "update" on the layers and see
            // if this helps. Shouldn't have to, but who knows.
            while (System.currentTimeMillis() < deadline) {
                for (var layer : map.getLayers()) {
                    layer.update();
                }
                runGraphicsTasks();
                Thread.yield();
            }

            for (var layer : map.getLayers()) {
                var readyState = layer.isReadyToRenderOffscreen();
                if (readyState != LayerReadyState.READY) {
                    throw new MapLayerException(layer, readyState);
                }
            }
        }
    }

    private void runGraphicsTasks() {
        assertGlContextThread();
        //noinspection StatementWithEmptyBody
        while (map.getScheduler().runGraphicsTasks()) {}
    }

    private void assertGlContextThread() {
        if (Thread.currentThread().threadId() != threadId) {
            throw new RuntimeException(
                    "must be called from the thread that initialized this OffscreenMapRenderer object -- the GL context is made active _only_ on that thread");
        }
    }

    public static class MapLayerException extends Exception {
        private final LayerInterface layer;
        private final LayerReadyState state;

        MapLayerException(LayerInterface layer, LayerReadyState state) {
            this.layer = layer;
            this.state = state;
        }

        public LayerReadyState getState() {
            return state;
        }

        public LayerInterface getLayer() {
            return layer;
        }

        @Override
        public String getMessage() {
            return String.format(
                    "Layer of type %s returned readiness state %s",
                    layer.getClass().getCanonicalName(), state);
        }
    }
}
