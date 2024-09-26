package io.openmobilemaps.mapscore.map.loader;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.logging.Logger;

import javax.imageio.ImageIO;

import io.openmobilemaps.mapscore.graphics.BufferedImageTextureHolder;
import io.openmobilemaps.mapscore.shared.map.loader.Font;
import io.openmobilemaps.mapscore.shared.map.loader.FontData;
import io.openmobilemaps.mapscore.shared.map.loader.FontLoaderInterface;
import io.openmobilemaps.mapscore.shared.map.loader.FontLoaderResult;
import io.openmobilemaps.mapscore.shared.map.loader.FontWrapper;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderStatus;
import org.jetbrains.annotations.NotNull;

/**
 * Load fonts from local resources.
 *
 * <p>Fonts are expected to be defined by a JSON/PNG file pair. The JSON document is the font
 * manifest and the PNG is the multichannel signed-distance-field font atlas. See e.g.
 * <a href="https://github.com/soimy/msdf-bmfont-xml">https://github.com/soimy/msdf-bmfont-xml</a>
 */
public class FontLoader extends FontLoaderInterface {

    private static final Logger logger = Logger.getLogger(FontLoader.class.getName());
    private final HashMap<String, FontLoaderResult> fontCache;
    private final double dpFactor; // !< render-DPI / 160.0, factor for "Density Independent Pixel" size.

    /**
     * @param dpi: render resolution, for appropriate scaling.
     */
    public FontLoader(double dpi) {
        this.dpFactor = dpi / 160.0; // see android DisplayMetrics.density.
        fontCache = new HashMap<>();
    }

    @NotNull
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
            if (result.getStatus() == LoaderStatus.ERROR_404) {
                result = loadFontFallback(fontName);
                var fallbackName = "";
                if (result.getFontData() != null) {
                    fallbackName = result.getFontData().getInfo().getName();
                }
                System.out.printf(
                        "loadFont(%s) -> Fallback %s -> %s\n",
                        fontName, fallbackName, result.getStatus());
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
        logger.finer("Loading fallback for font " + fontName);
        return loadFont("Roboto-Regular"); // TODO: fallback to _any_ available font? Configurable?
    }

    protected FontLoaderResult readFont(InputStream manifestStream, InputStream imageStream) {
        try {
            var manifest = FontJsonManifestReader.read(manifestStream);

            // Adapt size for render resolution.
            // Note: this feels a bit clunky, couldn't this be done in the shader?
            manifest =
                    new FontData(
                            new FontWrapper(
                                    manifest.getInfo().getName(),
                                    manifest.getInfo().getLineHeight(),
                                    manifest.getInfo().getBase(),
                                    manifest.getInfo().getBitmapSize(),
                                    manifest.getInfo().getSize() * dpFactor // <-
                                    ),
                            manifest.getGlyphs());

            logger.finer("Font manifest:\n" + manifest.getInfo());

            var image = new BufferedImageTextureHolder(ImageIO.read(imageStream));
            return new FontLoaderResult(image, manifest, LoaderStatus.OK);
        } catch (IOException | FontJsonManifestReader.InvalidManifestException e) {
            logger.warning(e.toString());
            return new FontLoaderResult(null, null, LoaderStatus.ERROR_OTHER);
        }
    }
}
