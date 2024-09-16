#include <GL/osmesa.h>
#include <cstdlib>
#include <cstring>
#include <jni.h>

extern "C" {

JNIEXPORT jlong JNICALL Java_io_openmobilemaps_mapscore_graphics_util_OSMesa_createContext(JNIEnv *, jclass) {
#if OSMESA_MAJOR_VERSION > 11 || (OSMESA_MAJOR_VERSION == 11 && OSMESA_MINOR_VERSION >= 2)
    int osmesa_attribs[] = {
        OSMESA_FORMAT, OSMESA_BGRA, 
        OSMESA_PROFILE, OSMESA_COMPAT_PROFILE,
        OSMESA_STENCIL_BITS, 8,
        OSMESA_DEPTH_BITS, 16,
        OSMESA_ACCUM_BITS, 16,
        0,
    };
    OSMesaContext ctx = OSMesaCreateContextAttribs(osmesa_attribs, nullptr);
#elif OSMESA_MAJOR_VERSION > 3 || (OSMESA_MAJOR_VERSION == 3 && OSMESA_MINOR_VERSION >= 5) 
    OSMesaContext ctx = OSMesaCreateContextExt(OSMESA_BGRA, 16, 8, 16, nullptr);
#else
    // Just hope
    OSMesaContext ctx = OSMesaCreateContext(OSMESA_BGRA, nullptr);
#endif
    return (jlong)ctx; // "type-punning" aka. a lazy hack
}

JNIEXPORT jlong JNICALL Java_io_openmobilemaps_mapscore_graphics_util_OSMesa_makeCurrent(JNIEnv *, jclass, jlong ctxArg, jlong bufArg, jint width,
                                                                                         jint height) {
    OSMesaContext ctx = (OSMesaContext)ctxArg;
    void *buf = (void *)bufArg;
    buf = realloc(buf, width * height * 4);
    if (buf == NULL) {
      return (jlong)buf;
    }
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

JNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_graphics_util_OSMesa_destroy(JNIEnv *, jclass, jlong ctxArg, jlong bufArg) {
    OSMesaContext ctx = (OSMesaContext)ctxArg;
    OSMesaDestroyContext(ctx);

    void *buf = (void *)bufArg;
    if (buf != NULL) {
        free(buf);
    }
}
}
