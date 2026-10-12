#include "FontMgrDefaultFactory.hh"

#if defined(SK_BUILD_FOR_MAC) || defined(SK_BUILD_FOR_IOS)
#include "ports/SkFontMgr_mac_ct.h"
#endif

#ifdef SK_BUILD_FOR_WIN
#include "ports/SkTypeface_win.h"
#endif

#if (defined(SK_BUILD_FOR_UNIX) || defined(SK_BUILD_FOR_LINUX)) && !defined(SKIKO_WASM)
#include "ports/SkFontMgr_fontconfig.h"
#include "ports/SkFontScanner_FreeType.h"
#endif

#if defined(SK_BUILD_FOR_ANDROID) || defined(__ANDROID__)
#include "ports/SkFontMgr_android.h"
#include "ports/SkFontScanner_FreeType.h"
#include "ports/SkFontMgr_fontconfig.h"

// Compatibility fallback for Android Bionic: forward any FontConfig requests to Android's native font manager
sk_sp<SkFontMgr> SkFontMgr_New_FontConfig(FcConfig* fc, std::unique_ptr<SkFontScanner> scanner) {
    if (!scanner) {
        scanner = SkFontScanner_Make_FreeType();
    }
    return SkFontMgr_New_Android(nullptr, std::move(scanner));
}
#endif

#ifdef SKIKO_WASM
#include <vector>
#include "SkData.h"
#include "ports/SkFontMgr_data.h"
struct SkEmbeddedResource { const uint8_t* data; size_t size; };
struct SkEmbeddedResourceHeader { const SkEmbeddedResource* entries; int count; };
extern "C" const SkEmbeddedResourceHeader SK_EMBEDDED_FONTS;
#endif

sk_sp<SkFontMgr> SkFontMgrSkikoDefault() {
#if defined(SKIKO_WASM)
    if (SK_EMBEDDED_FONTS.count == 0) {
        return nullptr;
    }

    std::vector<sk_sp<SkData>> fontDataVector;
    fontDataVector.reserve(SK_EMBEDDED_FONTS.count);

    for (size_t i = 0; i < SK_EMBEDDED_FONTS.count; ++i) {
        fontDataVector.push_back(
            SkData::MakeWithoutCopy(SK_EMBEDDED_FONTS.entries[i].data,
                                    SK_EMBEDDED_FONTS.entries[i].size)
        );
    }

    return SkFontMgr_New_Custom_Data(SkSpan<sk_sp<SkData>>(fontDataVector));
#elif defined(SK_BUILD_FOR_MAC) || defined(SK_BUILD_FOR_IOS)
    return SkFontMgr_New_CoreText(nullptr);
#elif defined(SK_BUILD_FOR_WIN)
    return SkFontMgr_New_DirectWrite();
#elif defined(__ANDROID__) || defined(SK_BUILD_FOR_ANDROID)
    return SkFontMgr_New_Android(nullptr, SkFontScanner_Make_FreeType());
#elif defined(SK_BUILD_FOR_UNIX) || defined(SK_BUILD_FOR_LINUX)
    return SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#else
    return nullptr;
#endif
}
