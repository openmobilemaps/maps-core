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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.URI;
import java.net.URLConnection;
import java.nio.ByteBuffer;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collection;

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
    private static final Logger logger = LoggerFactory.getLogger(LocalDataLoader.class);
    private final Collection<String> allowedPrefixes;

    public LocalDataLoader() {
        allowedPrefixes = new ArrayList<>();
    }

    /**
     * @param allowedPrefixes Enable loading from file: resources for these paths. This should be
     *                        restricted to paths containing relevant map resources for security reasons.
     */
    public LocalDataLoader(Collection<String> allowedPrefixes) {
        this.allowedPrefixes = allowedPrefixes;
    }

    private boolean isAllowed(final URI uri) {
        if ("jar".equals(uri.getScheme())) {
            return true;
        } else if ("file".equals(uri.getScheme())) {
            String normalizedPath = Paths.get(uri.getPath()).normalize().toString();
            return allowedPrefixes.stream()
                    .anyMatch(normalizedPath::startsWith);
        } else {
            return false;
        }
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
            try (var inputStream = connection.getInputStream()) {
                var data = inputStream.readAllBytes();
                var buf = ByteBuffer.allocateDirect(data.length);
                buf.put(data);
                result = new DataLoaderResult(buf, null, LoaderStatus.OK, null);
            }
        } catch (FileNotFoundException e) {
            result = new DataLoaderResult(null, null, LoaderStatus.ERROR_404, e.toString());
        } catch (IOException e) {
            result = new DataLoaderResult(null, null, LoaderStatus.ERROR_OTHER, e.toString());
        }
        logger.info("loadData {} -> {}", uri, result.getStatus());
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
        logger.info("loadTexture {} -> {}", uri, result.getStatus());
        return result;
    }

    @NotNull
    @Override
    public Future<TextureLoaderResult> loadTextureAsync(@NotNull String url, @Nullable String etag) {
        // No async here.
        var result = new Promise<TextureLoaderResult>();
        result.setValue(loadTexture(url, etag));
        return result.getFuture();
    }

    @Override
    public void cancel(@NotNull String url) {
    }

}
