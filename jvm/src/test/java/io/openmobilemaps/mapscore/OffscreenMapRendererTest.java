package io.openmobilemaps.mapscore;

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

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.time.Duration;
import java.util.ArrayList;

import static org.junit.jupiter.api.Assertions.*;

public class OffscreenMapRendererTest {
    private static final boolean updateGolden = Boolean.parseBoolean(System.getProperty("updateGolden", "false"));

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

    private static String addSuffixBeforeExtension(String filename, String suffix) {
        int dotIndex = filename.lastIndexOf('.');
        if (dotIndex == -1) {
            return filename + suffix; // no extension found
        }
        return filename.substring(0, dotIndex) + suffix + filename.substring(dotIndex);
    }


    private static RectCoord bboxCH() {
        // rough bbox of geojson data in in style_geojson_ch.json
        return new RectCoord(
                new Coord(CoordinateSystemIdentifiers.EPSG4326(), 5.9, 47.9, 0.0),
                new Coord(CoordinateSystemIdentifiers.EPSG4326(), 10.5, 45.8, 0.0));
    }

    private static void assertImageMatchesGolden(BufferedImage actual, String name) {
        final String fileName =
                OffscreenMapRendererTest.class.getSimpleName() + "_" + name + ".png";
        
        // Create test-diffs directory for better organization
        File diffDir = new File("test-diffs");
        diffDir.mkdirs();
        
        // Dump the actual image for analysis and/or updating the golden file.
        File actualFile = new File(diffDir, addSuffixBeforeExtension(fileName, "_actual"));
        try {
            ImageIO.write(actual, "png", actualFile);
            System.out.println("Wrote actual image to: " + actualFile.getAbsolutePath());
        } catch (IOException e) {
            System.err.println("Failed to write actual image: " + e.getMessage());
        }
        
        if (updateGolden) {
            // Copy the actual image to resources directory when updating golden
            try {
                File goldenFile = new File(diffDir, addSuffixBeforeExtension(fileName, "_golden"));
                goldenFile.getParentFile().mkdirs();
                ImageIO.write(actual, "png", goldenFile);
                System.out.println("Updated golden image: " + goldenFile.getAbsolutePath());
            } catch (IOException e) {
                System.err.println("Failed to update golden image: " + e.getMessage());
            }
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
        
        // Save golden image for comparison
        File goldenFile = new File(diffDir, fileName + "_golden");
        try {
            ImageIO.write(golden, "png", goldenFile);
            System.out.println("Wrote golden image to: " + goldenFile.getAbsolutePath());
        } catch (IOException e) {
            System.err.println("Failed to write golden image for comparison: " + e.getMessage());
        }
        
        // Create visual diff
        BufferedImage diffImage = createDiffImage(golden, actual);
        File diffFile = new File(diffDir, addSuffixBeforeExtension(fileName, "_diff"));
        try {
            ImageIO.write(diffImage, "png", diffFile);
            System.out.println("Wrote diff image to: " + diffFile.getAbsolutePath());
        } catch (IOException e) {
            System.err.println("Failed to write diff image: " + e.getMessage());
        }
        
        // Count different pixels for a more informative error message
        int differentPixels = 0;
        for (int i = 0; i < goldenRGB.length; i++) {
            if (goldenRGB[i] != actualRGB[i]) {
                differentPixels++;
            }
        }
        
        if (differentPixels > 0) {
            double percentDifferent = (differentPixels * 100.0) / goldenRGB.length;
            String errorMsg = String.format(
                "Images differ: %d pixels (%.2f%%) are different. " +
                "Check artifacts: %s+_actual, %s_golden, %s_diff. " +
                "To approve changes, run: mvn test -DupdateGolden=true",
                differentPixels, percentDifferent, fileName, fileName, fileName
            );
            fail(errorMsg);
        }
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
    public void testStyleJsonPortrait() {
        OffscreenMapRenderer renderer = new OffscreenMapRenderer(400, 700, 4);

        var map = renderer.getMap();
        addTestStyleLayer(renderer.getMap(), "style_geojson_ch.json");
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
        addTestStyleLayer(map, "style_geojson_ch_label.json");
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
