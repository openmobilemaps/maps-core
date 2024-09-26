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

import java.awt.image.BufferedImage;

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

        // - getVpMatrix must be called before first update.
        map.getCamera().asCameraInterface().getVpMatrix();
        for (var layer : map.getLayers()) {
            layer.enableAnimations(false);
        }

        awaitReady();

        // Text icon fade in workaround:
        // - getVpMatrix must be called before first update.
        // - need two calls to update before first actual draw.
        //    -> first update runs symbol collision detection
        //    -> snd update to show non-colliding symbols.
        //    (and another one that is implicit in drawFrame?)
        // XXX: still have text labels flickering back and forth.
        map.getCamera().asCameraInterface().getVpMatrix();
        for (var layer : map.getLayers()) {
            layer.update();
            layer.update();
        }
        map.drawFrame();
        return ctx.getImage();
    }

    private void awaitReady() throws MapLayerException {
        // Note: this sort of duplicates `map.drawReadyFrame`.
        // The key difference is that this implementation assumes that we're already on the render
        // thread so we don't have to juggle with invalidate and callbacks etc.

        // TODO: looks like we need a timeout? Randomly gets stuck with layers never getting ready
        // sometimes...

        boolean allReady;
        long triggerUpdate = System.currentTimeMillis() + 10;
        do {
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

            final long now = System.currentTimeMillis();
            if (now > triggerUpdate) {
                // Trigger layer updates. This _might_ trigger some more activity that might make
                // the layer ready, eventually.
                for (var layer : map.getLayers()) {
                    layer.update();
                }
            }
        } while (!allReady);
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

        @Override
        public String getMessage() {
            return String.format(
                    "Layer of type %s returned readiness state %s",
                    layer.getClass().getCanonicalName(), state);
        }
    }
}
