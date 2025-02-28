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

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;

import javax.imageio.ImageIO;

public abstract class BaseDataLoader extends LoaderInterface {

    protected final Logger logger = LoggerFactory.getLogger(getClass());
    private final ConcurrentHashMap<String, CompletableFuture<DataLoaderResult>> dataLoads =
            new ConcurrentHashMap<>();
    private final ConcurrentHashMap<String, CompletableFuture<TextureLoaderResult>> textureLoads =
            new ConcurrentHashMap<>();

    /**
     * Simplified and combined DataLoaderResult / TextureLoaderResult, as result type for internal
     * combined loadAsync.
     *
     * @param url The url param used to load this data
     * @param data Input stream with the data / texture data. MUST be set if status == OK.
     * @param etag ETag header returned from the server if applicable
     * @param status
     * @param errorCode Optional diagnostics message for error.
     */
    protected record LoadResult(
            String url,
            @Nullable InputStream data,
            @Nullable String etag,
            @NotNull LoaderStatus status,
            @Nullable String errorCode) {
        public LoadResult { }
    }
    ;

    /**
     * Fetch data for loadData/loadDataAsync and, by default, for loadTexture/loadTextureAsync.
     * Override this method in a concrete data loader to get the full DataLoaderInterface.
     *
     * <p>The BaseDataLoader handles generic exceptions in the async CompletionStages, so this may
     * be omitted in subclasses.
     *
     * @param url
     * @param etag
     * @return A future with the internal LoadResult.
     */
    @NotNull
    protected abstract CompletableFuture<LoadResult> fetchData(
            @NotNull String url, @Nullable String etag);

    /**
     * Fetch data for loadTexture / loadTextureAsync. By default, this calls the (abstract)
     * fetchData. Only override this if loading textures requires specific handling, e.g. if they
     * are loaded from a different path.
     *
     * @param url
     * @param etag
     * @return A future with the internal LoadResult.
     */
    @NotNull
    protected CompletableFuture<LoadResult> fetchTexture(
            @NotNull String url, @Nullable String etag) {
        return fetchData(url, etag);
    }

    @NotNull
    @Override
    public Future<DataLoaderResult> loadDataAsync(@NotNull String url, @Nullable String etag) {
        var promise = new Promise<DataLoaderResult>();
        dataLoads.compute(
                url,
                (paramUrl, ongoingLoad) -> {
                    if (ongoingLoad != null) {
                        return ongoingLoad.thenApply(
                                result -> {
                                    promise.setValue(result);
                                    return result;
                                });
                    } else {
                        return fetchData(paramUrl, etag)
                                .thenApplyAsync(this::completeLoadData)
                                .exceptionally(
                                        e -> {
                                            logger.info(
                                                    "loadData {} -> {}",
                                                    paramUrl,
                                                    LoaderStatus.ERROR_OTHER,
                                                    e);
                                            return new DataLoaderResult(
                                                    null,
                                                    null,
                                                    LoaderStatus.ERROR_OTHER,
                                                    e.toString());
                                        })
                                .whenComplete(
                                        (ignoredResult, ignoredException) ->
                                                dataLoads.remove(paramUrl))
                                .thenApply(
                                        result -> {
                                            promise.setValue(result);
                                            return result;
                                        });
                    }
                });
        return promise.getFuture();
    }

    @NotNull
    @Override
    public Future<TextureLoaderResult> loadTextureAsync(
            @NotNull String url, @Nullable String etag) {
        var promise = new Promise<TextureLoaderResult>();
        textureLoads.compute(
                url,
                (paramUrl, ongoingLoad) -> {
                    if (ongoingLoad != null) {
                        return ongoingLoad.thenApply(
                                result -> {
                                    promise.setValue(result);
                                    return result;
                                });
                    } else {
                        return fetchTexture(paramUrl, etag)
                                .thenApplyAsync(this::completeLoadTexture)
                                .exceptionally(
                                        e -> {
                                            logger.info(
                                                    "loadTexture {} -> {}",
                                                    paramUrl,
                                                    LoaderStatus.ERROR_OTHER,
                                                    e);
                                            return new TextureLoaderResult(
                                                    null,
                                                    null,
                                                    LoaderStatus.ERROR_OTHER,
                                                    e.toString());
                                        })
                                .whenComplete(
                                        (ignoredResult, ignoredException) ->
                                                textureLoads.remove(paramUrl))
                                .thenApply(
                                        result -> {
                                            promise.setValue(result);
                                            return result;
                                        });
                    }
                });
        return promise.getFuture();
    }

