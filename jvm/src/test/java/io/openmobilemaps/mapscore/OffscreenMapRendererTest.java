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
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import javax.imageio.ImageIO;

public class OffscreenMapRendererTest {
    private static final boolean updateGolden =
            Boolean.parseBoolean(System.getProperty("updateGolden", "false"));

    private static String loadResource(String resourceName) {
        try {
            try (InputStream resource =
                    OffscreenMapRendererTest.class.getResourceAsStream("/" + resourceName)) {
                if (resource == null) {
                    fail("Resource not found: " + resourceName);
                }
                return new String(resource.readAllBytes());
            }
        } catch (IOException e) {
            return fail("Failed to load resource", e);
        }
    }

    private static BufferedImage loadImageResource(String resourceName) {
        try {
            try (InputStream resource =
                    OffscreenMapRendererTest.class.getResourceAsStream("/" + resourceName)) {
                if (resource == null) {
                    fail("Resource not found: " + resourceName);
                }
                return ImageIO.read(resource);
            }
        } catch (IOException e) {
            return fail("Failed to load image resource", e);
        }
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

    private static File dumpTestImage(BufferedImage image, String name, String state) {
        final String fileName =
                OffscreenMapRendererTest.class.getSimpleName() + "_" + name + "_" + state + ".png";

        // Create test-diffs directory for better organization
        File diffDir = new File("test-diffs");
        diffDir.mkdirs();

        File file = new File(diffDir, fileName);
        try {
            ImageIO.write(image, "png", file);
            System.out.println("Wrote " + state + " image to: " + file.getAbsolutePath());
        } catch (IOException e) {
            System.err.println("Failed to write image: " + e.getMessage());
        }
        return file;
    }

    private static void assertImageMatchesReference(
            BufferedImage expected, BufferedImage actual, String name) {
        assertImageMatchesReference(expected, actual, name, null);
    }

    private static void assertImageMatchesReference(
            BufferedImage expected, BufferedImage actual, String name, String message) {
        final String fileName =
                OffscreenMapRendererTest.class.getSimpleName() + "_" + name + ".png";

        // Dump the actual image for analysis.
        final var actualFile = dumpTestImage(actual, name, "actual");

        assertEquals(expected.getHeight(), actual.getHeight());
        assertEquals(expected.getWidth(), actual.getWidth());
        final int w = expected.getWidth();
        final int h = expected.getHeight();
        int[] expectedRGB = expected.getRGB(0, 0, w, h, null, 0, w);
        int[] actualRGB = actual.getRGB(0, 0, w, h, null, 0, w);

        // Save golden image for comparison
        final var expectedFile = dumpTestImage(expected, name, "expected");

        // Create visual diff
        BufferedImage diffImage = createDiffImage(expected, actual);
        final var diffFile = dumpTestImage(diffImage, name, "diff");

        // Count different pixels for a more informative error message
        int differentPixels = 0;
        for (int i = 0; i < expectedRGB.length; i++) {
            if (expectedRGB[i] != actualRGB[i]) {
                differentPixels++;
            }
        }

        if (differentPixels > 0) {
            double percentDifferent = (differentPixels * 100.0) / expectedRGB.length;
            String errorMsg =
                    String.format(
                            """
                            Images differ: %d pixels (%.2f%%) are different.
                            Check artifacts: %s, %s, %s.
                            """,
                            differentPixels,
                            percentDifferent,
                            actualFile.getName(),
                            expectedFile.getName(),
                            diffFile.getName());
            if (message != null) {
                errorMsg += "\n" + message;
            }
            fail(errorMsg);
        }
    }

    private static void assertImageMatchesGolden(BufferedImage actual, String name) {
        final String fileName =
                OffscreenMapRendererTest.class.getSimpleName() + "_" + name + ".png";

        if (updateGolden) {
            // Copy the actual image to resources directory when updating golden
            // NOTE: path is relative to current working directory.
            try {
                Path goldenResource = Paths.get("src", "test", "resources", "golden", fileName);
                goldenResource.getParent().toFile().mkdirs();
                ImageIO.write(actual, "png", goldenResource.toFile());
                System.out.println("Updated golden image: " + goldenResource.toAbsolutePath());
            } catch (IOException e) {
                System.err.println("Failed to update golden image: " + e.getMessage());
            }
            return;
        }

        String goldenImgResource = "/golden/" + fileName;
        InputStream goldenImgStream =
                OffscreenMapRendererTest.class.getResourceAsStream(goldenImgResource);
        if (goldenImgStream == null) {
            dumpTestImage(actual, name, "actual");
            fail("required resource " + goldenImgResource + " not found.");
        }
        BufferedImage golden = null;
        try {
            golden = ImageIO.read(goldenImgStream);
        } catch (IOException e) {
            dumpTestImage(actual, name, "actual");
            fail("Could not get ground truth image: " + e.getMessage());
        }

        assertImageMatchesReference(
                golden, actual, name, "To approve changes, run: mvn test -DupdateGolden=true");
    }

    private static BufferedImage createDiffImage(BufferedImage golden, BufferedImage actual) {
        int w = golden.getWidth();
        int h = golden.getHeight();
        BufferedImage diff = new BufferedImage(w, h, BufferedImage.TYPE_INT_RGB);

        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++) {
                int goldenRGB = golden.getRGB(x, y);
                int actualRGB = actual.getRGB(x, y);

                if (goldenRGB == actualRGB) {
                    // Same pixel - show in grayscale
                    int gray = (goldenRGB >> 16) & 0xFF; // Use red component
                    gray = gray / 3; // Darken for better contrast with differences
                    diff.setRGB(x, y, (gray << 16) | (gray << 8) | gray);
                } else {
                    // Different pixel - highlight in red
                    diff.setRGB(x, y, 0xFF0000);
                }
            }
        }
        return diff;
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
                                            null))));

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
