package io.openmobilemaps.mapscore.graphics.util;

import java.awt.image.BufferedImage;

/**
 * Simple bindings for OSMesa, the Mesa Off-Screen rendering interface.
 *
 * The interface is somewhat oversimplified for our current use case and does
 * not expose the full functionality of OSMesa.
 */
public class OSMesa {
  private final long ctx; // OSMesa context handle (actually a pointer)
  private long buf; // Opaque pointer to byte buffer allocated in makeCurrent, the image data.
  private int width;
  private int height;

  public class OSMesaError extends Error {
    OSMesaError(String reason) {
      super(reason);
    }
  }

  /**
   * Create the OSMesa context. The context needs to be activated with
   * makeCurrent before any GL operations can take place.
   */
  public OSMesa() {
    this.ctx = createContext();
    if (ctx == 0) {
      throw new OSMesaError("Could not create OSMesa context");
    }
  }

  /**
   * Create the OSMesa context and activate it immediately.
   */
  public OSMesa(int width, int height) throws OSMesaError {
    this();
    makeCurrent(width, height);
  }

  /**
   * Activate this OSMesa context for GL operations and initialize the
   * renderbuffer.
   * This can be called any number of times to modify the renderbuffer size.
   */
  public void makeCurrent(int width, int height) throws OSMesaError {
    this.width = width;
    this.height = height;
    buf = OSMesa.makeCurrent(ctx, buf, width, height);
    if (buf == 0) {
      throw new OSMesaError("Could not activate OSMesa context");
    }
  }

  /**
   * Get the rendered image.
   */
  // NOTE: in the OSMesa C-API, the image is rendered directly to the
  // user-provided buffer. Currently, we make a couple of copies. (Could be
  // optimized using ByteBuffer).
  public BufferedImage getImage() {
    var out = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
    var tmpARGB = new int[width * height];
    readARGB(buf, tmpARGB);
    setARGBflipV(out, width, height, tmpARGB);
    return out;
  }

  // Analogous BufferedImage.setRGB with a vertical flip.
  // Simplified (startX/Y = 0 and scansize == width).
  static private void setARGBflipV(BufferedImage image, int w, int h, int[] buf) {
    Object pixel = null;
    var colorModel = image.getColorModel();
    var raster = image.getRaster();

    int off = 0;
    for (int y = h - 1; y >= 0; --y) {
      for (int x = 0; x < w; ++x) {
        pixel = colorModel.getDataElements(buf[off++], pixel);
        raster.setDataElements(x, y, pixel);
      }
    }
  }

  public void destroy() {
    destroy(ctx, buf);
  }

  private static native long createContext();

  private static native long makeCurrent(long ctx, long buf, int width, int height);

  private static native void readARGB(long buf, int[] out);

  private static native void destroy(long ctx, long buf);
}
