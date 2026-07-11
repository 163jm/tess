#include "MpvPlayer.h"
#include <stdexcept>
#include <cstdio>

namespace player::core {

namespace {
void CheckError(const MpvApi& api, int code, const char* what) {
    if (code < 0) {
        throw std::runtime_error(std::string(what) + " 失败: " + api.error_string(code));
    }
}
} // namespace

MpvPlayer::MpvPlayer() = default;

MpvPlayer::~MpvPlayer() {
    Shutdown();
}

void MpvPlayer::Initialize(const std::wstring& libmpvDllPath) {
    auto& loader = MpvLoader::Instance();
    loader.Load(libmpvDllPath);
    api_ = loader.Api();

    handle_ = api_.create();
    if (!handle_) {
        throw std::runtime_error("mpv_create 返回空指针");
    }

    // 一些基础选项：硬件解码、日志级别等。
    api_.set_option_string(handle_, "hwdec", "auto-safe");
    api_.set_option_string(handle_, "vo", "libmpv"); // 使用 render API 而非自管窗口
    api_.set_option_string(handle_, "keep-open", "yes");
    api_.request_log_messages(handle_, "warn");

    CheckError(api_, api_.initialize(handle_), "mpv_initialize");
}

void MpvPlayer::InitRenderContext(GlGetProcAddressFn getProcAddress, void* getProcAddressCtx) {
    if (!handle_) {
        throw std::runtime_error("InitRenderContext 之前必须先调用 Initialize");
    }

    mpv_opengl_init_params glInitParams{};
    glInitParams.get_proc_address = getProcAddress;
    glInitParams.get_proc_address_ctx = getProcAddressCtx;

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInitParams},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };

    int rc = api_.render_context_create(&renderCtx_, handle_, params);
    CheckError(api_, rc, "mpv_render_context_create");

    api_.render_context_set_update_callback(renderCtx_, &MpvPlayer::OnMpvRenderUpdate, this);
}

void MpvPlayer::OnMpvRenderUpdate(void* ctx) {
    auto* self = static_cast<MpvPlayer*>(ctx);
    if (self->renderReadyCb_) {
        self->renderReadyCb_();
    }
}

void MpvPlayer::SetRenderReadyCallback(RenderReadyCallback cb) {
    renderReadyCb_ = std::move(cb);
}

void MpvPlayer::RenderFrame(int fbo, int width, int height) {
    if (!renderCtx_) return;

    mpv_opengl_fbo mpFbo{};
    mpFbo.fbo = fbo;
    mpFbo.w = width;
    mpFbo.h = height;

    int flipY = 1; // OpenGL FBO 默认左下角为原点，需要翻转
    mpv_render_param renderParams[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpFbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flipY},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };

    api_.render_context_render(renderCtx_, renderParams);
}

void MpvPlayer::LoadFile(const std::wstring& path) {
    // mpv 的 C API 是 UTF-8，这里做一次简单转换。
    int size = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, utf8.data(), size, nullptr, nullptr);

    const char* cmd[] = {"loadfile", utf8.c_str(), nullptr};
    CheckError(api_, api_.command(handle_, cmd), "loadfile");
}

void MpvPlayer::Play() {
    int flag = 0;
    api_.set_property(handle_, "pause", MPV_FORMAT_FLAG, &flag);
}

void MpvPlayer::Pause() {
    int flag = 1;
    api_.set_property(handle_, "pause", MPV_FORMAT_FLAG, &flag);
}

void MpvPlayer::TogglePause() {
    if (IsPaused()) Play(); else Pause();
}

void MpvPlayer::Seek(double seconds, bool relative) {
    char secBuf[64];
    snprintf(secBuf, sizeof(secBuf), "%.3f", seconds);
    const char* cmd[] = {"seek", secBuf, relative ? "relative" : "absolute", nullptr};
    api_.command(handle_, cmd);
}

void MpvPlayer::SetVolume(int volume0to100) {
    double v = volume0to100;
    api_.set_property(handle_, "volume", MPV_FORMAT_DOUBLE, &v);
}

double MpvPlayer::GetPositionSeconds() const {
    double pos = 0;
    api_.get_property(handle_, "time-pos", MPV_FORMAT_DOUBLE, &pos);
    return pos;
}

double MpvPlayer::GetDurationSeconds() const {
    double dur = 0;
    api_.get_property(handle_, "duration", MPV_FORMAT_DOUBLE, &dur);
    return dur;
}

bool MpvPlayer::IsPaused() const {
    int flag = 0;
    api_.get_property(handle_, "pause", MPV_FORMAT_FLAG, &flag);
    return flag != 0;
}

void MpvPlayer::PumpEventsBlocking(std::function<void(mpv_event*)> onEvent) {
    while (handle_) {
        mpv_event* event = api_.wait_event(handle_, 1.0);
        if (!event) continue;
        if (event->event_id == MPV_EVENT_NONE) continue;
        if (onEvent) onEvent(event);
        if (event->event_id == MPV_EVENT_SHUTDOWN) break;
    }
}

void MpvPlayer::Shutdown() {
    if (renderCtx_) {
        api_.render_context_free(renderCtx_);
        renderCtx_ = nullptr;
    }
    if (handle_) {
        api_.terminate_destroy(handle_);
        handle_ = nullptr;
    }
}

} // namespace player::core
