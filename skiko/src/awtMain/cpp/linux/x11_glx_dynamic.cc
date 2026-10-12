#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "SkikoBionic"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

static void* get_libx11() {
    static void* handle = nullptr;
    if (handle) return handle;
    const char* paths[] = {
        "libX11.so",
        "libX11.so.6",
        "/data/data/com.termux/files/usr/lib/libX11.so",
        "/data/data/com.termux/files/usr/lib/libX11.so.6",
        nullptr
    };
    for (int i = 0; paths[i]; i++) {
        handle = dlopen(paths[i], RTLD_LAZY | RTLD_GLOBAL);
        if (handle) break;
    }
    return handle;
}

static void* get_libgl() {
    static void* handle = nullptr;
    if (handle) return handle;
    const char* paths[] = {
        "libGL.so",
        "libGL.so.1",
        "libGLX.so",
        "libGLX.so.0",
        "/data/data/com.termux/files/usr/lib/libGL.so",
        "/data/data/com.termux/files/usr/lib/libGL.so.1",
        nullptr
    };
    for (int i = 0; paths[i]; i++) {
        handle = dlopen(paths[i], RTLD_LAZY | RTLD_GLOBAL);
        if (handle) break;
    }
    return handle;
}

extern "C" {

// 1. XCloseDisplay
int XCloseDisplay(Display *display) {
    if (!display) return 0;
    typedef int (*fn_t)(Display*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XCloseDisplay");
    }
    return fn ? fn(display) : 0;
}

// 2. XCreateGC
GC XCreateGC(Display *display, Drawable d, unsigned long valuemask, XGCValues *values) {
    if (!display) return nullptr;
    typedef GC (*fn_t)(Display*, Drawable, unsigned long, XGCValues*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XCreateGC");
    }
    return fn ? fn(display, d, valuemask, values) : nullptr;
}

// 3. XFree
int XFree(void *data) {
    if (!data) return 0;
    typedef int (*fn_t)(void*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XFree");
    }
    return fn ? fn(data) : 0;
}

// 4. XFreeGC
int XFreeGC(Display *display, GC gc) {
    if (!display || !gc) return 0;
    typedef int (*fn_t)(Display*, GC);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XFreeGC");
    }
    return fn ? fn(display, gc) : 0;
}

// 5. XGetGeometry
Status XGetGeometry(Display *display, Drawable d, Window *root_return, int *x_return, int *y_return,
                    unsigned int *width_return, unsigned int *height_return,
                    unsigned int *border_width_return, unsigned int *depth_return) {
    typedef Status (*fn_t)(Display*, Drawable, Window*, int*, int*, unsigned int*, unsigned int*, unsigned int*, unsigned int*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XGetGeometry");
    }
    return fn ? fn(display, d, root_return, x_return, y_return, width_return, height_return, border_width_return, depth_return) : 0;
}

// 6. XGetWindowAttributes
Status XGetWindowAttributes(Display *display, Window w, XWindowAttributes *window_attributes_return) {
    typedef Status (*fn_t)(Display*, Window, XWindowAttributes*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XGetWindowAttributes");
    }
    return fn ? fn(display, w, window_attributes_return) : 0;
}

// 7. XInitImage
Status XInitImage(XImage *image) {
    typedef Status (*fn_t)(XImage*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XInitImage");
    }
    return fn ? fn(image) : 0;
}

// 8. XOpenDisplay
Display *XOpenDisplay(_Xconst char *display_name) {
    typedef Display* (*fn_t)(_Xconst char*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XOpenDisplay");
    }
    return fn ? fn(display_name) : nullptr;
}

// 9. XPutImage
int XPutImage(Display *display, Drawable d, GC gc, XImage *image,
              int src_x, int src_y, int dest_x, int dest_y,
              unsigned int width, unsigned int height) {
    typedef int (*fn_t)(Display*, Drawable, GC, XImage*, int, int, int, int, unsigned int, unsigned int);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XPutImage");
    }
    return fn ? fn(display, d, gc, image, src_x, src_y, dest_x, dest_y, width, height) : 0;
}

// 10. XQueryTree
Status XQueryTree(Display *display, Window w, Window *root_return, Window *parent_return,
                  Window **children_return, unsigned int *nchildren_return) {
    typedef Status (*fn_t)(Display*, Window, Window*, Window*, Window**, unsigned int*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XQueryTree");
    }
    if (fn) return fn(display, w, root_return, parent_return, children_return, nchildren_return);
    if (root_return) *root_return = 0;
    if (parent_return) *parent_return = 0;
    if (children_return) *children_return = nullptr;
    if (nchildren_return) *nchildren_return = 0;
    return 0;
}

