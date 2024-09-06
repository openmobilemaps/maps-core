package io.openmobilemaps.mapscore.graphics;

import io.openmobilemaps.mapscore.shared.graphics.objects.TextureHolderInterface;

import io.openmobilemaps.mapscore.graphics.util.GlTextureHelper;
import java.awt.image.BufferedImage;

public class BufferedImageTextureHolder extends TextureHolderInterface {
  protected BufferedImage image;
  protected int usageCounter;
  protected int textureId;

  public BufferedImageTextureHolder(BufferedImage image) {
    this.image = image;
    this.usageCounter = 0;
  }

  @Override 
  public int getImageWidth() {
    return image.getWidth();
  }

  @Override 
  public int getImageHeight() {
    return image.getHeight();
  }

  @Override 
  public int getTextureWidth() {
    return image.getWidth();
  }

  @Override 
  public int getTextureHeight() {
    return image.getHeight();
  }

  @Override
  public int attachToGraphics() { 
    synchronized(this) {
      if(usageCounter == 0) {
        textureId = GlTextureHelper.createTexture(image);
        System.out.printf("attach to graphics %d\n", textureId);
      }
      usageCounter++;
    }
    return textureId; 
  }

  @Override
  public void clearFromGraphics() {
    synchronized(this) {
      usageCounter--;
      if(usageCounter == 0) {
        GlTextureHelper.deleteTexture(textureId);
        textureId = 0;
      }
    }
  }
}
