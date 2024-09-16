#define GL_GLEXT_PROTOTYPES 1
#include <GL/osmesa.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <jni.h>

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

static bool checkFramebufferStatus() {
    std::string status;
    switch (glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
    case GL_FRAMEBUFFER_COMPLETE:
        return true;
    case GL_FRAMEBUFFER_UNDEFINED:
        status = "GL_FRAMEBUFFER_UNDEFINED";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        status = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        status = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        status = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        status = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        status = "GL_FRAMEBUFFER_UNSUPPORTED";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        status = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        break;
    }
    std::cout << status << std::endl;
    return false;
}

extern "C" {

// Internal state. Instead of setting and fetching these in the Java object, passed up and kept as opaque pointer.
struct JNIOSMesaState {
    OSMesaContext ctx;
    void *buf;

    GLuint width;
    GLuint height;

    // Multisampled framebuffer / renderbuffer objects; if using Multisample Anti-Aliasing.
    GLuint fbo;
    GLuint rbo[2];
};

JNIEXPORT jlong JNICALL Java_io_openmobilemaps_mapscore_graphics_util_OSMesa_createContext(JNIEnv *, jclass) {

    JNIOSMesaState *state = new JNIOSMesaState{};

#if OSMESA_MAJOR_VERSION > 11 || (OSMESA_MAJOR_VERSION == 11 && OSMESA_MINOR_VERSION >= 2)
    int osmesa_attribs[] = {
        OSMESA_FORMAT,
        OSMESA_BGRA,
        OSMESA_PROFILE,
        OSMESA_COMPAT_PROFILE,
        OSMESA_STENCIL_BITS,
        8,
        OSMESA_DEPTH_BITS,
        16,
        OSMESA_ACCUM_BITS,
        16,
        0,
    };
    state->ctx = OSMesaCreateContextAttribs(osmesa_attribs, nullptr);
#elif OSMESA_MAJOR_VERSION > 3 || (OSMESA_MAJOR_VERSION == 3 && OSMESA_MINOR_VERSION >= 5)
    state->ctx = OSMesaCreateContextExt(OSMESA_BGRA, 16, 8, 16, nullptr);
#else
    // Just hope
    state->ctx = SMesaContext ctx = OSMesaCreateContext(OSMESA_BGRA, nullptr);
#endif

    return (jlong)state; // "type-punning" aka. a lazy hack
}

JNIEXPORT jlong JNICALL Java_io_openmobilemaps_mapscore_graphics_util_OSMesa_makeCurrent(JNIEnv *, jclass, jlong stateArg,
                                                                                         jint width, jint height, jint numSamples) {
    JNIOSMesaState *state = (JNIOSMesaState *)stateArg;
    state->buf = realloc(state->buf, width * height * 4);
    if (state->buf == NULL) {
        return false;
    }
    state->width = width;
    state->height = height;
    bool ret = OSMesaMakeCurrent(state->ctx, state->buf, GL_UNSIGNED_BYTE, width, height);
    if (!ret) {
        return false;
    }

    glDeleteFramebuffers(1, &state->fbo);
    glDeleteRenderbuffers(2, state->rbo);
    if (numSamples == 0) {
        return true;
    }

    glGenRenderbuffers(2, state->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, state->rbo[0]);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, numSamples, GL_RGBA, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, state->rbo[1]);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, numSamples, GL_DEPTH24_STENCIL8, width, height);

    glGenFramebuffers(1, &state->fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state->fbo);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, state->rbo[0]);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, state->rbo[1]);
    return checkFramebufferStatus();
}

JNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_graphics_util_OSMesa_readARGB(JNIEnv *env, jclass, jlong stateArg,
                                                                                     jintArray out) {
    JNIOSMesaState *state = (JNIOSMesaState *)stateArg;

    glFinish();

    if (state->fbo != 0) {
        // For the multisampled draw buffer, blit (down-sample) to the default framebuffer.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, state->fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, state->width, state->height, 0, 0, state->width, state->height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state->fbo);
        glCheckError();
    }

    // NOTE: little-endian! Implicit byte-order conversion from BGRA byte buffer into ARGB ints (highest-order byte is A)
    const jsize outLen = env->GetArrayLength(out);
    jint *outBuf = env->GetIntArrayElements(out, NULL);

    // memcpy(outBuf, buf, sizeof(jint) * outLen); // NOTE for whatever reason, the blit above does not seem to directly affect
    // state->buf as I expected with OSMesa. Need to read pixel instead.
    glReadPixels(0, 0, state->width, state->width, GL_BGRA, GL_UNSIGNED_BYTE, outBuf);
    glCheckError();
    env->ReleaseIntArrayElements(out, outBuf, 0);
}

JNIEXPORT void JNICALL Java_io_openmobilemaps_mapscore_graphics_util_OSMesa_destroy(JNIEnv *, jclass, jlong stateArg) {
    JNIOSMesaState *state = (JNIOSMesaState *)stateArg;

    OSMesaDestroyContext(state->ctx);
    free(state->buf);
    glDeleteFramebuffers(1, &state->fbo);
    glDeleteRenderbuffers(2, state->rbo);

    delete (state);
}
}
