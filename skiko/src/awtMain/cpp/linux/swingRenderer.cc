#include <jawt_md.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include "jni_helpers.h"

class OffScreenContext
{
public:
    Display* display;
    GLXContext context;
    GLXFBConfig* fbConfigs;

    OffScreenContext(Display* _display, GLXContext _context, GLXFBConfig* _fbConfigs) {
        display = _display;
        context = _context;
        fbConfigs = _fbConfigs;
    }

    ~OffScreenContext() {
        XFree(fbConfigs);
        glXDestroyContext(display, context);
        XCloseDisplay(display);
    }

    static OffScreenContext* create() {
        const char* envApi = getenv("SKIKO_RENDER_API");
        if (envApi && (strcmp(envApi, "SOFTWARE_COMPAT") == 0 ||
                       strcmp(envApi, "SOFTWARE_FAST") == 0 ||
                       strcmp(envApi, "SOFTWARE") == 0)) {
            return nullptr;
        }

        const int glxContextAttribs[] {
            GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
            GLX_RENDER_TYPE, GLX_RGBA_BIT,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            None
        };

        Display* display = XOpenDisplay(nullptr);

        if (!display) {
            return nullptr;
        }

        // Must verify GLX extension is active on X server before calling glXChooseFBConfig
        int errorBase = 0, eventBase = 0;
        if (!glXQueryExtension(display, &errorBase, &eventBase)) {
            XCloseDisplay(display);
            return nullptr;
        }

        int numConfigs = 0;
        GLXFBConfig* fbConfigs = glXChooseFBConfig(display, DefaultScreen(display), glxContextAttribs, &numConfigs);

        if (!fbConfigs) {
            XCloseDisplay(display);
            return nullptr;
        }

        XVisualInfo* visual = glXGetVisualFromFBConfig(display, fbConfigs[0]);

        if (!visual) {
            XFree(fbConfigs);
            XCloseDisplay(display);
            return nullptr;
        }

        GLXContext context = glXCreateContext(display, visual, nullptr, True);

        XFree(visual);

        if (!context) {
            XFree(fbConfigs);
            XCloseDisplay(display);
            return nullptr;
        }

        return new OffScreenContext(display, context, fbConfigs);
    }
};

class OffScreenBuffer
{
public:
    Display* display;
    GLXPbuffer pbuffer;
    int width;
    int height;

    OffScreenBuffer(Display* _display, GLXPbuffer _pbuffer, int _width, int _height) {
        display = _display;
        pbuffer = _pbuffer;
        width = _width;
        height = _height; 
    }

    ~OffScreenBuffer() {
        if (display && pbuffer) {
            glXDestroyPbuffer(display, pbuffer);
        }
    }

    static OffScreenBuffer* create(OffScreenContext* context, int width, int height) {
        if (!context || !context->display || !context->fbConfigs) {
            return nullptr;
        }

        int pbufferAttribs[] = {
            GLX_PBUFFER_WIDTH, width,
            GLX_PBUFFER_HEIGHT, height,
            None  
        };

        GLXPbuffer pbuffer = glXCreatePbuffer(context->display, context->fbConfigs[0], pbufferAttribs);

        if (!pbuffer) {
            return nullptr; 
        }

        return new OffScreenBuffer(context->display, pbuffer, width, height);
    }
};

class OffScreenTexture
{
public:
    GLuint tex;
    GLuint fbo;

    OffScreenTexture(GLuint _tex, GLuint _fbo) {
        tex = _tex;
        fbo = _fbo;
    }

    ~OffScreenTexture() {
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &tex);
    }
};

