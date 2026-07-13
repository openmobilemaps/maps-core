package io.openmobilemaps.mapscore;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.fail;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.nio.file.Paths;

import javax.imageio.ImageIO;

final class GoldenImageAssertions {
    private static final boolean updateGolden =
            Boolean.parseBoolean(System.getProperty("updateGolden", "false"));

    private GoldenImageAssertions() {}

    static String loadResource(Class<?> testClass, String resourceName) {
        try {
            try (InputStream resource = testClass.getResourceAsStream("/" + resourceName)) {
                if (resource == null) {
                    fail("Resource not found: " + resourceName);
                }
                return new String(resource.readAllBytes());
            }
        } catch (IOException e) {
            return fail("Failed to load resource", e);
        }
    }

    static BufferedImage loadImageResource(Class<?> testClass, String resourceName) {
        try {
            try (InputStream resource = testClass.getResourceAsStream("/" + resourceName)) {
                if (resource == null) {
                    fail("Resource not found: " + resourceName);
                }
                return ImageIO.read(resource);
            }
        } catch (IOException e) {
            return fail("Failed to load image resource", e);
        }
    }

    static void assertImageMatchesReference(
            Class<?> testClass, BufferedImage expected, BufferedImage actual, String name) {
        assertImageMatchesReference(testClass, expected, actual, name, null);
    }

    static void assertImageMatchesReference(
            Class<?> testClass,
            BufferedImage expected,
            BufferedImage actual,
            String name,
            String message) {
        final var actualFile = dumpTestImage(testClass, actual, name, "actual");

        assertEquals(expected.getHeight(), actual.getHeight());
        assertEquals(expected.getWidth(), actual.getWidth());
        final int w = expected.getWidth();
        final int h = expected.getHeight();
        int[] expectedRGB = expected.getRGB(0, 0, w, h, null, 0, w);
        int[] actualRGB = actual.getRGB(0, 0, w, h, null, 0, w);

        final var expectedFile = dumpTestImage(testClass, expected, name, "expected");
        BufferedImage diffImage = createDiffImage(expected, actual);
        final var diffFile = dumpTestImage(testClass, diffImage, name, "diff");

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

    static void assertImageMatchesGolden(Class<?> testClass, BufferedImage actual, String name) {
        final String fileName = testClass.getSimpleName() + "_" + name + ".png";

        if (updateGolden) {
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
        InputStream goldenImgStream = testClass.getResourceAsStream(goldenImgResource);
        if (goldenImgStream == null) {
            dumpTestImage(testClass, actual, name, "actual");
            fail("required resource " + goldenImgResource + " not found.");
        }
        BufferedImage golden = null;
        try {
            golden = ImageIO.read(goldenImgStream);
        } catch (IOException e) {
            dumpTestImage(testClass, actual, name, "actual");
            fail("Could not get ground truth image: " + e.getMessage());
        }

        assertImageMatchesReference(
                testClass, golden, actual, name, "To approve changes, run: mvn test -DupdateGolden=true");
    }

    private static File dumpTestImage(
            Class<?> testClass, BufferedImage image, String name, String state) {
        final String fileName = testClass.getSimpleName() + "_" + name + "_" + state + ".png";

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

    private static BufferedImage createDiffImage(BufferedImage golden, BufferedImage actual) {
        int w = golden.getWidth();
        int h = golden.getHeight();
        BufferedImage diff = new BufferedImage(w, h, BufferedImage.TYPE_INT_RGB);

        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++) {
                int goldenRGB = golden.getRGB(x, y);
                int actualRGB = actual.getRGB(x, y);

                if (goldenRGB == actualRGB) {
                    int gray = (goldenRGB >> 16) & 0xFF;
                    gray = gray / 3;
                    diff.setRGB(x, y, (gray << 16) | (gray << 8) | gray);
                } else {
                    diff.setRGB(x, y, 0xFF0000);
                }
            }
        }
        return diff;
    }
}
