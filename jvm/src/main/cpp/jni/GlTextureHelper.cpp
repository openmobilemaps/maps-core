#include <GLES3/gl32.h>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <iostream>

extern "C" {


static GLenum glCheckError_(const char *file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            error = "STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            error = "STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << ":" << line << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

JNIEXPORT jint JNICALL Java_io_openmobilemaps_mapscore_graphics_util_GlTextureHelper_createTextureARGB(JNIEnv *env, jclass, jintArray data, jint width, jint height) {
  GLuint textureId;
  glCheckError();
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glCheckError();

  const jsize outLen = env->GetArrayLength(data);
  jint *dataBuf = env->GetIntArrayElements(data, NULL);

  // NOTE: data as int is RGBA, little endian makes implicit convesion to BGRA.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0,  GL_RGBA,  GL_UNSIGNED_BYTE, dataBuf);
  glCheckError();

  env->ReleaseIntArrayElements(data, dataBuf, JNI_ABORT);

  return (jint)textureId;

}

JNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_graphics_util_GlTextureHelper_deleteTexture(JNIEnv *, jclass, jint textureId) {
  GLuint textures[] = { (GLuint)textureId };
  glDeleteTextures(1, textures);
  glGetError();
}


}
