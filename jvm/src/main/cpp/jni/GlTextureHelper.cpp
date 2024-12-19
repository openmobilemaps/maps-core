#include <GL/gl.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <jni.h>

extern "C" {

JNIEXPORT jint JNICALL Java_io_openmobilemaps_mapscore_graphics_util_GlTextureHelper_createTextureARGB(JNIEnv *env, jclass,
                                                                                                       jintArray data, jint width,
                                                                                                       jint height) {
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const jsize outLen = env->GetArrayLength(data);
    jint *dataBuf = env->GetIntArrayElements(data, NULL);

    // NOTE: little-endian! Implicit byte-order conversion from ARGB ints (highest-order byte is A) into BGRA byte buffer
    // XXX: why isnt this upside down?
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_BGRA, GL_UNSIGNED_BYTE, dataBuf);

    env->ReleaseIntArrayElements(data, dataBuf, JNI_ABORT);

    return (jint)textureId;
}

JNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_graphics_util_GlTextureHelper_deleteTexture(JNIEnv *, jclass,
                                                                                                   jint textureId) {
    GLuint textures[] = {(GLuint)textureId};
    glDeleteTextures(1, textures);
}
}
