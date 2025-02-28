package io.openmobilemaps.mapscore.map.loader;


import io.openmobilemaps.mapscore.shared.map.loader.LoaderStatus;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.concurrent.CompletableFuture;
import java.util.zip.GZIPInputStream;

/**
 * Implementation of LoaderInterface for HTTP/HTTPS resources using the default {@link
 * java.net.http.HttpClient}.
 *
 * <p>Textures are returned in the {@link
 * io.openmobilemaps.mapscore.graphics.BufferedImageTextureHolder} which can load textures for
 * OpenGL.
 */
public class HttpDataLoader extends BaseDataLoader {
    private static final Logger logger = LoggerFactory.getLogger(HttpDataLoader.class);
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

    @Override
    protected CompletableFuture<LoadResult> fetchData(@NotNull String url, @Nullable String etag) {
        final URI uri = URI.create(url);
        if (!isHTTP(uri)) {
            return CompletableFuture.completedFuture(
                    new LoadResult(url, null, null, LoaderStatus.NOOP, null));
        }
        ;
        var request = HttpRequest.newBuilder(uri).build();
        return httpClient
                .sendAsync(request, HttpResponse.BodyHandlers.ofInputStream())
                .thenApply(response -> responseToLoadResult(url, response));
    }

    private LoadResult responseToLoadResult(String url, HttpResponse<InputStream> response) {
        LoaderStatus error;
        String errorMsg = null;
        if (response.statusCode() == 200) {
            try {
                return new LoadResult(
                        url,
                        getDecodedInputStream(response),
                        response.headers().firstValue("etag").orElse(null),
                        LoaderStatus.OK,
                        null);
            } catch (IOException e) {
                error = LoaderStatus.ERROR_OTHER;
                errorMsg = e.getMessage();
            }
        } else if (response.statusCode() == 400) {
            error = LoaderStatus.ERROR_400;
        } else if (response.statusCode() == 404) {
            error = LoaderStatus.ERROR_404;
        } else {
            error = LoaderStatus.ERROR_OTHER;
        }
        return new LoadResult(url, null, null, error, errorMsg);
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
}
