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
  // user-provided buffer. With JNI, this doesn't work (buffer needs to be
  // explicitly released for data to be visible in Java)
  public BufferedImage getImage() {
    var out = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
    var tmpARGB = new int[width * height];
    read(buf, tmpARGB);
    out.setRGB(0, 0, width, height, tmpARGB, 0, width);
    return out;
  }

  protected void finalize() {
    free(buf);
  }

  private static native long createContext();

  private static native long makeCurrent(long ctx, int width, int height);

  private static native void read(long buf, int[] out);

  private static native void free(long buf);
}
