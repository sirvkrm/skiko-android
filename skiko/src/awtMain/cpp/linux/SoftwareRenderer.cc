#include <jawt_md.h>
#include <X11/Xresource.h>
#include <stdint.h>
#include "jni_helpers.h"

#include "SkColorSpace.h"
#include "SkSurface.h"
#include "src/core/SkAutoMalloc.h"

class SoftwareDevice
{
public:
    Display* display;
    Window window;
    GC gc;
    sk_sp<SkSurface> surface;
    unsigned int depth = 0;
    SkColorType colorSpace = kUnknown_SkColorType;

    void initDevice() {
        Window wnd;
        int x, y;
        unsigned int w, h, border;
        XGetGeometry(display, window, &wnd, &x, &y, &w, &h, &border, &depth);

        switch(depth) {
            case 16:
                colorSpace = kRGB_565_SkColorType;
                break;
            case 24: case 32:
                colorSpace = kBGRA_8888_SkColorType;
                break;
            default:
                colorSpace = kUnknown_SkColorType;
        }
    }

    ~SoftwareDevice() {
        if (display != NULL && gc != NULL)
        {
            XFreeGC(display, gc);
        }
    }
};

extern "C"
{
    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_renderer_LinuxSoftwareRenderer_createDevice(
        JNIEnv *env, jobject renderer, jlong displayPtr, jlong windowPtr, jint width, jint height)
    {
        Display *display = fromJavaPointer<Display *>(displayPtr);
        Window window = fromJavaPointer<Window>(windowPtr);
        if (!display || !window) {
            return 0L;
        }

        SoftwareDevice *device = new SoftwareDevice();
        device->display = display;
        device->window = window;
        device->gc = XCreateGC(device->display, device->window, 0, nullptr);
        if (!device->gc) {
            delete device;
            return 0L;
        }

        device->initDevice();
        if (device->colorSpace == kUnknown_SkColorType)
        {
            delete device;
            return 0L;
        }

        device->surface.reset();
        SkImageInfo info = SkImageInfo::Make(
            width, height, device->colorSpace, kPremul_SkAlphaType,
            SkColorSpace::MakeSRGB());
        device->surface = SkSurfaces::Raster(info);

        return toJavaPointer(device);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_renderer_AbstractDirectSoftwareRenderer_resize(
        JNIEnv *env, jobject renderer, jlong devicePtr, jint width, jint height)
    {
        SoftwareDevice *device = fromJavaPointer<SoftwareDevice *>(devicePtr);
        if (!device) return;
        device->surface.reset();
        SkImageInfo info = SkImageInfo::Make(
            width, height, device->colorSpace, kPremul_SkAlphaType,
            SkColorSpace::MakeSRGB());
        device->surface = SkSurfaces::Raster(info);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_renderer_AbstractDirectSoftwareRenderer_acquireSurface(
        JNIEnv *env, jobject renderer, jlong devicePtr)
    {
        SoftwareDevice *device = fromJavaPointer<SoftwareDevice *>(devicePtr);
        if (!device || !device->surface) return 0L;
        return toJavaPointer(device->surface.release());
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_renderer_AbstractDirectSoftwareRenderer_finishFrame(
        JNIEnv *env, jobject renderer, jlong devicePtr, jlong surfacePtr)
    {
        SoftwareDevice *device = fromJavaPointer<SoftwareDevice *>(devicePtr);
        SkSurface *surface = fromJavaPointer<SkSurface *>(surfacePtr);
        if (!device || !surface) return;

        SkPixmap pm;
        if (!surface->peekPixels(&pm)) {
            return;
        }
        int bitsPerPixel = pm.info().bytesPerPixel() * 8;
        XImage image;
        memset(&image, 0, sizeof(image));
        image.width = pm.width();
        image.height = pm.height();
        image.format = ZPixmap;
        image.data = (char*) pm.addr();
        image.byte_order = LSBFirst;
        image.bitmap_unit = bitsPerPixel;
        image.bitmap_bit_order = LSBFirst;
        image.bitmap_pad = bitsPerPixel;
        image.depth = device->depth ? device->depth : 24;
        image.bytes_per_line = pm.rowBytes();
        image.bits_per_pixel = bitsPerPixel;
        if (!XInitImage(&image)) {
            return;
        }
        if (device->display && device->window && device->gc) {
            XPutImage(device->display, device->window, device->gc, &image, 0, 0, 0, 0, pm.width(), pm.height());
        }
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_renderer_AbstractDirectSoftwareRenderer_disposeDevice(
        JNIEnv *env, jobject renderer, jlong devicePtr)
    {
        SoftwareDevice *device = fromJavaPointer<SoftwareDevice *>(devicePtr);
        if (device) {
            delete device;
        }
    }

    // Backwards compatibility aliases for Android Studio / Compose Desktop (Redrawer)
    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_redrawer_LinuxSoftwareRedrawer_createDevice(
        JNIEnv *env, jobject redrawer, jlong displayPtr, jlong windowPtr, jint width, jint height)
    {
        return Java_org_jetbrains_skiko_renderer_LinuxSoftwareRenderer_createDevice(env, redrawer, displayPtr, windowPtr, width, height);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_AbstractDirectSoftwareRedrawer_resize(
        JNIEnv *env, jobject redrawer, jlong devicePtr, jint width, jint height)
    {
        Java_org_jetbrains_skiko_renderer_AbstractDirectSoftwareRenderer_resize(env, redrawer, devicePtr, width, height);
    }

    JNIEXPORT jlong JNICALL Java_org_jetbrains_skiko_redrawer_AbstractDirectSoftwareRedrawer_acquireSurface(
        JNIEnv *env, jobject redrawer, jlong devicePtr)
    {
        return Java_org_jetbrains_skiko_renderer_AbstractDirectSoftwareRenderer_acquireSurface(env, redrawer, devicePtr);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_AbstractDirectSoftwareRedrawer_finishFrame(
        JNIEnv *env, jobject redrawer, jlong devicePtr, jlong surfacePtr)
    {
        Java_org_jetbrains_skiko_renderer_AbstractDirectSoftwareRenderer_finishFrame(env, redrawer, devicePtr, surfacePtr);
    }

    JNIEXPORT void JNICALL Java_org_jetbrains_skiko_redrawer_AbstractDirectSoftwareRedrawer_disposeDevice(
        JNIEnv *env, jobject redrawer, jlong devicePtr)
    {
        Java_org_jetbrains_skiko_renderer_AbstractDirectSoftwareRenderer_disposeDevice(env, redrawer, devicePtr);
    }
}
