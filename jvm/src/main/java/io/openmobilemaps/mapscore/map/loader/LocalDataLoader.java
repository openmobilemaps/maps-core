package io.openmobilemaps.mapscore.map.loader;

import com.snapchat.djinni.Future;
import com.snapchat.djinni.Promise;

import io.openmobilemaps.mapscore.graphics.BufferedImageTextureHolder;
import io.openmobilemaps.mapscore.shared.map.loader.DataLoaderResult;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderStatus;
import io.openmobilemaps.mapscore.shared.map.loader.TextureLoaderResult;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.awt.image.BufferedImage;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.*;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collection;
import java.util.logging.Logger;

import javax.imageio.ImageIO;

/**
 * Data loader for jar:file:... or file: URLs.
 *
 * <p>Implementation of LoaderInterface for local resources from files and jar-files.
 *
 * <p>Textures are returned in the {@link
 * io.openmobilemaps.mapscore.graphics.BufferedImageTextureHolder} which can load textures for
 * OpenGL.
 */
@SuppressWarnings("unused")
public class LocalDataLoader extends LoaderInterface {
    private static final Logger logger = Logger.getLogger(LocalDataLoader.class.getName());
    private final Collection<String> allowedPrefixes;

    public LocalDataLoader() {
        allowedPrefixes = new ArrayList<>();
    }

    /**
     * @param allowedPrefixes Enable loading from file: resources for these paths. This should be
     *     restricted to paths containing relevant map resources for security reasons.
     */
    public LocalDataLoader(Collection<String> allowedPrefixes) {
        this.allowedPrefixes = allowedPrefixes;
    }

    private boolean isAllowed(final URI uri) {
        return "jar".equals(uri.getScheme())
                || ("file".equals(uri.getScheme())
                        && allowedPrefixes.stream()
                                .anyMatch(prefix -> uri.getPath().startsWith(prefix)));
    }

    @NotNull
    @Override
    public DataLoaderResult loadData(@NotNull String url, @Nullable String etag) {
        final URI uri = URI.create(url);
        if (!isAllowed(uri)) {
            return new DataLoaderResult(null, null, LoaderStatus.NOOP, null);
        }

        DataLoaderResult result;
        try {
            URLConnection connection = uri.toURL().openConnection();
            var data = connection.getInputStream().readAllBytes();
            var buf = ByteBuffer.allocateDirect(data.length);
            buf.put(data);
            result = new DataLoaderResult(buf, null, LoaderStatus.OK, null);
        } catch (FileNotFoundException e) {
            result = new DataLoaderResult(null, null, LoaderStatus.ERROR_404, e.toString());
        } catch (IOException e) {
            result = new DataLoaderResult(null, null, LoaderStatus.ERROR_OTHER, e.toString());
        }
        logger.info(String.format("loadData %s -> %s", uri, result.getStatus()));
        return result;
    }

    @NotNull
    @Override
    public Future<DataLoaderResult> loadDataAsync(@NotNull String url, @Nullable String etag) {
        // No async here.
        var result = new Promise<DataLoaderResult>();
        result.setValue(loadData(url, etag));
        return result.getFuture();
    }

    @NotNull
    @Override
    public TextureLoaderResult loadTexture(@NotNull String url, @Nullable String etag) {
        final URI uri = URI.create(url);
        if (!isAllowed(uri)) {
            return new TextureLoaderResult(null, null, LoaderStatus.NOOP, null);
        }

        TextureLoaderResult result;
        try {
            BufferedImage image = ImageIO.read(uri.toURL());
            return new TextureLoaderResult(
                    new BufferedImageTextureHolder(image), null, LoaderStatus.OK, null);
        } catch (FileNotFoundException e) {
            result = new TextureLoaderResult(null, null, LoaderStatus.ERROR_404, e.toString());
        } catch (IOException e) {
            result = new TextureLoaderResult(null, null, LoaderStatus.ERROR_OTHER, e.toString());
        }
        logger.info(String.format("loadTexture %s -> %s", uri, result.getStatus()));
        return result;
    }

    @NotNull
    @Override
    public Future<TextureLoaderResult> loadTextureAsnyc(
            @NotNull String url, @Nullable String etag) {
        // No async here.
        var result = new Promise<TextureLoaderResult>();
        result.setValue(loadTexture(url, etag));
        return result.getFuture();
    }

    @Override
    public void cancel(@NotNull String url) {}

}
