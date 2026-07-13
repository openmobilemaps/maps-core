package io.openmobilemaps.mapscore;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.fail;

import io.openmobilemaps.mapscore.map.util.OffscreenMapRenderer;
import io.openmobilemaps.mapscore.shared.graphics.common.Color;
import io.openmobilemaps.mapscore.shared.map.Camera3dConfig;
import io.openmobilemaps.mapscore.shared.map.CameraInterpolation;
import io.openmobilemaps.mapscore.shared.map.CameraInterpolationValue;
import io.openmobilemaps.mapscore.shared.map.MapConfig;
import io.openmobilemaps.mapscore.shared.map.MapCamera3dMode;
import io.openmobilemaps.mapscore.shared.map.MapInterface;
import io.openmobilemaps.mapscore.shared.map.coordinates.Coord;
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemIdentifiers;
import io.openmobilemaps.mapscore.shared.map.coordinates.CoordinateSystemFactory;
import io.openmobilemaps.mapscore.shared.map.coordinates.RectCoord;
import io.openmobilemaps.mapscore.shared.map.layers.ColorStateList;
import io.openmobilemaps.mapscore.shared.map.layers.SizeType;
import io.openmobilemaps.mapscore.shared.map.layers.line.LineCapType;
import io.openmobilemaps.mapscore.shared.map.layers.line.LineFactory;
import io.openmobilemaps.mapscore.shared.map.layers.line.LineJoinType;
import io.openmobilemaps.mapscore.shared.map.layers.line.LineLayerInterface;
import io.openmobilemaps.mapscore.shared.map.layers.line.LineStyle;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.awt.image.BufferedImage;
import java.time.Duration;
import java.util.ArrayList;

public class LineRenderingTest {
    private static final int WIDTH = 1200;
    private static final int HEIGHT = 800;
    private static final int NUM_SAMPLES = 4;

    @BeforeAll
    public static void setUp() {
        System.setProperty("io.openmobilemaps.mapscore.debug", "true");
        MapsCore.initialize();
    }

    @Test
    public void testLineStyleMatrix() {
        renderLineStyleMatrix(false, "testLineStyleMatrix");
    }

    @Test
    public void testLineStyleMatrix3d() {
        renderLineStyleMatrix(true, "testLineStyleMatrix3d");
    }

    private static void renderLineStyleMatrix(boolean is3d, String goldenName) {
        OffscreenMapRenderer renderer = createRenderer(is3d);

        var map = renderer.getMap();
        map.setBackgroundColor(rgba(246, 248, 250, 1.0f));
        addLineStyleMatrix(map, is3d);
        map.getCamera().moveToBoundingBox(bboxLineCases(), 0.0f, false, null, null);
        if (is3d) {
            configure3dCamera(map);
        }

        try {
            BufferedImage image = renderer.drawFrame(Duration.ofSeconds(1));
            GoldenImageAssertions.assertImageMatchesGolden(
                    LineRenderingTest.class, image, goldenName);
        } catch (Exception e) {
            fail(e.getMessage());
        } finally {
            renderer.destroy();
        }
    }

    private static OffscreenMapRenderer createRenderer(boolean is3d) {
        if (!is3d) {
            return new OffscreenMapRenderer(WIDTH, HEIGHT, NUM_SAMPLES);
        }

        return new OffscreenMapRenderer(
                WIDTH,
                HEIGHT,
                NUM_SAMPLES,
                new MapConfig(CoordinateSystemFactory.getUnitSphereSystem()),
                2 * 90.714286f,
                true);
    }

    private static void configure3dCamera(MapInterface map) {
        var camera = map.getCamera();
        var camera3d = camera.asMapCamera3d();
        if (camera3d == null) {
            fail("Expected a 3D camera");
        }

        camera3d.setCameraMode(MapCamera3dMode.ORBIT);
        camera3d.setCameraConfig(
                new Camera3dConfig(
                        "line_rendering_test_3d",
                        false,
                        null,
                        0,
                        200_000_000.0f,
                        1_000.0f,
                        constantCameraInterpolation(25.0f),
                        constantCameraInterpolation(0.0f)),
                null,
                null,
                null);
        camera.setRotation(12.0f, false);
    }

    private static CameraInterpolation constantCameraInterpolation(float value) {
        ArrayList<CameraInterpolationValue> stops = new ArrayList<>();
        stops.add(new CameraInterpolationValue(0.0f, value));
        stops.add(new CameraInterpolationValue(1.0f, value));
        return new CameraInterpolation(stops);
    }

