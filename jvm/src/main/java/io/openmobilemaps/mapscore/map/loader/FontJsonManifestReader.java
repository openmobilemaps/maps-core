package io.openmobilemaps.mapscore.map.loader;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import io.openmobilemaps.mapscore.shared.graphics.common.Quad2dD;
import io.openmobilemaps.mapscore.shared.graphics.common.Vec2D;
import io.openmobilemaps.mapscore.shared.map.loader.FontData;
import io.openmobilemaps.mapscore.shared.map.loader.FontGlyph;
import io.openmobilemaps.mapscore.shared.map.loader.FontWrapper;

class FontJsonManifestReader {
  /**
   * Read font manifest json document and return the corresponding
   * FontData representation.
   */
  static FontData read(InputStream stream) throws IOException, InvalidManifestException {
    try {
      return extractFontData(new ObjectMapper().readTree(stream));
    } catch (IllegalArgumentException e) {
      throw new InvalidManifestException(e.getMessage());
    }
  }

  private static FontData extractFontData(JsonNode object) {
    JsonNode info = object.required("info");
    var name = info.required("face").asText();
    var size = info.required("size").asDouble();

    var common = object.required("common");
    var lineHeight = common.required("lineHeight").asDouble();
    var base = common.required("base").asDouble();
    var imageSize = common.required("scaleW").asDouble(); // XXX: really ignore scaleH?

    var fontWrapper = new FontWrapper(
        name,
        lineHeight,
        base,
        new Vec2D(imageSize, imageSize),
        size);

    var chars = object.required("chars");
    var glyphs = new ArrayList<FontGlyph>();
    if (chars.isArray()) {
      for (var glyphEntry : chars) {

        var width = glyphEntry.required("width").asDouble();
        var height = glyphEntry.required("height").asDouble();
        var s0 = glyphEntry.required("x").asDouble();
        var s1 = s0 + width;
        var t0 = glyphEntry.required("y").asDouble();
        var t1 = t0 + height;

        s0 = s0 / imageSize;
        s1 = s1 / imageSize;
        t0 = t0 / imageSize;
        t1 = t1 / imageSize;

        var bearing = new Vec2D(glyphEntry.required("xoffset").asDouble() / size,
            -glyphEntry.required("yoffset").asDouble() / size);
        var advance = new Vec2D(glyphEntry.required("xadvance").asDouble() / size, 0.0);
        var bbox = new Vec2D(width / size, height / size);

        glyphs.add(new FontGlyph(
            glyphEntry.required("char").asText(),
            advance,
            bbox,
            bearing,
            new Quad2dD(
                new Vec2D(s0, t1),
                new Vec2D(s1, t1),
                new Vec2D(s1, t0),
                new Vec2D(s0, t0))));
      }
    }
    return new FontData(fontWrapper, glyphs);
  }

  public static class InvalidManifestException extends Exception {
    public InvalidManifestException(String errorMessage) {
      super(errorMessage);
    }
  }
}
