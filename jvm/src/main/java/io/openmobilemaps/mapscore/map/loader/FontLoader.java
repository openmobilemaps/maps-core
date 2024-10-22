package io.openmobilemaps.mapscore.map.loader;

import io.openmobilemaps.mapscore.graphics.BufferedImageTextureHolder;
import io.openmobilemaps.mapscore.shared.map.loader.Font;
import io.openmobilemaps.mapscore.shared.map.loader.FontData;
import io.openmobilemaps.mapscore.shared.map.loader.FontLoaderInterface;
import io.openmobilemaps.mapscore.shared.map.loader.FontLoaderResult;
import io.openmobilemaps.mapscore.shared.map.loader.FontWrapper;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderStatus;

import org.jetbrains.annotations.NotNull;

import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.logging.Logger;

import javax.imageio.ImageIO;

/**
 * Load fonts from local ClassLoader resources.
 *
 * <p>Fonts are expected to be defined by a JSON/PNG file pair. The JSON document is the font
 * manifest and the PNG is the multichannel signed-distance-field font atlas. See e.g. <a
 * href="https://github.com/soimy/msdf-bmfont-xml">https://github.com/soimy/msdf-bmfont-xml</a>
 */
public class FontLoader extends FontLoaderInterface {

    private static final Logger logger = Logger.getLogger(FontLoader.class.getName());
    private final HashMap<String, FontLoaderResult> fontCache;
    private final double
            dpFactor; // !< render-DPI / 160.0, factor for "Density Independent Pixel" size.
    private final ClassLoader classLoader;
    private final String fontDirectory;
    private final String fallbackFontName;

    /**
     * @param dpi render resolution, for appropriate scaling.
     * @param classLoader ClassLoader in which to look for font resources
     * @param fontDirectory: directory in which to look for font resources
     * @param fallbackFontName optional, name of font to use as fallback for unknown fonts
     */
    public FontLoader(
            double dpi,
            @NotNull ClassLoader classLoader,
            @NotNull String fontDirectory,
            String fallbackFontName) {
        this.dpFactor = dpi / 160.0; // see android DisplayMetrics.density.
        this.classLoader = classLoader;
        this.fontDirectory = fontDirectory;
        this.fallbackFontName = fallbackFontName;
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
            logger.info("loadFont " + fontName);
            var result = loadFont(fontName);
            String fallbackName = null;
            if (result.getStatus() == LoaderStatus.ERROR_404) {
                fallbackName = getFallbackFont(fontName);
            }
            if (fallbackName != null) {
                result = loadFont(getFallbackFont(fontName));
                logger.info(
                        String.format(
                                "loadFont(%s) -> Fallback %s -> %s",
                                fontName, fallbackName, result.getStatus()));
            } else {
                logger.info(String.format("loadFont %s -> %s", fontName, result.getStatus()));
            }
            fontCache.put(fontName, result);
            return result;
        }
    }

    protected FontLoaderResult loadFont(String fontName) {

        try (FontDataStreams streams = findFontData(fontName)) {
            if (streams == null || streams.image() == null || streams.manifest() == null) {
                return new FontLoaderResult(null, null, LoaderStatus.ERROR_404);
            }
            return readFont(streams.image(), streams.manifest());
        } catch (Exception e) {
            return new FontLoaderResult(null, null, LoaderStatus.ERROR_OTHER);
        }
    }

    /**
     * Find the font data, that is the font.png image and font.json manifest. Implementation should
     * return null if either item could not be found.
     *
     * @return InputStreams for font data, i.e. the png image and the manifest, or null if not
     *     found.
     */
    protected FontDataStreams findFontData(String fontName) {
        return new FontDataStreams(
                classLoader.getResourceAsStream(
                        String.format("%s/%s.png", fontDirectory, fontName)),
                classLoader.getResourceAsStream(
                        String.format("%s/%s.json", fontDirectory, fontName)));
    }

    protected String getFallbackFont(String ignoredFontName) {
        return fallbackFontName;
    }

    protected FontLoaderResult readFont(InputStream imageStream, InputStream manifestStream) {
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

            logger.info("Font manifest:\n" + manifest.getInfo());

            var image = new BufferedImageTextureHolder(ImageIO.read(imageStream));
            return new FontLoaderResult(image, manifest, LoaderStatus.OK);
        } catch (IOException | FontJsonManifestReader.InvalidManifestException e) {
            logger.warning(e.toString());
            return new FontLoaderResult(null, null, LoaderStatus.ERROR_OTHER);
        }
    }

    protected static record FontDataStreams(InputStream image, InputStream manifest)
            implements AutoCloseable {
        @Override
        public void close() throws Exception {
            if (image != null) {
                image.close();
            }
            if (manifest != null) {
                manifest.close();
            }
        }
    }
}