    private static RectCoord bboxLineCases() {
        return new RectCoord(
                new Coord(CoordinateSystemIdentifiers.EPSG4326(), -0.3, 7.1, 0.0),
                new Coord(CoordinateSystemIdentifiers.EPSG4326(), 10.3, 0.0, 0.0));
    }

    private static Coord lineCoord(double x, double y) {
        return new Coord(CoordinateSystemIdentifiers.EPSG4326(), x, y, 0.0);
    }

    private static ArrayList<Coord> lineCoords(boolean is3d, double... xy) {
        assertEquals(0, xy.length % 2);
        ArrayList<Coord> coords = new ArrayList<>();
        for (int i = 0; i < xy.length; i += 2) {
            coords.add(lineCoord(xy[i], xy[i + 1]));
        }
        return coords;
    }

    private static ArrayList<Float> dashArray(float... values) {
        ArrayList<Float> result = new ArrayList<>();
        for (float value : values) {
            result.add(value);
        }
        return result;
    }

    private static Color rgba(int red, int green, int blue, float alpha) {
        return new Color(red / 255.0f, green / 255.0f, blue / 255.0f, alpha);
    }

    private static ColorStateList colorState(Color normal) {
        return new ColorStateList(normal, normal);
    }

    private static LineStyle lineStyle(
            Color color,
            Color gapColor,
            float opacity,
            float blur,
            float width,
            ArrayList<Float> dashArray,
            float dashFade,
            float dashAnimationSpeed,
            LineCapType lineCap,
            LineJoinType lineJoin,
            float offset,
            boolean dotted,
            float dottedSkew) {
        return new LineStyle(
                colorState(color),
                colorState(gapColor),
                opacity,
                blur,
                SizeType.SCREEN_PIXEL,
                width,
                dashArray,
                dashFade,
                dashAnimationSpeed,
                lineCap,
                lineJoin,
                offset,
                dotted,
                dottedSkew);
    }

    private static void addLine(
            LineLayerInterface layer,
            String identifier,
            ArrayList<Coord> coordinates,
            LineStyle style) {
        layer.add(LineFactory.createLine(identifier, coordinates, style));
    }

