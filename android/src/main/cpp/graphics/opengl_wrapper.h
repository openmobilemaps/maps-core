/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#if defined(__linux__) // also matches __ANDROID__
  #include <GLES3/gl32.h>
  #include <GLES3/gl3ext.h>
#elif defined(__APPLE__)
  #include <OpenGLES/ES1/glext.h>
  #include <OpenGLES/ES2/gl.h>
  #include <OpenGLES/ES2/glext.h>
#elif defined(_WIN32)
  // Enable function definitions in the GL headers below
  #define GL_GLEXT_PROTOTYPES
  // OpenGL ES includes
  #include <GLES2/gl2.h>
  #include <GLES2/gl2ext.h>
  // EGL includes
  #include <EGL/egl.h>
  #include <EGL/eglext.h>
  #include <EGL/eglplatform.h>

  #include <angle_windowsstore.h>
#else
  #error("unsupported platform")
#endif
