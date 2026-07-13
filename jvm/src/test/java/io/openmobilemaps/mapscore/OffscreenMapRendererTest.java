package io.openmobilemaps.mapscore;

import static org.junit.jupiter.api.Assertions.*;

import io.openmobilemaps.mapscore.map.util.MapTileRenderer;
import io.openmobilemaps.mapscore.map.util.OffscreenMapRenderer;
import io.openmobilemaps.mapscore.map.util.Tiled2dMapVectorLayerBuilder;
import io.openmobilemaps.mapscore.shared.map.MapInterface;
import io.openmobilemaps.mapscore.shared.map.coordinates.Coord;
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemIdentifiers;
import io.openmobilemaps.mapscore.shared.map.coordinates.RectCoord;
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.Tiled2dMapVectorLayerInterface;
import io.openmobilemaps.mapscore.shared.map.layers.tiled.vector.VectorLayerFeatureInfoValue;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.function.Executable;

import java.awt.image.BufferedImage;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class OffscreenMapRendererTest {
    private static String loadResource(String resourceName) {
        return GoldenImageAssertions.loadResource(OffscreenMapRendererTest.class, resourceName);
    }

    private static BufferedImage loadImageResource(String resourceName) {
        return GoldenImageAssertions.loadImageResource(
                OffscreenMapRendererTest.class, resourceName);
    }

    private static Tiled2dMapVectorLayerInterface addTestStyleLayer(
            MapInterface map, String styleJsonFile) {

        String styleJsonData = loadResource(styleJsonFile);
        return new Tiled2dMapVectorLayerBuilder(map)
                .withLayerName("test-layer")
                .withFontLoader(
                        OffscreenMapRendererTest.class.getClassLoader(), "fonts", "Roboto-Regular")
                .withStyleJsonData(styleJsonData)
                .build();
    }

    private static RectCoord bboxCH() {
        // rough bbox of geojson data in in style_geojson_ch.json
        return new RectCoord(
                new Coord(CoordinateSystemIdentifiers.EPSG4326(), 5.9, 47.9, 0.0),
                new Coord(CoordinateSystemIdentifiers.EPSG4326(), 10.5, 45.8, 0.0));
    }

    private static void assertImageMatchesReference(
            BufferedImage expected, BufferedImage actual, String name) {
        GoldenImageAssertions.assertImageMatchesReference(
                OffscreenMapRendererTest.class, expected, actual, name);
    }

    private static void assertImageMatchesGolden(BufferedImage actual, String name) {
        GoldenImageAssertions.assertImageMatchesGolden(
                OffscreenMapRendererTest.class, actual, name);
    }

    @BeforeAll
    public static void setUp() {
        System.setProperty("io.openmobilemaps.mapscore.debug", "true");

        MapsCore.initialize();
    }

    @Test
    public void testStyleJson() {
        OffscreenMapRenderer renderer = new OffscreenMapRenderer(1200, 800, 4);

        var map = renderer.getMap();
        addTestStyleLayer(renderer.getMap(), "styles/style_geojson_ch.json");
        map.getCamera().moveToBoundingBox(bboxCH(), 0.0f, false, null, null);

        try {
            BufferedImage image = renderer.drawFrame();
            assertImageMatchesGolden(image, "testStyleJson");
        } catch (Exception e) {
            fail(e.getMessage());
        } finally {
            renderer.destroy();
        }
    }

    @Test
    public void testStyleJsonPortrait() {
        OffscreenMapRenderer renderer = new OffscreenMapRenderer(400, 700, 4);

        var map = renderer.getMap();
        addTestStyleLayer(renderer.getMap(), "styles/style_geojson_ch.json");
        map.getCamera().moveToBoundingBox(bboxCH(), 0.0f, false, null, null);

        try {
            BufferedImage image = renderer.drawFrame();
            assertImageMatchesGolden(image, "testStyleJsonPortrait");
        } catch (Exception e) {
            fail(e.getMessage());
        } finally {
            renderer.destroy();
        }
    }

    @Test
    public void testStyleJsonLabel() {
        OffscreenMapRenderer renderer = new OffscreenMapRenderer(1200, 800, 4);

        var map = renderer.getMap();
        addTestStyleLayer(map, "styles/style_geojson_ch_label.json");
        map.getCamera().moveToBoundingBox(bboxCH(), 0.0f, false, null, null);

        try {
            BufferedImage image = renderer.drawFrame(Duration.ofSeconds(1));
            assertImageMatchesGolden(image, "testStyleJsonLabel");
        } catch (Exception e) {
            fail(e.getMessage());
        } finally {
            renderer.destroy();
        }
    }

    @Test
    public void testTiler() {
        OffscreenMapRenderer renderer = new OffscreenMapRenderer(256, 256, 4);

        addTestStyleLayer(renderer.getMap(), "styles/style_geojson_ch.json");

        MapTileRenderer tiler = new MapTileRenderer(renderer);

        try {
            final int z = 7;
            MapTileRenderer.TileRange tileRange = tiler.getTileRange(z, bboxCH());
            ArrayList<Executable> subtests = new ArrayList<>();
            for (int xcol = tileRange.minColumn(); xcol <= tileRange.maxColumn(); xcol++) {
                for (int yrow = tileRange.minRow(); yrow <= tileRange.maxRow(); yrow++) {
                    final int finalXcol = xcol;
                    final int finalYrow = yrow;
                    subtests.add(
                            () -> {
                                BufferedImage tile = tiler.renderTile(z, finalXcol, finalYrow);
                                assertImageMatchesGolden(
                                        tile,
                                        String.format(
                                                "testTiler_tile_%d_%d_%d",
                                                z, finalXcol, finalYrow));
                            });
                }
            }
            assertAll(subtests);
        } finally {
            renderer.destroy();
        }
    }

    @Test
    public void testSprites() {
        sharedSpriteTest("testSprites", true);
    }

    @Test
    public void testSpritesMissingSheet() {
        sharedSpriteTest("testSpritesMissingSheet", false);
    }

    private void sharedSpriteTest(String name, boolean loadLightSprites) {
        OffscreenMapRenderer renderer = new OffscreenMapRenderer(1200, 800, 4);

        var map = renderer.getMap();
        var layer =
                new Tiled2dMapVectorLayerBuilder(map)
                        .withLayerName("test-layer")
                        .withFontLoader(
                                OffscreenMapRendererTest.class.getClassLoader(),
                                "fonts",
                                "Roboto-Regular")
                        .withLocalDataProvider(
                                loadResource("styles/multisprite/style.json"),
                                (spriteId, url, scale) ->
                                        switch (spriteId) {
                                            case "default" ->
                                                    loadImageResource(
                                                            "styles/multisprite/sprite.png");
                                            case "light" ->
                                                    loadLightSprites
                                                            ? loadImageResource(
                                                                    "styles/multisprite/lightbasemap.png")
                                                            : null;
                                            default -> null;
                                        },
                                (spriteId, url, scale) ->
                                        switch (spriteId) {
                                            case "default" ->
                                                    loadResource("styles/multisprite/sprite.json");
                                            case "light" ->
                                                    loadLightSprites
                                                            ? loadResource(
                                                                    "styles/multisprite/lightbasemap.json")
                                                            : null;
                                            default -> null;
                                        },
                                (sourceName, url) ->
                                        switch (sourceName) {
                                            case "country_ch" -> loadResource("ch.geojson");
                                            default -> null;
                                        })
                        .build();
        var camera = map.getCamera();
        camera.moveToBoundingBox(bboxCH(), 0.0f, false, null, null);
        final var initialZoom = camera.getZoom();

        try {
            BufferedImage baseFrame = renderer.drawFrame();
            assertImageMatchesGolden(baseFrame, name);

            // Set and reset state
            layer.setGlobalState(
                    new HashMap<>(
                            Map.of(
                                    "state-icon-image",
                                    new VectorLayerFeatureInfoValue(
                                            "default:hazard",
                                            null,
                                            null,
                                            null,
                                            null,
                                            null,
                                            null),
                                    "state-toggle",
                                    new VectorLayerFeatureInfoValue(
                                            null,
                                            null,
                                            null,
                                            true,
                                            null,
                                            null,
                                            null)
                                    )));

            var image = renderer.drawFrame();
            assertImageMatchesGolden(image, name + "_state");

            layer.setGlobalState(new HashMap<>());
            image = renderer.drawFrame();
            assertImageMatchesReference(baseFrame, image, name + "_state_reset");

            // Zoom in and back out
            camera.setZoom(1_000_000.0, false);
            image = renderer.drawFrame();
            assertImageMatchesGolden(image, name + "_zoom");

            camera.setZoom(initialZoom, false);
            image = renderer.drawFrame();
            assertImageMatchesReference(baseFrame, image, name +"_zoom_reset");

            // Rotate a little
            camera.setRotation(45.0f, false);
            image = renderer.drawFrame();
            assertImageMatchesGolden(image, name + "_rotate");

            camera.setRotation(0, false);
            image = renderer.drawFrame();
            assertImageMatchesReference(baseFrame, image, name + "_rotate_reset");

            // Pause/resume
            map.pause();
            map.resume();
            image = renderer.drawFrame();
            assertImageMatchesReference(baseFrame, image, name + "_pause-resume");

        } catch (Exception e) {
            fail(e.getMessage());
        } finally {
            renderer.destroy();
        }
    }
}
