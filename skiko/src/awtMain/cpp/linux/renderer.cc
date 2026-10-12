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
        Display *display = fromJavaPointer<Display *>(displayPtr);
        Window window = fromJavaPointer<Window>(windowPtr);

        // according to:
        // https://opengl.gpuinfo.org/listreports.php?extension=GLX_EXT_swap_control
        // https://opengl.gpuinfo.org/listreports.php?extension=GLX_MESA_swap_control
        // https://opengl.gpuinfo.org/listreports.php?extension=GLX_SGI_swap_control
        // there is no Linux that doesn't support at least one of these extensions
        static PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC) glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");
        if (glXSwapIntervalEXT != NULL)
        {
            glXSwapIntervalEXT(display, window, interval);
        }
        else
        {
            static PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESA = (PFNGLXSWAPINTERVALMESAPROC) glXGetProcAddress((const GLubyte*)"glXSwapIntervalMESA");
            if (glXSwapIntervalMESA != NULL)
            {
                glXSwapIntervalMESA(interval);
            }
            else
            {
                static PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC) glXGetProcAddress((const GLubyte*)"glXSwapIntervalSGI");
                if (glXSwapIntervalSGI != NULL)
                {
                    glXSwapIntervalSGI(interval);
                }
            }
        }
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_swapBuffers(JNIEnv *env, jobject renderer, jlong displayPtr, jlong windowPtr)
    {
        Display *display = fromJavaPointer<Display *>(displayPtr);
        Window window = fromJavaPointer<Window>(windowPtr);

        glXSwapBuffers(display, window);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_makeCurrent(JNIEnv *env, jobject renderer, jlong displayPtr, jlong windowPtr, jlong contextPtr)
    {
        Display *display = fromJavaPointer<Display *>(displayPtr);
        Window window = fromJavaPointer<Window>(windowPtr);
        GLXContext *context = fromJavaPointer<GLXContext *>(contextPtr);

        glXMakeCurrent(display, window, *context);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_createContext(JNIEnv *env, jobject renderer, jlong displayPtr, jboolean transparency)
    {
        Display *display = fromJavaPointer<Display *>(displayPtr);
        if (!display) return 0;

        XVisualInfo *vi;

        if (transparency)
        {
            GLint att[] = {
                GLX_RGBA,
                GLX_RED_SIZE, 8,
                GLX_GREEN_SIZE, 8,
                GLX_BLUE_SIZE, 8,
                GLX_ALPHA_SIZE, 8,
                GLX_DOUBLEBUFFER, True, None
            };
            vi = glXChooseVisual(display, 0, att);
        }
        else {
            GLint att[] = {GLX_RGBA, GLX_DOUBLEBUFFER, True, None};
            vi = glXChooseVisual(display, 0, att);
        }

        if (!vi) return 0;

        GLXContext *context = new GLXContext(glXCreateContext(display, vi, NULL, GL_TRUE));
        return toJavaPointer(context);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_renderer_LinuxOpenGLRendererKt_destroyContext(JNIEnv *env, jobject renderer, jlong displayPtr, jlong contextPtr)
    {
        Display *display = fromJavaPointer<Display *>(displayPtr);
        GLXContext *context = fromJavaPointer<GLXContext *>(contextPtr);

        if (display && context) {
            glXDestroyContext(display, *context);
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
