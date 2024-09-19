package io.openmobilemaps.mapscore;

import java.util.logging.Logger;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

public class MapsCore {
  public static void initialize() {
    final var logger = Logger.getLogger(MapsCore.class.getName());

    // This wont work for all platforms; it only has to work for the platforms that we build the library.
    // Currently, that is:
    //  - linux amd64
    final String os = System.getProperty("os.name").toLowerCase();
    final String arch = System.getProperty("os.arch");

    final boolean debug = Boolean.getBoolean("io.openmobilemaps.mapscore.debug");

    final String buildType = debug ? "_debug" : "";
    final String libName = String.format("libmapscore_jni_%s_%s%s.so", os, arch, buildType);
    final String libResourcePath = "/native/" + libName;

    logger.config("Loading native library from resource path " + libResourcePath);

    try {
      File tempLib = File.createTempFile("libmapscore_jni_", ".so");
      tempLib.deleteOnExit();
      InputStream lib = MapsCore.class.getResourceAsStream(libResourcePath);
      Files.copy(lib, tempLib.toPath(), StandardCopyOption.REPLACE_EXISTING);

      System.load(tempLib.getAbsolutePath());
    } catch (IOException e) {
      throw new RuntimeException("Could not load native mapscore library", e);
    }
  }
}
