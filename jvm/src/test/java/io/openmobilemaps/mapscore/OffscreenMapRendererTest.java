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

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.function.Executable;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.time.Duration;
import java.util.ArrayList;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.imageio.ImageIO;

public class OffscreenMapRendererTest {
    private static final boolean updateGolden = false;

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

    // TODO: replace this by/make this into a generic image comparison test
    // TODO: what is a _good_ place to dump the "actual" image? Use perceptual diff instead of
    // simple comparison? Create a diff?
    private static void assertImageMatchesGolden(BufferedImage actual, String name) {
        final String fileName =
                OffscreenMapRendererTest.class.getSimpleName() + "_" + name + ".png";
        // Dump the actual image for analysis and/or updating the golden file.
        try {
            ImageIO.write(actual, "png", new File(fileName));
        } catch (IOException e) {
            // ignore
        }
        if (updateGolden) {
            return;
        }

        String goldenImgResource = "/golden/" + fileName;
        InputStream goldenImgStream =
                OffscreenMapRendererTest.class.getResourceAsStream(goldenImgResource);
        if (goldenImgStream == null) {
            fail("required resource " + goldenImgResource + " not found.");
        }
        BufferedImage golden = null;
        try {
            golden = ImageIO.read(goldenImgStream);

        } catch (IOException e) {
            fail("Could not get ground truth image: " + e.getMessage());
        }

        assertEquals(golden.getHeight(), actual.getHeight());
        assertEquals(golden.getWidth(), actual.getWidth());
        final int w = golden.getWidth();
        final int h = golden.getHeight();
        int[] goldenRGB = golden.getRGB(0, 0, w, h, null, 0, w);
        int[] actualRGB = actual.getRGB(0, 0, w, h, null, 0, w);
        assertArrayEquals(goldenRGB, actualRGB, "Images differ");
    }

    @BeforeAll
    public static void setUp() {
        System.setProperty("io.openmobilemaps.mapscore.debug", "true");
        Logger.getLogger("").setLevel(Level.CONFIG);

        MapsCore.initialize();
    }

    @Test
    public void testStyleJson() {
        OffscreenMapRenderer renderer = new OffscreenMapRenderer(1200, 800, 4);

        var map = renderer.getMap();
        addTestStyleLayer(renderer.getMap(), "style_geojson_ch.json");
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
    public void testStyleJsonLabel() {
        OffscreenMapRenderer renderer = new OffscreenMapRenderer(1200, 800, 4);

        var map = renderer.getMap();
        addTestStyleLayer(map, "style_geojson_ch_label.json");
        map.getCamera().moveToBoundingBox(bboxCH(), 0.0f, false, null, null);

        try {
            BufferedImage ignoredFirstPass = renderer.drawFrame(Duration.ofSeconds(1));
            // XXX: text is broken and only appears on second draw call (again).
            // Either needs proper fix or workarounds in OffscreenMapRenderer.
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

        addTestStyleLayer(renderer.getMap(), "style_geojson_ch.json");

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
}
