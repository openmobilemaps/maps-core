package io.openmobilemaps.mapscore.map.loader;

import com.snapchat.djinni.Future;
import io.openmobilemaps.mapscore.shared.map.loader.DataLoaderResult;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderInterface;
import io.openmobilemaps.mapscore.shared.map.loader.TextureLoaderResult;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.util.logging.Logger;

public class ResourceDataLoader extends LoaderInterface {
    private static final Logger logger = Logger.getLogger(ResourceDataLoader.class.getName());
    private static final ClassLoader classLoader;


    @NotNull
    @Override
    public DataLoaderResult loadData(@NotNull String url, @Nullable String etag) {
        return null;
    }

    @NotNull
    @Override
    public Future<DataLoaderResult> loadDataAsync(@NotNull String url, @Nullable String etag) {
        return null;
    }
    @NotNull
    @Override
    public TextureLoaderResult loadTexture(@NotNull String url, @Nullable String etag) {
        return null;
    }

    @NotNull
    @Override
    public Future<TextureLoaderResult> loadTextureAsync(@NotNull String url, @Nullable String etag) {
        return null;
    }


    @Override
    public void cancel(@NotNull String url) { }
}
