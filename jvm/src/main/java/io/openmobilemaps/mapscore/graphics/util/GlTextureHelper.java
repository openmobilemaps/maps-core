package io.openmobilemaps.mapscore.graphics.util;

import java.awt.image.BufferedImage;

public class GlTextureHelper {

  public static int createTexture(BufferedImage image) {
    int width = image.getWidth();
    int height = image.getHeight();

    int[] data = image.getRGB(0, 0, width, height, null, 0, width);

    return createTextureARGB(data, width, height);
  }

  public static native void deleteTexture(int textureId);

  private static native int createTextureARGB(int[] data, int width, int height);
}