    private static void addLineStyleMatrix(MapInterface map, boolean is3d) {
        LineLayerInterface lineLayer = LineLayerInterface.create();
        map.addLayer(lineLayer.asLayerInterface());

        LineCapType[] caps = {LineCapType.BUTT, LineCapType.ROUND, LineCapType.SQUARE};
        LineJoinType[] joins = {LineJoinType.MITER, LineJoinType.ROUND, LineJoinType.BEVEL};
        Color[] colors = {
            rgba(35, 88, 183, 1.0f),
            rgba(218, 78, 40, 1.0f),
            rgba(33, 145, 80, 1.0f),
            rgba(138, 73, 169, 1.0f),
            rgba(201, 144, 39, 1.0f),
            rgba(24, 129, 141, 1.0f),
            rgba(186, 58, 115, 1.0f),
            rgba(85, 91, 104, 1.0f),
            rgba(95, 124, 24, 1.0f)
        };

        int styleIndex = 0;
        for (int joinIndex = 0; joinIndex < joins.length; joinIndex++) {
            for (int capIndex = 0; capIndex < caps.length; capIndex++) {
                double x = 0.55 + capIndex * 3.25;
                double y = 6.25 - joinIndex * 1.05;
                addLine(
                        lineLayer,
                        "cap_join_" + caps[capIndex] + "_" + joins[joinIndex],
                        lineCoords(is3d, x, y, x + 0.7, y + 0.58, x + 1.4, y),
                        lineStyle(
                                colors[styleIndex++],
                                rgba(0, 0, 0, 0.0f),
                                1.0f,
                                0.0f,
                                17.0f,
                                dashArray(),
                                0.0f,
                                0.0f,
                                caps[capIndex],
                                joins[joinIndex],
                                0.0f,
                                false,
                                1.0f));
            }
        }

        addLine(
                lineLayer,
                "wide_blur",
                lineCoords(is3d, 0.55, 2.9, 2.0, 3.35, 3.45, 2.9),
                lineStyle(
                        rgba(247, 138, 37, 1.0f),
                        rgba(0, 0, 0, 0.0f),
                        1.0f,
                        4.0f,
                        22.0f,
                        dashArray(),
                        0.0f,
                        0.0f,
                        LineCapType.ROUND,
                        LineJoinType.ROUND,
                        0.0f,
                        false,
                        1.0f));

        var offsetLine = lineCoords(is3d, 4.2, 2.9, 5.55, 3.35, 7.2, 2.9);
        addLine(
                lineLayer,
                "offset_positive",
                offsetLine,
                lineStyle(
                        rgba(41, 171, 226, 1.0f),
                        rgba(0, 0, 0, 0.0f),
                        1.0f,
                        0.0f,
                        6.0f,
                        dashArray(),
                        0.0f,
                        0.0f,
                        LineCapType.ROUND,
                        LineJoinType.ROUND,
                        12.0f,
                        false,
                        1.0f));
        addLine(
                lineLayer,
                "offset_negative",
                offsetLine,
                lineStyle(
                        rgba(217, 58, 132, 1.0f),
                        rgba(0, 0, 0, 0.0f),
                        1.0f,
                        0.0f,
                        6.0f,
                        dashArray(),
                        0.0f,
                        0.0f,
                        LineCapType.ROUND,
                        LineJoinType.ROUND,
                        -12.0f,
                        false,
                        1.0f));

        addLine(
                lineLayer,
                "dash_gap_color",
                lineCoords(is3d, 7.85, 2.9, 9.7, 3.35),
                lineStyle(
                        rgba(58, 96, 201, 1.0f),
                        rgba(245, 196, 66, 1.0f),
                        1.0f,
                        0.0f,
                        13.0f,
                        dashArray(1.5f, 0.8f),
                        0.0f,
                        0.0f,
                        LineCapType.SQUARE,
                        LineJoinType.MITER,
                        0.0f,
                        false,
                        1.0f));

        addLine(
                lineLayer,
                "dash_fade",
                lineCoords(is3d, 0.55, 1.7, 3.2, 1.7),
                lineStyle(
                        rgba(34, 132, 88, 1.0f),
                        rgba(255, 255, 255, 0.0f),
                        1.0f,
                        0.0f,
                        13.0f,
                        dashArray(1.0f, 0.7f, 0.35f, 0.7f),
                        0.45f,
                        0.0f,
                        LineCapType.BUTT,
                        LineJoinType.MITER,
                        0.0f,
                        false,
                        1.0f));

        addLine(
                lineLayer,
                "dotted",
                lineCoords(is3d, 3.75, 1.7, 6.4, 1.7),
                lineStyle(
                        rgba(113, 80, 172, 1.0f),
                        rgba(0, 0, 0, 0.0f),
                        1.0f,
                        0.0f,
                        14.0f,
                        dashArray(),
                        0.0f,
                        0.0f,
                        LineCapType.ROUND,
                        LineJoinType.ROUND,
                        0.0f,
                        true,
                        1.0f));

        addLine(
                lineLayer,
                "dotted_skew",
                lineCoords(is3d, 7.0, 1.7, 9.65, 1.7),
                lineStyle(
                        rgba(198, 94, 39, 1.0f),
                        rgba(0, 0, 0, 0.0f),
                        1.0f,
                        0.0f,
                        14.0f,
                        dashArray(),
                        0.0f,
                        0.0f,
                        LineCapType.ROUND,
                        LineJoinType.ROUND,
                        0.0f,
                        true,
                        0.62f));

        addLine(
                lineLayer,
                "opacity",
                lineCoords(is3d, 0.55, 0.75, 3.2, 0.75),
                lineStyle(
                        rgba(25, 96, 154, 1.0f),
                        rgba(0, 0, 0, 0.0f),
                        0.38f,
                        0.0f,
                        20.0f,
                        dashArray(),
                        0.0f,
                        0.0f,
                        LineCapType.SQUARE,
                        LineJoinType.MITER,
                        0.0f,
                        false,
                        1.0f));

        addLine(
                lineLayer,
                "map_unit_width",
                lineCoords(is3d, 3.75, 0.75, 6.4, 0.75),
                new LineStyle(
                        colorState(rgba(82, 131, 53, 1.0f)),
                        colorState(rgba(0, 0, 0, 0.0f)),
                        1.0f,
                        0.0f,
                        SizeType.MAP_UNIT,
                        is3d ? 0.0025f : 18.0f,
                        dashArray(),
                        0.0f,
                        0.0f,
                        LineCapType.ROUND,
                        LineJoinType.ROUND,
                        0.0f,
                        false,
                        1.0f));
    }
}