    @NotNull
    @Override
    public DataLoaderResult loadData(@NotNull String url, @Nullable String etag) {
        try {
            return loadDataAsync(url, etag).get();
        } catch (InterruptedException | ExecutionException e) {
            return new DataLoaderResult(null, null, LoaderStatus.ERROR_OTHER, e.getMessage());
        }
    }

    @NotNull
    @Override
    public TextureLoaderResult loadTexture(@NotNull String url, @Nullable String etag) {
        try {
            return loadTextureAsync(url, etag).get();
        } catch (InterruptedException | ExecutionException e) {
            return new TextureLoaderResult(null, null, LoaderStatus.ERROR_OTHER, e.getMessage());
        }
    }

    // Finalize the data load by converting the data in internal representation with an InputStream
    // into the DataLoaderResult with a natively allocated ByteBuffer.
    private DataLoaderResult completeLoadData(LoadResult loadResult) {
        if (loadResult.status() != LoaderStatus.OK) {
            logger.info(
                    "loadData {} -> {}, {}",
                    loadResult.url(),
                    loadResult.status(),
                    loadResult.errorCode());
            return new DataLoaderResult(
                    null, loadResult.etag(), loadResult.status(), loadResult.errorCode());
        }

        try (InputStream data = loadResult.data()) {
            ByteBuffer buf = null;
            if (data != null) {
                var tmp = data.readAllBytes();
                buf = ByteBuffer.allocateDirect(tmp.length);
                buf.put(tmp);
            }
            logger.info("loadData {} -> {}", loadResult.url(), LoaderStatus.OK);
            return new DataLoaderResult(buf, loadResult.etag(), LoaderStatus.OK, null);
        } catch (IOException | UnsupportedOperationException e) {
            logger.info("loadData {} -> {}", loadResult.url(), LoaderStatus.ERROR_OTHER, e);
            return new DataLoaderResult(null, null, LoaderStatus.ERROR_OTHER, e.getMessage());
        }
    }

    // Finalize the texture load by decoding the image from the data InputStream and wrapping this
    // in a BufferedImageTextureHolder.
    private TextureLoaderResult completeLoadTexture(LoadResult loadResult) {
        if (loadResult.status() != LoaderStatus.OK) {
            logger.info(
                    "loadTexture {} -> {}, {}",
                    loadResult.url(),
                    loadResult.status(),
                    loadResult.errorCode());
            return new TextureLoaderResult(
                    null, loadResult.etag(), loadResult.status(), loadResult.errorCode());
        }

        LoaderStatus error;
        String errorMsg;
        try (InputStream data = loadResult.data()) {
            assert data != null;
            var image = ImageIO.read(data);
            if (image != null) {
                logger.info("loadTexture {} -> {}", loadResult.url(), LoaderStatus.OK);
                return new TextureLoaderResult(
                        new BufferedImageTextureHolder(image),
                        loadResult.etag(),
                        LoaderStatus.OK,
                        null);
            } else {
                error = LoaderStatus.ERROR_OTHER;
                errorMsg = "No image reader can decode data";
            }
        } catch (IOException e) {
            error = LoaderStatus.ERROR_OTHER;
            errorMsg = e.getMessage();
        }
        logger.info(
                "loadTexture {} -> {}, {}", loadResult.url(), LoaderStatus.ERROR_OTHER, errorMsg);
        return new TextureLoaderResult(null, null, error, errorMsg);
    }

    @Override
    public void cancel(@NotNull String url) {
        var ongoingDataLoad = dataLoads.remove(url);
        if (ongoingDataLoad != null) {
            ongoingDataLoad.cancel(true);
        }
        var ongoingTextureLoad = textureLoads.remove(url);
        if (ongoingTextureLoad != null) {
            ongoingTextureLoad.cancel(true);
        }
    }
}
