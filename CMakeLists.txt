cmake_minimum_required(VERSION 3.25)
project(NativePlayer LANGUAGES CXX)

# ---------------------------------------------------------------------------
# 阶段 1 目标：一个 WinUI3 窗口，内嵌 SwapChainPanel，用 libmpv 播放本地视频文件。
# 仅在 Windows + MSVC 下有效（本工程依赖 C++/WinRT 与 Windows App SDK）。
# ---------------------------------------------------------------------------

if(NOT WIN32)
    message(FATAL_ERROR "NativePlayer 仅支持 Windows 平台编译（需要 WinUI3 / C++/WinRT）。")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# /await 已不需要（C++20 原生协程），但 WinRT 仍需要以下选项
add_compile_options(/permissive- /Zc:__cplusplus /EHsc /utf-8)
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

# ---------------------------------------------------------------------------
# Windows App SDK (WinUI3) —— 通过 NuGet 引入。
# 推荐直接用 Visual Studio 打开本目录（File > Open > CMake），VS 的 CMake
# 集成会自动处理 NuGet 还原。若命令行编译，需先用 nuget.exe restore。
# ---------------------------------------------------------------------------
set(WINDOWSAPPSDK_VERSION "1.5.240311000" CACHE STRING "Windows App SDK NuGet 包版本")

# ---------------------------------------------------------------------------
# libmpv：使用现成开发包（来自 shinchiro/mpv-winbuild-cmake 的 Release）。
# 约定目录结构：
#   third_party/libmpv/include/mpv/client.h  render.h  render_gl.h ...
#   third_party/libmpv/lib/libmpv.dll.a   (mingw 导入库，可选)
#   third_party/libmpv/lib/libmpv-2.dll   (运行时 DLL，构建后自动复制到输出目录)
# 见 third_party/libmpv/README.md 获取下载与放置说明。
# ---------------------------------------------------------------------------
set(LIBMPV_ROOT ${CMAKE_SOURCE_DIR}/third_party/libmpv)
set(LIBMPV_INCLUDE_DIR ${LIBMPV_ROOT}/include)
set(LIBMPV_DLL ${LIBMPV_ROOT}/lib/libmpv-2.dll)

if(NOT EXISTS ${LIBMPV_INCLUDE_DIR}/mpv/client.h)
    message(WARNING "未找到 ${LIBMPV_INCLUDE_DIR}/mpv/client.h — 请先按照 third_party/libmpv/README.md 放置 libmpv 开发包。")
endif()

option(NATIVEPLAYER_CORE_ONLY "只配置 Core 静态库，跳过 App（App 依赖 WinUI3/XAML，纯 CMake 不驱动编译）" OFF)

add_subdirectory(Core)

if(NOT NATIVEPLAYER_CORE_ONLY)
    add_subdirectory(App)
endif()
