AR = ['/root/clang+llvm-11.1.0-x86_64-linux-gnu-ubuntu-16.04/bin/llvm-ar']
ARFLAGS = ['rcs']
BINDIR = '/root/souce-engine-cssoso-1.0/out/lib/arm64-v8a'
BIT32_MANDATORY = False
CC = ['clang', '--target=aarch64-linux-android21']
CCLNK_SRC_F = []
CCLNK_TGT_F = ['-o']
CC_NAME = 'clang'
CC_SRC_F = []
CC_TGT_F = ['-c', '-o']
CC_VERSION = ('11', '1', '0')
CFLAGS = ['--sysroot=/root/android-ndk-r10e/platforms/android-21/arch-arm64', '-I/root/android-ndk-r10e/sources/android/support/include', '-DANDROID', '-D__ANDROID__', '-MMD', '-fno-strict-aliasing', '-fvisibility=hidden', '-O2', '-funsafe-math-optimizations', '-ftree-vectorize', '-ffast-math', '-pipe', '-fPIC', '-L/root/souce-engine-cssoso-1.0/lib/android/aarch64/', '-pthread', '-I/root/souce-engine-cssoso-1.0/thirdparty/curl/include', '-I/root/souce-engine-cssoso-1.0/thirdparty/SDL', '-I/root/souce-engine-cssoso-1.0/thirdparty/openal-soft/include/', '-I/root/souce-engine-cssoso-1.0/thirdparty/fontconfig', '-I/root/souce-engine-cssoso-1.0/thirdparty/freetype/include', '-llog', '-lz', '-funwind-tables', '-g', '-fsigned-char', '-w']
CFLAGS_MACBUNDLE = ['-fPIC']
CFLAGS_cshlib = ['-fPIC']
COMPILER_CC = 'clang'
COMPILER_CXX = 'clang++'
CPPPATH_ST = '-I%s'
CXX = ['clang++', '--target=aarch64-linux-android21']
CXXFLAGS = ['--sysroot=/root/android-ndk-r10e/platforms/android-21/arch-arm64', '-I/root/android-ndk-r10e/sources/android/support/include', '-DANDROID', '-D__ANDROID__', '-fno-sized-deallocation', '-MMD', '-fno-strict-aliasing', '-fvisibility=hidden', '-O2', '-funsafe-math-optimizations', '-ftree-vectorize', '-ffast-math', '-pipe', '-fPIC', '-L/root/souce-engine-cssoso-1.0/lib/android/aarch64/', '-pthread', '-I/root/souce-engine-cssoso-1.0/thirdparty/curl/include', '-I/root/souce-engine-cssoso-1.0/thirdparty/SDL', '-I/root/souce-engine-cssoso-1.0/thirdparty/openal-soft/include/', '-I/root/souce-engine-cssoso-1.0/thirdparty/fontconfig', '-I/root/souce-engine-cssoso-1.0/thirdparty/freetype/include', '-llog', '-lz', '-funwind-tables', '-g', '-fsigned-char', '-std=c++11', '-fpermissive', '-w']
CXXFLAGS_MACBUNDLE = ['-fPIC']
CXXFLAGS_cxxshlib = ['-fPIC']
CXXLNK_SRC_F = []
CXXLNK_TGT_F = ['-o']
CXX_NAME = 'clang'
CXX_SRC_F = []
CXX_TGT_F = ['-c', '-o']
DEDICATED = False
DEFINES = ['DX_TO_GL_ABSTRACTION', 'GL_GLEXT_PROTOTYPES', 'BINK_VIDEO', 'TOGLES', 'USE_SDL=1', 'PLATFORM_64BITS=1', 'ANDROID=1', '_ANDROID=1', 'LINUX=1', '_LINUX=1', 'POSIX=1', '_POSIX=1', 'GNUC', 'NO_HOOK_MALLOC', '_DLL_EXT=.so', 'NO_MEMOVERRIDE_NEW_DELETE=1', 'NDEBUG', 'GIT_COMMIT_HASH="[]"', 'HAVE_JPEG=1', 'HAVE_PNG=1', 'HAVE_CURL=1', 'HAVE_ZLIB=1', 'FAST_MATERIALVAR_ACCESS=1']
DEFINES_ST = '-D%s'
DEFINE_COMMENTS = {'USE_SDL': '', 'PLATFORM_64BITS': '', 'NO_MEMOVERRIDE_NEW_DELETE': '', 'GIT_COMMIT_HASH': '', 'HAVE_JPEG': '', 'HAVE_PNG': '', 'HAVE_CURL': '', 'HAVE_ZLIB': '', 'DATAMODEL_LIB': '', 'DMXLOADER_LIB': '', 'FILESYSTEM_STDIO_EXPORTS': '', 'DONT_PROTECT_FILEIO_FUNCTIONS': '', 'SUPPORT_PACKED_STORE': '', 'VERSION_SAFE_STEAM_API_INTERFACES': '', 'LAUNCHERONLY': '', 'CFLAGS': '', 'LDFLAGS': '', 'FAST_MATERIALVAR_ACCESS': '', 'MATHLIB_LIB': '', '_WINDOWS': '', 'SERVERBROWSER_EXPORTS': '', 'GAME_SRC': '', 'SOUNDEMITTERSYSTEM_EXPORTS': '', 'WAF_CFLAGS': '', 'WAF_LDFLAGS': '', 'TIER0_DLL_EXPORT': '', 'VGUIMATSURFACE_DLL_EXPORT': '', 'GAMEUI_EXPORTS': '', 'VSTDLIB_DLL_EXPORT': '', 'VTEX_DLL': '', 'UTILS': '', 'VTEX_DLL_EXPORTS': '', 'PROTECTED_THINGS_DISABLE': '', 'UNICODE_EXPORTS': '', 'TOGL_DLL_EXPORT': '', 'OPUS_EXPORTS': ''}
DEST_BINFMT = 'elf'
DEST_CPU = 'aarch64'
DEST_OS = 'android'
DEST_OS2 = 'android'
ENABLE_GCCDEPS = ['c', 'cxx']
ENVNAME = 'materialsystem/shaderlib'
GIT = ['/usr/bin/git']
GL = True
HAVE_CURL = 1
HAVE_JPEG = 1
HAVE_M = True
HAVE_PNG = 1
HAVE_ZLIB = 1
INCLUDES = ['/root/android-ndk-r10e/sources/cxx-stl/gnu-libstdc++/4.9/include', '/root/android-ndk-r10e/sources/cxx-stl/gnu-libstdc++/4.9/libs/arm64-v8a/include', '/root/souce-engine-cssoso-1.0/common']
LDFLAGS = ['-no-canonical-prefixes', '-lgcc', '-stdlib=libstdc++', '-lgnustl_static']
LIBDIR = '/root/souce-engine-cssoso-1.0/out/lib/arm64-v8a'
LIBPATH_ST = '-L%s'
LIB_ANDROID_SUPPORT = ['android_support']
LIB_BZ2 = ['bz2']
LIB_CURL = ['curl']
LIB_DL = ['dl']
LIB_FT2 = ['freetype2']
LIB_JPEG = ['jpeg']
LIB_M = ['m']
LIB_OPUS = ['opus']
LIB_PNG = ['png']
LIB_SDL2 = ['SDL2']
LIB_ST = '-l%s'
LIB_ZLIB = ['z']
LINKFLAGS = ['--gcc-toolchain=/root/android-ndk-r10e/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64', '--sysroot=/root/android-ndk-r10e/platforms/android-21/arch-arm64', '-fuse-ld=lld', '-Wl,--hash-style=both', '-Wl,--no-undefined', '-fvisibility=hidden', '-pipe', '-fPIC', '-L/root/souce-engine-cssoso-1.0/lib/android/aarch64/', '-pthread', '-I/root/souce-engine-cssoso-1.0/thirdparty/curl/include', '-I/root/souce-engine-cssoso-1.0/thirdparty/SDL', '-I/root/souce-engine-cssoso-1.0/thirdparty/openal-soft/include/', '-I/root/souce-engine-cssoso-1.0/thirdparty/fontconfig', '-I/root/souce-engine-cssoso-1.0/thirdparty/freetype/include', '-llog', '-lz', '-funwind-tables', '-g', '-fsigned-char']
LINKFLAGS_MACBUNDLE = ['-bundle', '-undefined', 'dynamic_lookup']
LINKFLAGS_cshlib = ['-shared']
LINKFLAGS_cstlib = ['-Wl,-Bstatic']
LINKFLAGS_cxxshlib = ['-shared']
LINKFLAGS_cxxstlib = ['-Wl,-Bstatic']
LINK_CC = ['clang', '--target=aarch64-linux-android21']
LINK_CXX = ['clang++', '--target=aarch64-linux-android21']
MSVC_SUBSYSTEM = 'WINDOWS,5.01'
MSVC_TARGETS = ['x64']
OBJCOPY = ['/root/clang+llvm-11.1.0-x86_64-linux-gnu-ubuntu-16.04/bin/llvm-objcopy']
OPUS = False
PREFIX = '/root/souce-engine-cssoso-1.0/out/lib/arm64-v8a'
RPATH_ST = '-Wl,-rpath,%s'
SDL = 1
SHLIB_MARKER = '-Wl,-Bdynamic'
SONAME_ST = '-Wl,-h,%s'
STLIBPATH = ['/root/android-ndk-r10e/sources/cxx-stl/gnu-libstdc++/4.9/libs/arm64-v8a']
STLIBPATH_ST = '-L%s'
STLIB_MARKER = '-Wl,-Bstatic'
STLIB_ST = '-l%s'
STRIP = ['llvm-strip']
STRIPFLAGS = []
SUBPROJECT_PATH = ['materialsystem/shaderlib']
TESTS = False
TOGLES = True
cprogram_PATTERN = '%s'
cshlib_PATTERN = 'lib%s.so'
cstlib_PATTERN = 'lib%s.a'
cxxprogram_PATTERN = '%s'
cxxshlib_PATTERN = 'lib%s.so'
cxxstlib_PATTERN = 'lib%s.a'
define_key = ['USE_SDL', 'PLATFORM_64BITS', 'NO_MEMOVERRIDE_NEW_DELETE', 'GIT_COMMIT_HASH', 'HAVE_JPEG', 'HAVE_PNG', 'HAVE_CURL', 'HAVE_ZLIB', 'FAST_MATERIALVAR_ACCESS']
macbundle_PATTERN = '%s.bundle'
