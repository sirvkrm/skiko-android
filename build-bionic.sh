#!/usr/bin/env bash
# ==============================================================================
# Build script for Skiko (libskiko-linux-arm64.so) on Android Bionic ARM64
# Compatible with Termux on-device compilation and Linux x86_64 NDK cross-compilation
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKIKO_CORE="${SCRIPT_DIR}/skiko"
BUILD_DIR="${SCRIPT_DIR}/build/bionic"
DIST_DIR="${SCRIPT_DIR}/dist"

mkdir -p "${BUILD_DIR}"
mkdir -p "${DIST_DIR}"

# Toolchain detection
if [ -d "/data/data/com.termux" ] || [ -n "${TERMUX_VERSION:-}" ]; then
    IS_TERMUX=1
    PREFIX="${PREFIX:-/data/data/com.termux/files/usr}"
    CC="${CC:-clang}"
    CXX="${CXX:-clang++}"
    STRIP="${STRIP:-llvm-strip}"
    LDFLAGS="-Wl,-rpath,/system/lib64 -Wl,-rpath,${PREFIX}/lib -Wl,-rpath,\$ORIGIN"
else
    IS_TERMUX=0
    ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-/opt/android-ndk/android-ndk-r26d}"
    TOOLCHAIN="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64"
    CC="${TOOLCHAIN}/bin/aarch64-linux-android24-clang"
    CXX="${TOOLCHAIN}/bin/aarch64-linux-android24-clang++"
    STRIP="${TOOLCHAIN}/bin/llvm-strip"
    LDFLAGS="-Wl,-rpath,/system/lib64 -Wl,-rpath,/data/data/com.termux/files/usr/lib -Wl,-rpath,\$ORIGIN"
fi

SKIA_DIR="${SKIA_DIR:-$(cd "${SCRIPT_DIR}/../skia" 2>/dev/null && pwd || echo "")}"
if [ -z "${SKIA_DIR}" ] || [ ! -d "${SKIA_DIR}" ]; then
    echo "ERROR: Skia directory not found. Set SKIA_DIR=/path/to/skia or place skia alongside this repo."
    exit 1
fi

SKIA_OUT="${SKIA_DIR}/out/Release-android-arm64"
if [ ! -f "${SKIA_OUT}/libskia.a" ]; then
    echo ">> Building Skia for Android ARM64..."
    cd "${SKIA_DIR}"
    if [ "${IS_TERMUX}" = "0" ]; then
        python3 tools/skia_release/build.py --target android --machine arm64 --ndk "${ANDROID_NDK_ROOT}"
    else
        python3 tools/skia_release/build.py --target android --machine arm64
    fi
    cd "${SCRIPT_DIR}"
fi

echo "=========================================================="
echo " Building Skiko (libskiko-linux-arm64.so)"
echo " CXX:      ${CXX}"
echo " Skia:     ${SKIA_DIR}"
echo " Skiko:    ${SKIKO_CORE}"
echo " Output:   ${DIST_DIR}/libskiko-linux-arm64.so"
echo "=========================================================="