extern "C"
{
    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_makeOffScreenContext(
        JNIEnv *env, jobject renderer)
    {
        OffScreenContext* context = OffScreenContext::create();
        return toJavaPointer(context);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_disposeOffScreenContext(
        JNIEnv *env, jobject renderer, jlong contextPtr)
    {
        OffScreenContext* context = fromJavaPointer<OffScreenContext *>(contextPtr);

        if (context) {
            delete context;
        }
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_makeOffScreenBuffer(
        JNIEnv *env, jobject renderer, jlong contextPtr, jlong oldBufferPtr, jint width, jint height)
    {
        OffScreenContext* context = fromJavaPointer<OffScreenContext *>(contextPtr);
        OffScreenBuffer* oldBuffer = fromJavaPointer<OffScreenBuffer *>(oldBufferPtr);
        
        OffScreenBuffer* buffer;
        if (oldBuffer == nullptr || oldBuffer->width != width || oldBuffer->height != height) {
            if (oldBuffer != nullptr) {
                delete oldBuffer;
            }
            buffer = OffScreenBuffer::create(context, width, height);
        } else {
            buffer = oldBuffer;
        }

        return toJavaPointer(buffer);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_disposeOffScreenBuffer(
        JNIEnv *env, jobject renderer, jlong bufferPtr)
    {
        OffScreenBuffer* buffer = fromJavaPointer<OffScreenBuffer *>(bufferPtr);
        if (buffer) {
            delete buffer;
        }
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_startRendering(
        JNIEnv *env, jobject renderer, jlong contextPtr, jlong bufferPtr)
    {
        OffScreenContext* context = fromJavaPointer<OffScreenContext *>(contextPtr);
        OffScreenBuffer* buffer = fromJavaPointer<OffScreenBuffer *>(bufferPtr);

        glXMakeCurrent(context->display, buffer->pbuffer, context->context);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_finishRendering(
        JNIEnv *env, jobject renderer, jlong contextPtr)
    {
        OffScreenContext* context = fromJavaPointer<OffScreenContext *>(contextPtr);
        glXMakeCurrent(context->display, None, nullptr);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_createAndBindTexture(
        JNIEnv *env, jobject renderer, jint width, jint height)
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

        OffScreenTexture *texture = new OffScreenTexture(tex, fbo);

        return toJavaPointer(texture);
    }

    JNIEXPORT GLuint JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_getFboId(
        JNIEnv *env, jobject renderer, jlong texturePtr)
    {
        OffScreenTexture *texture = fromJavaPointer<OffScreenTexture *>(texturePtr);

        return texture->fbo;
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_unbindAndDisposeTexture(
        JNIEnv *env, jobject renderer, jlong texturePtr)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        OffScreenTexture *texture = fromJavaPointer<OffScreenTexture *>(texturePtr);

        if (texture) {
            delete texture;
        }
    }

    // Backwards compatibility aliases for Android Studio / Compose Desktop (Redrawer)
    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRedrawer_makeOffScreenContext(
        JNIEnv *env, jobject redrawer)
    {
        return Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_makeOffScreenContext(env, redrawer);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRedrawer_disposeOffScreenContext(
        JNIEnv *env, jobject redrawer, jlong contextPtr)
    {
        Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_disposeOffScreenContext(env, redrawer, contextPtr);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRedrawer_makeOffScreenBuffer(
        JNIEnv *env, jobject redrawer, jlong contextPtr, jlong oldBufferPtr, jint width, jint height)
    {
        return Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_makeOffScreenBuffer(env, redrawer, contextPtr, oldBufferPtr, width, height);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRedrawer_disposeOffScreenBuffer(
        JNIEnv *env, jobject redrawer, jlong bufferPtr)
    {
        Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_disposeOffScreenBuffer(env, redrawer, bufferPtr);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRedrawer_startRendering(
        JNIEnv *env, jobject redrawer, jlong contextPtr, jlong bufferPtr)
    {
        Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_startRendering(env, redrawer, contextPtr, bufferPtr);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRedrawer_finishRendering(
        JNIEnv *env, jobject redrawer, jlong contextPtr)
    {
        Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_finishRendering(env, redrawer, contextPtr);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRedrawer_createAndBindTexture(
        JNIEnv *env, jobject redrawer, jint width, jint height)
    {
        return Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_createAndBindTexture(env, redrawer, width, height);
    }

    JNIEXPORT GLuint JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRedrawer_getFboId(
        JNIEnv *env, jobject redrawer, jlong texturePtr)
    {
        return Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_getFboId(env, redrawer, texturePtr);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRedrawer_unbindAndDisposeTexture(
        JNIEnv *env, jobject redrawer, jlong texturePtr)
    {
        Java_org_jetbrains_skiko_swing_LinuxOpenGLSwingRenderer_unbindAndDisposeTexture(env, redrawer, texturePtr);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_graphicapi_OpenGLOffscreenContext_makeOffScreenContext(
        JNIEnv *env, jobject contextHandler)
    {
        OffScreenContext* context = OffScreenContext::create();
        return toJavaPointer(context);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_graphicapi_OpenGLOffscreenContext_disposeOffScreenContext(
        JNIEnv *env, jobject contextHandler, jlong contextPtr)
    {
        OffScreenContext* context = fromJavaPointer<OffScreenContext *>(contextPtr);

        if (context) {
            delete context;
        }
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_graphicapi_OpenGLOffscreenContext_makeOffScreenBuffer(
        JNIEnv *env, jobject contextHandler, jlong contextPtr, jlong oldBufferPtr, jint width, jint height)
    {
        OffScreenContext* context = fromJavaPointer<OffScreenContext *>(contextPtr);
        OffScreenBuffer* oldBuffer = fromJavaPointer<OffScreenBuffer *>(oldBufferPtr);

        OffScreenBuffer* buffer;
        if (oldBuffer == nullptr || oldBuffer->width != width || oldBuffer->height != height) {
            if (oldBuffer != nullptr) {
                delete oldBuffer;
            }
            buffer = OffScreenBuffer::create(context, width, height);
        } else {
            buffer = oldBuffer;
        }

        return toJavaPointer(buffer);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_graphicapi_OpenGLOffscreenContext_disposeOffScreenBuffer(
        JNIEnv *env, jobject contextHandler, jlong bufferPtr)
    {
        OffScreenBuffer* buffer = fromJavaPointer<OffScreenBuffer *>(bufferPtr);
        if (buffer) {
            delete buffer;
        }
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_graphicapi_OpenGLOffscreenContext_startRendering(
        JNIEnv *env, jobject contextHandler, jlong contextPtr, jlong bufferPtr)
    {
        OffScreenContext* context = fromJavaPointer<OffScreenContext *>(contextPtr);
        OffScreenBuffer* buffer = fromJavaPointer<OffScreenBuffer *>(bufferPtr);

        glXMakeCurrent(context->display, buffer->pbuffer, context->context);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_graphicapi_OpenGLOffscreenContext_finishRendering(
        JNIEnv *env, jobject contextHandler, jlong contextPtr)
    {
        OffScreenContext* context = fromJavaPointer<OffScreenContext *>(contextPtr);
        glXMakeCurrent(context->display, None, nullptr);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_graphicapi_OpenGLOffscreenContext_createAndBindTexture(
        JNIEnv *env, jobject contextHandler, jint width, jint height)
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

        OffScreenTexture *texture = new OffScreenTexture(tex, fbo);

        return toJavaPointer(texture);
    }

    JNIEXPORT GLuint JNICALL Java_org_jetbrains_skiko_graphicapi_OpenGLOffscreenContext_getFboId(
        JNIEnv *env, jobject contextHandler, jlong texturePtr)
    {
        OffScreenTexture *texture = fromJavaPointer<OffScreenTexture *>(texturePtr);

        return texture->fbo;
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_graphicapi_OpenGLOffscreenContext_unbindAndDisposeTexture(
        JNIEnv *env, jobject contextHandler, jlong texturePtr)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        OffScreenTexture *texture = fromJavaPointer<OffScreenTexture *>(texturePtr);

        if (texture) {
            delete texture;
        }
    }
} // extern "C"