// 11. XResourceManagerString
char *XResourceManagerString(Display *display) {
    typedef char* (*fn_t)(Display*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XResourceManagerString");
    }
    return fn ? fn(display) : nullptr;
}

// 12. XrmGetResource
Bool XrmGetResource(XrmDatabase database, _Xconst char *str_name, _Xconst char *str_class,
                    char **str_type_return, XrmValue *value_return) {
    typedef Bool (*fn_t)(XrmDatabase, _Xconst char*, _Xconst char*, char**, XrmValue*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XrmGetResource");
    }
    return fn ? fn(database, str_name, str_class, str_type_return, value_return) : False;
}

// 13. XrmGetStringDatabase
XrmDatabase XrmGetStringDatabase(_Xconst char *data) {
    typedef XrmDatabase (*fn_t)(_Xconst char*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libx11();
        if (h) fn = (fn_t)dlsym(h, "XrmGetStringDatabase");
    }
    return fn ? fn(data) : nullptr;
}

// GLX 1. glXChooseFBConfig
GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen, const int *attribList, int *nitems) {
    typedef GLXFBConfig* (*fn_t)(Display*, int, const int*, int*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libgl();
        if (h) fn = (fn_t)dlsym(h, "glXChooseFBConfig");
    }
    if (fn) return fn(dpy, screen, attribList, nitems);
    if (nitems) *nitems = 0;
    return nullptr;
}

// GLX 2. glXChooseVisual
XVisualInfo *glXChooseVisual(Display *dpy, int screen, int *attribList) {
    typedef XVisualInfo* (*fn_t)(Display*, int, int*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libgl();
        if (h) fn = (fn_t)dlsym(h, "glXChooseVisual");
    }
    return fn ? fn(dpy, screen, attribList) : nullptr;
}

// GLX 3. glXCreateContext
GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct) {
    typedef GLXContext (*fn_t)(Display*, XVisualInfo*, GLXContext, Bool);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libgl();
        if (h) fn = (fn_t)dlsym(h, "glXCreateContext");
    }
    return fn ? fn(dpy, vis, shareList, direct) : nullptr;
}

// GLX 4. glXCreatePbuffer
GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config, const int *attribList) {
    typedef GLXPbuffer (*fn_t)(Display*, GLXFBConfig, const int*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libgl();
        if (h) fn = (fn_t)dlsym(h, "glXCreatePbuffer");
    }
    return fn ? fn(dpy, config, attribList) : 0;
}

// GLX 5. glXDestroyContext
void glXDestroyContext(Display *dpy, GLXContext ctx) {
    typedef void (*fn_t)(Display*, GLXContext);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libgl();
        if (h) fn = (fn_t)dlsym(h, "glXDestroyContext");
    }
    if (fn) fn(dpy, ctx);
}

// GLX 6. glXDestroyPbuffer
void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf) {
    typedef void (*fn_t)(Display*, GLXPbuffer);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libgl();
        if (h) fn = (fn_t)dlsym(h, "glXDestroyPbuffer");
    }
    if (fn) fn(dpy, pbuf);
}

// GLX 7. glXGetProcAddress
__GLXextFuncPtr glXGetProcAddress(const GLubyte *procName) {
    typedef __GLXextFuncPtr (*fn_t)(const GLubyte*);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libgl();
        if (h) fn = (fn_t)dlsym(h, "glXGetProcAddress");
    }
    return fn ? fn(procName) : nullptr;
}

// GLX 8. glXGetVisualFromFBConfig
XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config) {
    typedef XVisualInfo* (*fn_t)(Display*, GLXFBConfig);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libgl();
        if (h) fn = (fn_t)dlsym(h, "glXGetVisualFromFBConfig");
    }
    return fn ? fn(dpy, config) : nullptr;
}

// GLX 9. glXMakeCurrent
Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx) {
    typedef Bool (*fn_t)(Display*, GLXDrawable, GLXContext);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libgl();
        if (h) fn = (fn_t)dlsym(h, "glXMakeCurrent");
    }
    return fn ? fn(dpy, drawable, ctx) : False;
}

// GLX 10. glXSwapBuffers
void glXSwapBuffers(Display *dpy, GLXDrawable drawable) {
    typedef void (*fn_t)(Display*, GLXDrawable);
    static fn_t fn = nullptr;
    if (!fn) {
        void* h = get_libgl();
        if (h) fn = (fn_t)dlsym(h, "glXSwapBuffers");
    }
    if (fn) fn(dpy, drawable);
}

} // extern "C"
