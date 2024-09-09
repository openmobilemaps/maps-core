package io.openmobilemaps.mapscore;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

public class MapsCore {
  public static void initialize() throws IOException {
    File tempLib = File.createTempFile("libmapscore_jni_", ".so");
    tempLib.deleteOnExit();
    InputStream lib = MapsCore.class.getResourceAsStream("/native/libmapscore_jni.so");
    Files.copy(lib, tempLib.toPath(), StandardCopyOption.REPLACE_EXISTING);
    
    System.load(tempLib.getAbsolutePath());
  }
}
