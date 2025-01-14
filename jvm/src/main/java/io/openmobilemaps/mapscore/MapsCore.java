package io.openmobilemaps.mapscore;

import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

public class MapsCore {
    public static void initialize() {
        final var logger = LoggerFactory.getLogger(MapsCore.class);

        // This won't work for all platforms; it only has to work on the platforms that we build the
        // library for.
        // Currently, that is:
        //  - linux amd64
        final String os = System.getProperty("os.name").toLowerCase();
        final String arch = System.getProperty("os.arch");

        final boolean debug = Boolean.getBoolean("io.openmobilemaps.mapscore.debug");

        final String buildType = debug ? "_debug" : "";
        final String libName = String.format("libmapscore_jni_%s_%s%s.so", os, arch, buildType);
        final String libResourcePath = "/native/" + libName;

        logger.info("Loading native library from resource path {}", libResourcePath);

        try {
            File tempLib = File.createTempFile("libmapscore_jni_", ".so");
            tempLib.deleteOnExit();
            try (InputStream lib = MapsCore.class.getResourceAsStream(libResourcePath)) {
                if (lib == null) {
                    throw new RuntimeException(
                            "Could not load native mapscore library from resource path "
                                    + libResourcePath);
                }
                Files.copy(lib, tempLib.toPath(), StandardCopyOption.REPLACE_EXISTING);
            }
            System.load(tempLib.getAbsolutePath());
        } catch (IOException e) {
            throw new RuntimeException(
                    "Could not load native mapscore library from resource path " + libResourcePath,
                    e);
        }
    }
}
