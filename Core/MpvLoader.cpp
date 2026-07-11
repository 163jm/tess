#include "MpvLoader.h"

namespace player::core {

MpvLoader& MpvLoader::Instance() {
    static MpvLoader instance;
    return instance;
}

MpvLoader::~MpvLoader() {
    if (module_) {
        FreeLibrary(module_);
        module_ = nullptr;
    }
}

template <typename T>
T MpvLoader::LoadSym(const char* name) {
    auto proc = GetProcAddress(module_, name);
    if (!proc) {
        throw std::runtime_error(std::string("libmpv 缺少导出符号: ") + name);
    }
    return reinterpret_cast<T>(proc);
}

void MpvLoader::Load(const std::wstring& dllPath) {
    if (module_) {
        return; // 已加载
    }

    const std::wstring path = dllPath.empty() ? L"libmpv-2.dll" : dllPath;
    module_ = LoadLibraryW(path.c_str());
    if (!module_) {
        DWORD err = GetLastError();
        throw std::runtime_error(
            "无法加载 libmpv-2.dll (LoadLibraryW 错误码 " + std::to_string(err) +
            ")。请确认 libmpv-2.dll 与可执行文件在同一目录，或路径正确。");
    }

    api_.create = LoadSym<decltype(&mpv_create)>("mpv_create");
    api_.initialize = LoadSym<decltype(&mpv_initialize)>("mpv_initialize");
    api_.destroy = LoadSym<decltype(&mpv_destroy)>("mpv_destroy");
    api_.terminate_destroy = LoadSym<decltype(&mpv_terminate_destroy)>("mpv_terminate_destroy");
    api_.set_option_string = LoadSym<decltype(&mpv_set_option_string)>("mpv_set_option_string");
    api_.set_property_string = LoadSym<decltype(&mpv_set_property_string)>("mpv_set_property_string");
    api_.get_property = LoadSym<decltype(&mpv_get_property)>("mpv_get_property");
    api_.set_property = LoadSym<decltype(&mpv_set_property)>("mpv_set_property");
    api_.command = LoadSym<decltype(&mpv_command)>("mpv_command");
    api_.command_async = LoadSym<decltype(&mpv_command_async)>("mpv_command_async");
    api_.error_string = LoadSym<decltype(&mpv_error_string)>("mpv_error_string");
    api_.free = LoadSym<decltype(&mpv_free)>("mpv_free");
    api_.free_node_contents = LoadSym<decltype(&mpv_free_node_contents)>("mpv_free_node_contents");
    api_.wait_event = LoadSym<decltype(&mpv_wait_event)>("mpv_wait_event");
    api_.set_wakeup_callback = LoadSym<decltype(&mpv_set_wakeup_callback)>("mpv_set_wakeup_callback");
    api_.request_log_messages = LoadSym<decltype(&mpv_request_log_messages)>("mpv_request_log_messages");
    api_.observe_property = LoadSym<decltype(&mpv_observe_property)>("mpv_observe_property");

    api_.render_context_create = LoadSym<decltype(&mpv_render_context_create)>("mpv_render_context_create");
    api_.render_context_free = LoadSym<decltype(&mpv_render_context_free)>("mpv_render_context_free");
    api_.render_context_render = LoadSym<decltype(&mpv_render_context_render)>("mpv_render_context_render");
    api_.render_context_report_swap = LoadSym<decltype(&mpv_render_context_report_swap)>("mpv_render_context_report_swap");
    api_.render_context_set_update_callback = LoadSym<decltype(&mpv_render_context_set_update_callback)>("mpv_render_context_set_update_callback");
    api_.render_context_update = LoadSym<decltype(&mpv_render_context_update)>("mpv_render_context_update");
}

} // namespace player::core
