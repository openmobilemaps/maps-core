package io.openmobilemaps.mapscore;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.fail;

import io.openmobilemaps.mapscore.map.util.MapTileRenderer;
import io.openmobilemaps.mapscore.map.util.OffscreenMapRenderer;
import io.openmobilemaps.mapscore.map.util.Tiled2dMapVectorLayerBuilder;
import io.openmobilemaps.mapscore.shared.map.MapInterface;
import io.openmobilemaps.mapscore.shared.map.coordinates.Coord;
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemIdentifiers;
import io.openmobilemaps.mapscore.shared.map.coordinates.RectCoord;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;

import javax.imageio.ImageIO;

public class OffscreenMapRendererTest {
    private static final boolean updateGolden = false;

    private static void addTestStyleLayer(MapInterface map) {
        URL styleJsonUrl = OffscreenMapRendererTest.class.getResource("/test-style.json");
        if (styleJsonUrl == null) {
            fail("required resource test-style.json not found.");
        }
        String resourceDir = new File(styleJsonUrl.getPath()).getParent();

        Tiled2dMapVectorLayerBuilder.addFromStyleJson(
                map,
                "name",
                styleJsonUrl.toString(),
                resourceDir,
                OffscreenMapRendererTest.class.getClassLoader(),
                "fonts");
    }

    private static RectCoord bboxCH() {
        // rough bbox of geojson data in in test-style.json
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
        for (int y = 0; y < golden.getHeight(); y++) {
            for (int x = 0; x < golden.getWidth(); x++) {
                assertEquals(golden.getRGB(x, y), actual.getRGB(x, y));
            }
        }
    }

    @BeforeAll
    public static void setUp() {
        MapsCore.initialize();
    }

    @Test
    public void testStyleJson() {
        OffscreenMapRenderer renderer = new OffscreenMapRenderer(1200, 800, 4);

        addTestStyleLayer(renderer.getMap());

        var map = renderer.getMap();
        var cam = map.getCamera();
        cam.moveToBoundingBox(bboxCH(), 0.0f, false, null, null);

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
    public void testTiler() throws OffscreenMapRenderer.MapLayerException {
        OffscreenMapRenderer renderer = new OffscreenMapRenderer(256, 256, 4);

        addTestStyleLayer(renderer.getMap());

        MapTileRenderer tiler = new MapTileRenderer(renderer);

        final int z = 7;
        MapTileRenderer.TileRange tileRange = tiler.getTileRange(z, bboxCH());
        for (int xcol = tileRange.minColumn(); xcol <= tileRange.maxColumn(); xcol++) {
            for (int yrow = tileRange.minRow(); yrow <= tileRange.maxRow(); yrow++) {
                BufferedImage tile = tiler.renderTile(z, xcol, yrow);
                assertImageMatchesGolden(
                        tile, String.format("testTiler_tile_%d_%d_%d", z, xcol, yrow));
            }
        }
    }
}