# Setup isolated X11 / GL / KHR / fontconfig headers
X11_INC="${BUILD_DIR}/x11_inc"
mkdir -p "${X11_INC}"
if [ "${IS_TERMUX}" = "0" ] && [ ! -L "${X11_INC}/X11" ]; then
    ln -sfn /usr/include/X11 "${X11_INC}/X11"
    ln -sfn /usr/include/GL "${X11_INC}/GL"
    ln -sfn /usr/include/KHR "${X11_INC}/KHR"
    ln -sfn /usr/include/fontconfig "${X11_INC}/fontconfig"
    mkdir -p "${X11_INC}/dbus"
    if [ -d "/usr/include/dbus-1.0/dbus" ]; then
        ln -sfn /usr/include/dbus-1.0/dbus/* "${X11_INC}/dbus/" || true
    fi
    if [ -f "/usr/lib/x86_64-linux-gnu/dbus-1.0/include/dbus/dbus-arch-deps.h" ]; then
        ln -sfn /usr/lib/x86_64-linux-gnu/dbus-1.0/include/dbus/dbus-arch-deps.h "${X11_INC}/dbus/" || true
    fi
fi

# Locate JDK headers
JDK_INC_ARGS=()
if [ -n "${JAVA_HOME:-}" ] && [ -d "${JAVA_HOME}/include" ]; then
    JDK_INC_ARGS+=("-I${JAVA_HOME}/include" "-I${JAVA_HOME}/include/linux")
elif [ -d "/usr/lib/jvm/java-21-openjdk-amd64/include" ]; then
    JDK_INC_ARGS+=("-I/usr/lib/jvm/java-21-openjdk-amd64/include" "-I/usr/lib/jvm/java-21-openjdk-amd64/include/linux")
elif [ -d "/usr/lib/jvm/default-java/include" ]; then
    JDK_INC_ARGS+=("-I/usr/lib/jvm/default-java/include" "-I/usr/lib/jvm/default-java/include/linux")
fi

OBJ_DIR="${BUILD_DIR}/objs"
mkdir -p "${OBJ_DIR}"

python3 - <<PYEOF
import os, subprocess, concurrent.futures, sys

cxx = "${CXX}"
skia_dir = "${SKIA_DIR}"
skiko_core = "${SKIKO_CORE}"
obj_dir = "${OBJ_DIR}"
x11_inc = "${X11_INC}"
is_termux = "${IS_TERMUX}"

cflags = [
    "-std=c++20", "-O3", "-fPIC", "-fvisibility=default",
    "-fno-rtti", "-fno-exceptions",
    "-DGL_GLEXT_PROTOTYPES",
    "-DSK_ALLOW_STATIC_GLOBAL_INITIALIZERS=1",
    "-DSK_FORCE_DISTANCE_FIELD_TEXT=0",
    "-DSK_GAMMA_APPLY_TO_A8",
    "-DSK_GAMMA_SRGB",
    "-DSK_SCALAR_TO_FLOAT_EXCLUDED",
    "-DSK_SUPPORT_GPU=1",
    "-DSK_GANESH",
    "-DSK_GL",
    "-DSK_SHAPER_HARFBUZZ_AVAILABLE",
    "-DSK_UNICODE_AVAILABLE",
    "-DSK_SHAPER_UNICODE_AVAILABLE",
    "-DSK_SUPPORT_OPENCL=0",
    "-DSK_USING_THIRD_PARTY_ICU",
    "-DU_DISABLE_RENAMING=0",
    "-DU_DISABLE_VERSION_SUFFIX=1",
    "-DU_HAVE_LIB_SUFFIX=1",
    "-DU_LIB_SUFFIX_C_NAME=_skiko",
    "-DSK_BUILD_FOR_LINUX",
    "-DSK_VULKAN",
    f"-I{skia_dir}",
    f"-I{skia_dir}/include",
    f"-I{skia_dir}/include/third_party/vulkan",
    f"-I{skia_dir}/include/core",
    f"-I{skia_dir}/include/gpu",
    f"-I{skia_dir}/include/gpu/ganesh",
    f"-I{skia_dir}/include/effects",
    f"-I{skia_dir}/include/pathops",
    f"-I{skia_dir}/include/utils",
    f"-I{skia_dir}/include/codec",
    f"-I{skia_dir}/include/svg",
    f"-I{skia_dir}/modules/jsonreader",
    f"-I{skia_dir}/modules/skottie/include",
    f"-I{skia_dir}/modules/skparagraph/include",
    f"-I{skia_dir}/modules/skshaper/include",
    f"-I{skia_dir}/modules/skunicode/include",
    f"-I{skia_dir}/modules/sksg/include",
    f"-I{skia_dir}/modules/svg/include",
    f"-I{skia_dir}/third_party/externals/harfbuzz/src",
    f"-I{skia_dir}/third_party/icu",
    f"-I{skia_dir}/third_party/externals/icu/source/common",
    f"-I{skiko_core}/src/awtMain/cpp/include",
    f"-I{skiko_core}/src/jvmMain/cpp/common",
    f"-I{skiko_core}/src/jvmMain/cpp/include",
    f"-I{skiko_core}/src/commonMain/cpp/common/include",
]

jdk_flags = "${JDK_INC_ARGS[@]}".split()
cflags.extend(jdk_flags)

if is_termux == "0":
    cflags.append(f"-I{x11_inc}")
else:
    termux_prefix = os.environ.get("PREFIX", "/data/data/com.termux/files/usr")
    cflags.append(f"-I{termux_prefix}/include")

src_dirs = [
    f"{skiko_core}/src/commonMain/cpp/common",
    f"{skiko_core}/src/jvmMain/cpp/common",
    f"{skiko_core}/src/awtMain/cpp/linux",
]

sources = []
for d in src_dirs:
    for root, _, files in os.walk(d):
        for f in files:
            if f.endswith(".cc") or f.endswith(".cpp"):
                sources.append(os.path.join(root, f))

print(f">> Compiling {len(sources)} Skiko C++ files with {os.cpu_count()} threads...")

def compile_item(item):
    idx, src = item
    obj = os.path.join(obj_dir, f"obj_{idx}.o")
    if os.path.exists(obj) and os.path.getmtime(obj) > os.path.getmtime(src):
        return (True, src, obj)
    cmd = cxx.split() + cflags + ["-c", src, "-o", obj]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        return (False, src, res.stderr)
    return (True, src, obj)

with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
    results = list(executor.map(compile_item, enumerate(sources, 1)))

failed = [r for r in results if not r[0]]
if failed:
    print(f"ERROR: {len(failed)} files failed to compile:")
    for _, src, err in failed:
        print(f"--- {src} ---\n{err}")
    sys.exit(1)

print(">> All Skiko objects compiled successfully.")
PYEOF

TARGET_LIB="${DIST_DIR}/libskiko-linux-arm64.so"
echo ">> Linking ${TARGET_LIB}..."

SKIA_LIBS=(
    "${SKIA_OUT}/libskparagraph.a"
    "${SKIA_OUT}/libsvg.a"
    "${SKIA_OUT}/libskshaper.a"
    "${SKIA_OUT}/libskunicode_icu.a"
    "${SKIA_OUT}/libskunicode_core.a"
    "${SKIA_OUT}/libicu.a"
    "${SKIA_OUT}/libharfbuzz.a"
    "${SKIA_OUT}/libfreetype2.a"
    "${SKIA_OUT}/libexpat.a"
    "${SKIA_OUT}/libpng.a"
    "${SKIA_OUT}/libjpeg.a"
    "${SKIA_OUT}/libwebp.a"
    "${SKIA_OUT}/libzlib.a"
    "${SKIA_OUT}/libskia.a"
)

${CXX} -shared -Wl,-soname,libskiko-linux-arm64.so \
    ${LDFLAGS} \
    -Wl,-z,relro,-z,now \
    -Wl,--gc-sections \
    -Wl,--unresolved-symbols=ignore-all \
    -static-libstdc++ \
    -o "${TARGET_LIB}" \
    ${OBJ_DIR}/*.o \
    -Wl,--start-group \
    "${SKIA_LIBS[@]}" \
    -Wl,--end-group \
    -lGLESv3 -lEGL -llog -landroid -lm -ldl

echo ">> Stripping symbols..."
${STRIP} --strip-unneeded "${TARGET_LIB}"

echo "=========================================================="
echo " Successfully built: ${TARGET_LIB}"
file "${TARGET_LIB}"
ls -lh "${TARGET_LIB}"
echo "=========================================================="
