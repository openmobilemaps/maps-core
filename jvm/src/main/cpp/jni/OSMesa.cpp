#include <GL/osmesa.h>
#include <cstdlib>
#include <cstring>
#include <jni.h>

extern "C" {

JNIEXPORT jlong JNICALL Java_io_openmobilemaps_mapscore_graphics_util_OSMesa_createContext(JNIEnv *, jclass) {
#if OSMESA_MAJOR_VERSION > 11 || (OSMESA_MAJOR_VERSION == 11 && OSMESA_MINOR_VERSION >= 2)
    int osmesa_attribs[] = {
        OSMESA_FORMAT, OSMESA_BGRA, OSMESA_PROFILE, OSMESA_COMPAT_PROFILE, 0,
    };
    OSMesaContext ctx = OSMesaCreateContextAttribs(osmesa_attribs, nullptr);
#else
    // Just hope
    OSMesaContext ctx = OSMesaCreateContext(OSMESA_BGRA nullptr);
#endif
    return (jlong)ctx; // "type-punning" aka. a lazy hack
}

JNIEXPORT jlong JNICALL Java_io_openmobilemaps_mapscore_graphics_util_OSMesa_makeCurrent(JNIEnv *, jclass, jlong ctxArg, jint width,
                                                                                         jint height) {
    OSMesaContext ctx = (OSMesaContext)ctxArg;
    void *buf = malloc(width * height * 4);
    bool ret = OSMesaMakeCurrent(ctx, buf, GL_UNSIGNED_BYTE, width, height);
    if (!ret) {
        free(buf);
        buf = NULL;
    }
    return (jlong)buf;
}

JNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_graphics_util_OSMesa_readARGB(JNIEnv *env, jclass, jlong bufArg,
                                                                                     jintArray out) {
    void *buf = (void *)bufArg;
    if (buf == NULL) {
        return;
    }

    glFinish();

    // NOTE: little-endian! Implicit byte-order conversion from BGRA byte buffer into ARGB ints (highest-order byte is A)
    const jsize outLen = env->GetArrayLength(out);
    jint *outBuf = env->GetIntArrayElements(out, NULL);
    memcpy(outBuf, buf, sizeof(jint) * outLen);
    env->ReleaseIntArrayElements(out, outBuf, 0);
}

JNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_graphics_util_OSMesa_free(JNIEnv *, jclass, jlong bufArg) {
    void *buf = (void *)bufArg;
    if (buf != NULL) {
        free(buf);
    }
}
}
