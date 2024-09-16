package io.openmobilemaps.mapscore.map.util;

import java.awt.image.BufferedImage;
import java.sql.Time;

import io.openmobilemaps.mapscore.graphics.util.OSMesa;
import io.openmobilemaps.mapscore.shared.graphics.common.Color;
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2I;
import io.openmobilemaps.mapscore.shared.map.LayerInterface;
import io.openmobilemaps.mapscore.shared.map.LayerReadyState;
import io.openmobilemaps.mapscore.shared.map.MapCallbackInterface;
import io.openmobilemaps.mapscore.shared.map.MapConfig;
import io.openmobilemaps.mapscore.shared.map.MapInterface;
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemFactory;

public class OffscreenMapRenderer {
  private MapInterface map;
  private OSMesa ctx;
  private long threadId;

  public class MapLayerException extends Exception {
    private LayerInterface layer;
    private LayerReadyState state;

    MapLayerException(LayerInterface layer, LayerReadyState state) {
      this.layer = layer;
      this.state = state;
    }
  }

  public OffscreenMapRenderer(int width, int height, int numSamples) {
    // Screen DPI: 1 inch / 0.28mm (as defined in Annex E "Well-known scale sets" in "OpenGIS® Web Map Tile Service Implementation Standard").
    this(width, height, numSamples, new MapConfig(CoordinateSystemFactory.getEpsg3857System()), 90.714286f);
  }

  public OffscreenMapRenderer(int width, int height, int numSamples, MapConfig mapConfig, float dpi) {
    threadId = Thread.currentThread().threadId();

    map = MapInterface.createWithOpenGl(mapConfig, dpi);
    // Set dummy callbacks, we don't use these here.
		map.setCallbackHandler(new MapCallbackInterface() {
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
    assertGlContextThread();
    ctx.makeCurrent(width, height, numSamples);
    map.setViewportSize(new Vec2I(width, height));
  }

  public MapInterface getMap() {
    return map;
  }

  // TODO: necessary?
  public void addLayerSync(LayerInterface layer) {
    final int numBefore = map.getLayers().size();
    map.addLayer(layer);
    runGraphicsTasks();
    if (map.getLayers().size() != numBefore+1) {
      throw new AssertionError("expected new layer to be here by now, something is wrong");
    }

  }

  public BufferedImage drawFrame() throws MapLayerException {
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
        layer.enableAnimations(false);
        layer.update();
        layer.update();
    }
    map.drawFrame();
    return ctx.getImage();
  }

  private void awaitReady() throws MapLayerException {
    // Note: this sort of duplicates map.drawReadyFrame.
    // The key difference is that this implementation assumes that we're already on the render thread and so we don't have to juggle with invalidate and callbacks etc.

    // TODO: looks like we need a timeout? Randomly gets stuck with layers never getting ready sometimes...

    boolean allReady = true;
    int i = 0;
    do {
      runGraphicsTasks();
      allReady = true;
      for(var layer : map.getLayers()) {
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
      if(++i % 1000 == 0 && !allReady) {
        System.out.printf("waiting\n");
      }
    } while(!allReady);
  }

  private void runGraphicsTasks() {
    assertGlContextThread();
    while (map.getScheduler().runGraphicsTasks()) {
    }
  }

  private void assertGlContextThread() {
    if (Thread.currentThread().threadId() != threadId) { 
      throw new AssertionError("must be called from the thread that initialized this OffscreenMapRenderer object -- the GL context is made active _only_ on that thread"); 
    }
  }
}
