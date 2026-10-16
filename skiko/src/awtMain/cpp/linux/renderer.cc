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

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);

extern "C"
{
    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_setSwapInterval(JNIEnv *env, jobject renderer, jlong displayPtr, jlong windowPtr, jint interval)
    {
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_swapBuffers(JNIEnv *env, jobject renderer, jlong displayPtr, jlong windowPtr)
    {
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_makeCurrent(JNIEnv *env, jobject renderer, jlong displayPtr, jlong windowPtr, jlong contextPtr)
    {
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_createContext(JNIEnv *env, jobject renderer, jlong displayPtr, jboolean transparency)
    {
        // On Android Bionic / Termux-X11, GLX is not supported.
        // Returning 0L prompts Skiko to throw RenderException and fall back to SoftwareRenderer.
        return 0L;
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_destroyContext(JNIEnv *env, jobject renderer, jlong displayPtr, jlong contextPtr)
    {
        GLXContext *context = fromJavaPointer<GLXContext *>(contextPtr);
        if (context) {
            delete context;
        }
    }

    // Backwards compatibility aliases for Android Studio / Compose Desktop (Redrawer)
    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_LinuxOpenGLRedrawerKt_setSwapInterval(JNIEnv *env, jobject redrawer, jlong displayPtr, jlong windowPtr, jint interval)
    {
        Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_setSwapInterval(env, redrawer, displayPtr, windowPtr, interval);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_LinuxOpenGLRedrawerKt_swapBuffers(JNIEnv *env, jobject redrawer, jlong displayPtr, jlong windowPtr)
    {
        Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_swapBuffers(env, redrawer, displayPtr, windowPtr);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_LinuxOpenGLRedrawerKt_makeCurrent(JNIEnv *env, jobject redrawer, jlong displayPtr, jlong windowPtr, jlong contextPtr)
    {
        Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_makeCurrent(env, redrawer, displayPtr, windowPtr, contextPtr);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_redrawer_LinuxOpenGLRedrawerKt_createContext(JNIEnv *env, jobject redrawer, jlong displayPtr, jboolean transparency)
    {
        return Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_createContext(env, redrawer, displayPtr, transparency);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_LinuxOpenGLRedrawerKt_destroyContext(JNIEnv *env, jobject redrawer, jlong displayPtr, jlong contextPtr)
    {
        Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_destroyContext(env, redrawer, displayPtr, contextPtr);
    }
}
