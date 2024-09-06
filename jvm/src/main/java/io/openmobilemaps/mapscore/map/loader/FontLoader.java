package io.openmobilemaps.mapscore.map.loader;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;

import javax.imageio.ImageIO;

import io.openmobilemaps.mapscore.graphics.BufferedImageTextureHolder;
import io.openmobilemaps.mapscore.shared.map.loader.Font;
import io.openmobilemaps.mapscore.shared.map.loader.FontLoaderInterface;
import io.openmobilemaps.mapscore.shared.map.loader.FontLoaderResult;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderStatus;

public class FontLoader extends FontLoaderInterface {

  private HashMap<String, FontLoaderResult> fontCache;

  public FontLoader() {
    fontCache = new HashMap<String, FontLoaderResult>();
  }

  @Override
  public FontLoaderResult loadFont(Font font) {
    synchronized (this) { // TODO: lazy, should allow loading _different_ fonts concurrently.

      var fontName = font.getName();
      var entry = this.fontCache.get(fontName);
      if (entry != null) {
        return entry;
      }
      System.out.printf("loadFont %s\n", fontName);
      var result = loadFont(fontName);
      if(result.getStatus() == LoaderStatus.ERROR_404) {
        result = loadFontFallback(fontName);
        System.out.printf("loadFont [Fallback] %s -> %s\n", fontName, result.getStatus());
      } else {
        System.out.printf("loadFont %s -> %s\n", fontName, result.getStatus());
      }
      fontCache.put(fontName, result);
      return result;
    }
  }

  protected FontLoaderResult loadFont(String fontName) {
    // TODO: no clue what is a sensible place to look for these font files
    String manifestName = fontName + ".json";
    String imageName = fontName + ".png";
    // Try resources:
    InputStream manifest = getClass().getResourceAsStream(manifestName);
    InputStream image = getClass().getResourceAsStream(imageName);
    if (manifest != null && image != null) {
      return readFont(manifest, image);
    } else if (manifest != null || image != null) {
      // Error if one is found but not the other.
      return new FontLoaderResult(null, null, LoaderStatus.ERROR_404);
    }
    // Try current working directory:
    try {
      manifest = new FileInputStream(manifestName);
      image = new FileInputStream(imageName);
      return readFont(manifest, image);
    } catch (FileNotFoundException e) {
      return new FontLoaderResult(null, null, LoaderStatus.ERROR_404);
    }
  }

  protected FontLoaderResult loadFontFallback(String fontName) {
    return loadFont("averta-bold");
  }

  protected FontLoaderResult readFont(InputStream manifestStream, InputStream imageStream) {
    try {
      var manifest = FontJsonManifestReader.read(manifestStream);
      var image = new BufferedImageTextureHolder(ImageIO.read(imageStream));
      return new FontLoaderResult(image, manifest, LoaderStatus.OK);
    } catch (IOException e) {
      System.out.println(e);
      return new FontLoaderResult(null, null, LoaderStatus.ERROR_OTHER);
    } catch (FontJsonManifestReader.InvalidManifestException e) {
      System.out.println(e);
      return new FontLoaderResult(null, null, LoaderStatus.ERROR_OTHER);
    }

  }

}
