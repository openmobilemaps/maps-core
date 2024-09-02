package io.openmobilemaps.mapscore;

import com.jogamp.opengl.GLDrawableFactory;
import com.jogamp.opengl.GLProfile;
import com.jogamp.opengl.GLCapabilities;
import com.jogamp.opengl.GLAutoDrawable;
import com.jogamp.opengl.util.GLReadBufferUtil;
import com.jogamp.nativewindow.egl.EGLGraphicsDevice;

import java.io.File;

class GLThread {
  static GLAutoDrawable drawable;

  public static void initFNORD() {
    GLProfile glProfile = GLProfile.getDefault();
    GLCapabilities caps = new GLCapabilities(glProfile);
    caps.setHardwareAccelerated(true);
    caps.setOnscreen(false);
    drawable = GLDrawableFactory.getFactory(glProfile).createOffscreenAutoDrawable(null, caps, null, 300, 300);
    drawable.display();
    drawable.getContext().makeCurrent();
  }

  public static void doneFNORD() {
    try {
      drawable.display();
      var rbu = new GLReadBufferUtil(true, true);
      rbu.readPixels(drawable.getGL(), false);
      rbu.write(new File("frame.png"));
    } finally {
      drawable.destroy();
    }
  }
}
