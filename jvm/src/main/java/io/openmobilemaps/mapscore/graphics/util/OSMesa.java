package io.openmobilemaps.mapscore.graphics.util;

import java.awt.image.BufferedImage;

/**
 * Simple bindings for OSMesa, the Mesa Off-Screen rendering interface.
 *
 * <p>The interface is somewhat oversimplified for our current use case and does not expose the full
 * functionality of OSMesa.
 */
public class OSMesa {
    private final long state; // Opaque pointer keeping the C-internal state.
    private int width;
    private int height;

    /**
     * Create the OSMesa context. The context needs to be activated with makeCurrent before any GL
     * operations can take place.
     */
    public OSMesa() {
        this.state = createContext();
        if (state == 0) {
            throw new OSMesaException("Could not create OSMesa context");
        }
    }

    /**
     * Create the OSMesa context and activate it immediately.
     *
     * @param numSamples enables multisample anti-aliasing (MSAA). 0 disables MSAA. 4 is maximum.
     */
    public OSMesa(int width, int height, int numSamples) throws OSMesaException {
        this();
        makeCurrent(width, height, numSamples);
    }

    // Analogous BufferedImage.setRGB with a vertical flip.
    // Simplified (startX/Y = 0 and scansize == width).
    private static void setARGBflipV(BufferedImage image, int w, int h, int[] buf) {
        Object pixel = null;
        var colorModel = image.getColorModel();
        var raster = image.getRaster();

        int off = 0;
        for (int y = h - 1; y >= 0; --y) {
            for (int x = 0; x < w; ++x) {
                pixel = colorModel.getDataElements(buf[off++], pixel);
                raster.setDataElements(x, y, pixel);
            }
        }
    }

    private static native long createContext();

    private static native boolean makeCurrent(long state, int width, int height, int numSamples);

    private static native void readARGB(long state, int[] out);

    private static native void destroy(long state);

    /**
     * Activate this OSMesa context for GL operations on this thread and initialize the
     * renderbuffer. This can be called any number of times to modify the framebuffer size.
     */
    public void makeCurrent(int width, int height, int numSamples) throws OSMesaException {
        this.width = width;
        this.height = height;
        if (numSamples < 0 || numSamples > 4) {
            throw new IllegalArgumentException("numSamples must be between 0 and 4");
        }
        boolean ok = OSMesa.makeCurrent(state, width, height, numSamples);
        if (!ok) {
            throw new OSMesaException("Could not activate OSMesa context");
        }
    }

    /** Get the rendered image. */
    // NOTE: in the OSMesa C-API, the image is rendered directly to the
    // user-provided buffer. Currently, we make a couple of copies. (Could be
    // optimized using ByteBuffer).
    public BufferedImage getImage() {
        var out = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
        var tmpARGB = new int[width * height];
        readARGB(state, tmpARGB);
        setARGBflipV(out, width, height, tmpARGB);
        return out;
    }

    public void destroy() {
        destroy(state);
    }

    public static class OSMesaException extends RuntimeException {
        OSMesaException(String reason) {
            super(reason);
        }
    }
}
