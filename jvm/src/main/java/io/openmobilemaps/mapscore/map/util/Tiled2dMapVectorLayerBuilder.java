package io.openmobilemaps.mapscore.map.util;

import com.snapchat.djinni.Future;
import com.snapchat.djinni.Promise;

import io.openmobilemaps.mapscore.graphics.BufferedImageTextureHolder;
import io.openmobilemaps.mapscore.map.loader.FontLoader;
import io.openmobilemaps.mapscore.map.loader.HttpDataLoader;
import io.openmobilemaps.mapscore.shared.map.MapInterface;
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerInterface;
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerLocalDataProviderInterface;
import io.openmobilemaps.mapscore.shared.map.loader.*;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.awt.image.BufferedImage;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.function.BiFunction;
import java.util.function.Function;

/** Convenience helper to build a style.json layer with the default loaders. */
@SuppressWarnings("unused")
public class Tiled2dMapVectorLayerBuilder {

    private final MapInterface map;
    private String layerName;
    private final ArrayList<LoaderInterface> loaders;
    private Tiled2dMapVectorLayerLocalDataProviderInterface localDataProvider;
    @Nullable private FontLoaderInterface fontLoader;
    @Nullable private String styleJsonData;
    @Nullable private String styleJsonURL;

    public Tiled2dMapVectorLayerBuilder(MapInterface map) {
        this.map = map;
        this.layerName = "unnamed-layer";
        this.loaders = new ArrayList<>();
    }

    public Tiled2dMapVectorLayerInterface build() {
        final boolean localStyleJson = styleJsonData != null;
        final String styleJson = localStyleJson ? styleJsonData : styleJsonURL;

        final FontLoaderInterface fontLoader =
                this.fontLoader != null ? this.fontLoader : new NoFontLoader();

        Tiled2dMapVectorLayerInterface layer =
                Tiled2dMapVectorLayerInterface.createExplicitly(
                        layerName,
                        styleJson,
                        localStyleJson,
                        loaders,
                        fontLoader,
                        localDataProvider,
                        null,
                        null,
                        null);

        map.addLayer(layer.asLayerInterface());
        return layer;
    }

    public Tiled2dMapVectorLayerBuilder withLayerName(String layerName) {
        this.layerName = layerName;
        return this;
    }

    public Tiled2dMapVectorLayerBuilder withHttpLoader() {
        loaders.add(new HttpDataLoader());
        return this;
    }

    public Tiled2dMapVectorLayerBuilder withStyleJsonData(String styleJson) {
        if (styleJson != null && this.styleJsonURL != null) {
            throw new IllegalArgumentException("must not set both style data and style url");
        }
        this.styleJsonData = styleJson;
        return this;
    }

    public Tiled2dMapVectorLayerBuilder withStyleJsonURL(String styleJsonURL) {
        if (styleJsonURL != null && this.styleJsonData != null) {
            throw new IllegalArgumentException("must not set both style data and style url");
        }
        this.styleJsonURL = styleJsonURL;
        return this;
    }

    public Tiled2dMapVectorLayerBuilder withFontLoader(
            ClassLoader classLoader, String fontDirectory, @Nullable String fontFallbackName) {
        final float fontScalingDpi = map.getCamera().getScreenDensityPpi();
        this.fontLoader =
                new FontLoader(fontScalingDpi, classLoader, fontDirectory, fontFallbackName);
        return this;
    }

    public Tiled2dMapVectorLayerBuilder withLocalDataProvider(
            String styleJson,
            Function<Integer, BufferedImage> loadSprite,
            Function<Integer, String> loadSpriteJson,
            BiFunction<String, String, String> loadGeojson) {

        this.localDataProvider =
                new LocalDataProviderAdapter(styleJson, loadSprite, loadSpriteJson, loadGeojson);
        return this;
    }

    private static class NoFontLoader extends FontLoaderInterface {
        @NotNull
        @Override
        public FontLoaderResult loadFont(@NotNull Font font) {
            return new FontLoaderResult(null, null, LoaderStatus.ERROR_404);
        }
    }

    private static class LocalDataProviderAdapter
            extends Tiled2dMapVectorLayerLocalDataProviderInterface {
        private final String styleJson;
        private final Function<Integer, BufferedImage> loadSprite;
        private final Function<Integer, String> loadSpriteJson;
        private final BiFunction<String, String, String> loadGeojson;

        LocalDataProviderAdapter(
                String styleJson,
                Function<Integer, BufferedImage> loadSprite,
                Function<Integer, String> loadSpriteJson,
                BiFunction<String, String, String> loadGeojson) {
            this.styleJson = styleJson;
            this.loadSprite = loadSprite;
            this.loadSpriteJson = loadSpriteJson;
            this.loadGeojson = loadGeojson;
        }

        private static ByteBuffer allocateDirect(String data) {
            if (data == null) {
                return null;
            }
            byte[] encoded = data.getBytes();
            var buf = ByteBuffer.allocateDirect(encoded.length);
            buf.put(encoded);
            return buf;
        }

        private static Future<DataLoaderResult> wrapDataLoaderResult(String data) {
            ByteBuffer buf = allocateDirect(data);
            var p = new Promise<DataLoaderResult>();
            p.setValue(
                    new DataLoaderResult(
                            buf,
                            null,
                            buf != null ? LoaderStatus.OK : LoaderStatus.ERROR_404,
                            null));
            return p.getFuture();
        }

        private static Future<TextureLoaderResult> wrapTextureLoaderResult(BufferedImage image) {
            var p = new Promise<TextureLoaderResult>();
            p.setValue(
                    new TextureLoaderResult(
                            image != null ? new BufferedImageTextureHolder(image) : null,
                            null,
                            image != null ? LoaderStatus.OK : LoaderStatus.ERROR_404,
                            null));
            return p.getFuture();
        }

        @Nullable
        @Override
        public String getStyleJson() {
            return styleJson;
        }

        @NotNull
        @Override
        public Future<TextureLoaderResult> loadSpriteAsync(int scale) {
            BufferedImage image = loadSprite.apply(scale);
            return wrapTextureLoaderResult(image);
        }

        @NotNull
        @Override
        public Future<DataLoaderResult> loadSpriteJsonAsync(int scale) {
            String data = loadSpriteJson.apply(scale);
            return wrapDataLoaderResult(data);
        }

        @NotNull
        @Override
        public Future<DataLoaderResult> loadGeojson(
                @NotNull String sourceName, @NotNull String url) {
            String geoJson = loadGeojson.apply(sourceName, url);
            return wrapDataLoaderResult(geoJson);
        }
    }
}
