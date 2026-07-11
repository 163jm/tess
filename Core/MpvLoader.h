#pragma once
// -----------------------------------------------------------------------
// MpvLoader: 在运行时用 LoadLibraryW + GetProcAddress 动态加载 libmpv-2.dll。
//
// 为什么不直接链接 .lib？
//   libmpv 官方开发包（shinchiro/mpv-winbuild-cmake）是用 MinGW 工具链产出的，
//   其导入库 libmpv.dll.a 是 GNU 格式，MSVC 的 link.exe 无法直接使用。
//   为了避免引入额外的转换步骤（如 dlltool / lib.exe /DEF），这里改用运行时
//   动态加载，只依赖 client.h 中的类型声明和 DLL 导出符号名，规避 ABI 问题。
// -----------------------------------------------------------------------

#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>
#include <string>
#include <stdexcept>
#include <windows.h>

namespace player::core {

// 所有用到的 mpv C API 函数指针类型
struct MpvApi {
    decltype(&mpv_create) create = nullptr;
    decltype(&mpv_initialize) initialize = nullptr;
    decltype(&mpv_destroy) destroy = nullptr;
    decltype(&mpv_terminate_destroy) terminate_destroy = nullptr;
    decltype(&mpv_set_option_string) set_option_string = nullptr;
    decltype(&mpv_set_property_string) set_property_string = nullptr;
    decltype(&mpv_get_property) get_property = nullptr;
    decltype(&mpv_set_property) set_property = nullptr;
    decltype(&mpv_command) command = nullptr;
    decltype(&mpv_command_async) command_async = nullptr;
    decltype(&mpv_error_string) error_string = nullptr;
    decltype(&mpv_free) free = nullptr;
    decltype(&mpv_free_node_contents) free_node_contents = nullptr;
    decltype(&mpv_wait_event) wait_event = nullptr;
    decltype(&mpv_set_wakeup_callback) set_wakeup_callback = nullptr;
    decltype(&mpv_request_log_messages) request_log_messages = nullptr;
    decltype(&mpv_observe_property) observe_property = nullptr;

    // render API (mpv/render.h, render_gl.h)
    decltype(&mpv_render_context_create) render_context_create = nullptr;
    decltype(&mpv_render_context_free) render_context_free = nullptr;
    decltype(&mpv_render_context_render) render_context_render = nullptr;
    decltype(&mpv_render_context_report_swap) render_context_report_swap = nullptr;
    decltype(&mpv_render_context_set_update_callback) render_context_set_update_callback = nullptr;
    decltype(&mpv_render_context_update) render_context_update = nullptr;
};

// 单例：全局只加载一次 DLL
class MpvLoader {
public:
    static MpvLoader& Instance();

    // 加载指定路径的 libmpv-2.dll 并解析所有函数指针。
    // dllPath 为空时按 "libmpv-2.dll" 在标准搜索路径中查找。
    // 失败抛出 std::runtime_error。
    void Load(const std::wstring& dllPath = L"");

    bool IsLoaded() const { return module_ != nullptr; }
    const MpvApi& Api() const { return api_; }

private:
    MpvLoader() = default;
    ~MpvLoader();
    MpvLoader(const MpvLoader&) = delete;
    MpvLoader& operator=(const MpvLoader&) = delete;

    template <typename T>
    T LoadSym(const char* name);

    HMODULE module_ = nullptr;
    MpvApi api_{};
};

} // namespace player::core
