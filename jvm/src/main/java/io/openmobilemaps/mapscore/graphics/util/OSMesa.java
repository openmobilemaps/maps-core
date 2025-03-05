package io.openmobilemaps.mapscore.graphics.util;

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;

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
        // Direct access to DataBuffer is much faster than initializing it via setDataElement or setRGB.
        var dataBuf = (DataBufferInt)out.getRaster().getDataBuffer();
        readARGB(state, dataBuf.getData());
        flipV(dataBuf.getData(), width, height);
        return out;
    }

    private static void flipV(int[] image, int width, int height) {
        for(int yTop = 0; yTop < height / 2; ++yTop) {
            int yBot = height - yTop - 1;
            for (int x = 0; x < width; ++x) {
                int tmpTop = image[yTop * width + x];
                int tmpBot = image[yBot * width + x];
                image[yTop * width + x] = tmpBot;
                image[yBot * width + x] = tmpTop;
            }
        }
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
