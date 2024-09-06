package io.openmobilemaps.mapscore.graphics.util;

import java.awt.image.BufferedImage;

public class OSMesa {
  private final long ctx; // OSMesa context handle (actually a pointer)
  private long buf; // Opaque pointer to byte buffer allocated in makeCurrent, the image data.
  private int width;
  private int height;

  public OSMesa() throws Exception {
    this.ctx = createContext();
    if (ctx == 0) {
      throw new Exception("Could not create OSMesa context");
    }
  }

  public OSMesa(int width, int height) throws Exception {
      this();
      makeCurrent(width, height);
  }

  public void makeCurrent(int width, int height) throws Exception {
    free(buf);
    this.width = width;
    this.height = height;
    buf = OSMesa.makeCurrent(ctx, width, height);
    if (buf == 0) {
      throw new Exception("Could not make OSMesa context current");
    }
  }

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

  // Analogous BufferedImage.setRGB with a vertically flip.
  // Simplified (startX/Y = 0 and scansize == width). 
  static private void setARGBflipV(BufferedImage image, int w, int h, int[] buf) {
      Object pixel = null;
      var colorModel = image.getColorModel();
      var raster = image.getRaster();

      int off = 0;
      for(int y = h-1; y >= 0; --y) {
         for(int x = 0; x < w; ++x) {
            pixel = colorModel.getDataElements(buf[off++], pixel);
            raster.setDataElements(x, y, pixel);
         }
      }
  }

  protected void finalize() {
    free(buf);
  }

  private static native long createContext();

  private static native long makeCurrent(long ctx, int width, int height);

  private static native void readARGB(long buf, int[] out);

  private static native void free(long buf);
}
