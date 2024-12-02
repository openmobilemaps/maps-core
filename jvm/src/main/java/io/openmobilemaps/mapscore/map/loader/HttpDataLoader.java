package io.openmobilemaps.mapscore.map.loader;

import com.snapchat.djinni.Future;
import com.snapchat.djinni.Promise;

import io.openmobilemaps.mapscore.graphics.BufferedImageTextureHolder;
import io.openmobilemaps.mapscore.shared.map.loader.DataLoaderResult;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderStatus;
import io.openmobilemaps.mapscore.shared.map.loader.TextureLoaderResult;

import org.jetbrains.annotations.NotNull;

import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.ByteBuffer;
import java.time.Duration;
import java.util.logging.Logger;
import java.util.zip.GZIPInputStream;

import javax.imageio.ImageIO;

/**
 * Implementation of LoaderInterface for HTTP/HTTPS resources using the default {@link
 * java.net.http.HttpClient}.
 *
 * <p>Textures are returned in the {@link
 * io.openmobilemaps.mapscore.graphics.BufferedImageTextureHolder} which can load textures for
 * OpenGL.
 */
public class HttpDataLoader extends LoaderInterface {
    private static final Logger logger = Logger.getLogger(HttpDataLoader.class.getName());
    protected final HttpClient httpClient;

    public HttpDataLoader() {
        httpClient =
                HttpClient.newBuilder()
                        .followRedirects(HttpClient.Redirect.NORMAL)
                        .connectTimeout(Duration.ofSeconds(3))
                        .build();
    }

    public HttpDataLoader(HttpClient httpClient) {
        this.httpClient = httpClient;
    }

    private static InputStream getDecodedInputStream(HttpResponse<InputStream> httpResponse)
            throws IOException, UnsupportedOperationException {
        String encoding =
                httpResponse.headers().firstValue("Content-Encoding").orElse("").toLowerCase();
        return switch (encoding) {
            case "" -> httpResponse.body();
            case "gzip" -> new GZIPInputStream(httpResponse.body());
            default ->
                    throw new UnsupportedOperationException(
                            "Unexpected Content-Encoding: " + encoding);
        };
    }

    private static boolean isHTTP(final URI uri) {
        return "http".equals(uri.getScheme()) || "https".equals(uri.getScheme());
    }

    @NotNull
    @Override
    public DataLoaderResult loadData(@NotNull String url, String etag) {
        // TODO: cache?
        final URI uri = URI.create(url);
        if (!isHTTP(uri)) {
            return new DataLoaderResult(null, null, LoaderStatus.NOOP, null);
        }
        var request = HttpRequest.newBuilder(uri).build();
        try {
            HttpResponse<InputStream> response =
                    httpClient.send(request, HttpResponse.BodyHandlers.ofInputStream());
            return completeLoadData(response);
        } catch (IOException | InterruptedException e) {
            logger.info(String.format("loadData %s -> %s", url, e));
            return new DataLoaderResult(null, null, LoaderStatus.ERROR_OTHER, e.toString());
        }
    }

    @NotNull
    @Override
    public Future<DataLoaderResult> loadDataAsync(@NotNull String url, String etag) {

        var result = new Promise<DataLoaderResult>();
        final URI uri = URI.create(url);
        if (!isHTTP(uri)) {
            result.setValue(new DataLoaderResult(null, null, LoaderStatus.NOOP, null));
        } else {
            logger.info(String.format("loadDataAsync %s", url));
            var request = HttpRequest.newBuilder(uri).build();
            httpClient
                    .sendAsync(request, HttpResponse.BodyHandlers.ofInputStream())
                    .thenApply(this::completeLoadData)
                    .thenAccept(result::setValue);
        }
        return result.getFuture();
    }

    @NotNull
    @Override
    public TextureLoaderResult loadTexture(@NotNull String url, String etag) {
        final URI uri = URI.create(url);
        if (!isHTTP(uri)) {
            return new TextureLoaderResult(null, null, LoaderStatus.NOOP, null);
        }
        var request = HttpRequest.newBuilder(uri).build();
        try {
            HttpResponse<InputStream> response =
                    httpClient.send(request, HttpResponse.BodyHandlers.ofInputStream());
            return completeLoadTexture(response);
        } catch (InterruptedException | IOException e) {
            logger.info(String.format("loadTexture %s -> %s", url, e));
            return new TextureLoaderResult(null, null, LoaderStatus.ERROR_OTHER, e.toString());
        }
    }

    @NotNull
    @Override
    public Future<TextureLoaderResult> loadTextureAsnyc(@NotNull String url, String etag) {

        var result = new Promise<TextureLoaderResult>();
        final URI uri = URI.create(url);
        if (!isHTTP(uri)) {
            result.setValue(new TextureLoaderResult(null, null, LoaderStatus.NOOP, null));
        } else {
            logger.info(String.format("loadTextureAsync %s", url));
            var request = HttpRequest.newBuilder(uri).build();
            httpClient
                    .sendAsync(request, HttpResponse.BodyHandlers.ofInputStream())
                    .thenApply(this::completeLoadTexture)
                    .thenAccept(result::setValue);
        }
        return result.getFuture();
    }

    @Override
    public void cancel(@NotNull String url) {}

    protected DataLoaderResult completeLoadData(HttpResponse<InputStream> response) {
        LoaderStatus error;
        if (response.statusCode() == 200) {
            try {
                var body = getDecodedInputStream(response).readAllBytes();
                var buf = ByteBuffer.allocateDirect(body.length);
                buf.put(body);
                var result =
                        new DataLoaderResult(
                                buf,
                                response.headers().firstValue("etag").orElse(null),
                                LoaderStatus.OK,
                                null);
                logger.info(String.format("loadData %s -> %s", response.uri(), LoaderStatus.OK));
                return result;
            } catch (IOException | UnsupportedOperationException e) {
                error = LoaderStatus.ERROR_OTHER;
            }
        } else if (response.statusCode() == 400) {
            error = LoaderStatus.ERROR_400;
        } else if (response.statusCode() == 404) {
            error = LoaderStatus.ERROR_404;
        } else {
            error = LoaderStatus.ERROR_OTHER;
        }
        logger.info(String.format("loadData %s -> %s", response.uri(), error));
        return new DataLoaderResult(null, null, error, null);
    }

    protected TextureLoaderResult completeLoadTexture(HttpResponse<InputStream> response) {
        // TODO: MIME type?
        LoaderStatus error;
        if (response.statusCode() == 200) {
            try {
                var image = ImageIO.read(response.body());
                var result =
                        new TextureLoaderResult(
                                new BufferedImageTextureHolder(image),
                                response.headers().firstValue("etag").orElse(null),
                                LoaderStatus.OK,
                                null);
                logger.info(
                        String.format("loadTexture %s -> %s", response.uri(), LoaderStatus.OK));
                return result;
            } catch (IOException e) {
                error = LoaderStatus.ERROR_OTHER;
            }
        } else if (response.statusCode() == 400) {
            error = LoaderStatus.ERROR_400;
        } else if (response.statusCode() == 404) {
            error = LoaderStatus.ERROR_404;
        } else {
            error = LoaderStatus.ERROR_OTHER;
        }
        logger.info(String.format("loadTexture %s -> %s", response.uri(), error));
        return new TextureLoaderResult(null, null, error, null);
    }
}
