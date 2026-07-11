#pragma once
// -----------------------------------------------------------------------
// MpvPlayer: 封装 mpv_handle + mpv_render_context（OpenGL 后端）。
//
// 阶段 1 采用 OpenGL 渲染路径（配合 ANGLE 将 GL 调用转译到 D3D11），
// 这是社区里最成熟、坑最少的 mpv+Windows 集成方式（mpv.net、mpv-android
// 桌面移植等项目都类似）。后续如需极致性能，可以再迁移到 mpv 的原生
// D3D11 渲染接口，但复杂度显著更高，阶段 1 先不做。
// -----------------------------------------------------------------------

#include "MpvLoader.h"
#include <functional>
#include <memory>
#include <string>

namespace player::core {

// 渲染回调：当 mpv 有新帧需要绘制时触发（在非 UI 线程调用，
// 调用方需要自行切回 UI 线程再执行实际的 GL 渲染）。
using RenderReadyCallback = std::function<void()>;

class MpvPlayer {
public:
    MpvPlayer();
    ~MpvPlayer();

    MpvPlayer(const MpvPlayer&) = delete;
    MpvPlayer& operator=(const MpvPlayer&) = delete;

    // 第一步：创建 mpv 核心实例并初始化（不涉及渲染）。
    // libmpvDllPath 传入 libmpv-2.dll 的完整路径（通常是 exe 同目录）。
    void Initialize(const std::wstring& libmpvDllPath);

    // 第二步：绑定 OpenGL 渲染上下文。
    // getProcAddress 用于 mpv 内部初始化 GL 函数指针（对接 ANGLE/WGL）。
    using GlGetProcAddressFn = void* (*)(void* ctx, const char* name);
    void InitRenderContext(GlGetProcAddressFn getProcAddress, void* getProcAddressCtx);

    // 渲染一帧到指定的 FBO（由调用方在 GL 上下文当前线程调用）。
    void RenderFrame(int fbo, int width, int height);

    // 设置"有新帧待渲染"的回调（内部由 mpv 的更新线程触发，
    // 回调里请只做“post 消息到 UI 线程”，不要在回调里直接做 GL 调用）。
    void SetRenderReadyCallback(RenderReadyCallback cb);

    // 播放控制
    void LoadFile(const std::wstring& path);
    void Play();
    void Pause();
    void TogglePause();
    void Seek(double seconds, bool relative);
    void SetVolume(int volume0to100);

    double GetPositionSeconds() const;
    double GetDurationSeconds() const;
    bool IsPaused() const;

    // 事件循环：需要在独立线程中反复调用（阻塞等待 mpv 事件），
    // 阶段1先用简单轮询线程，事件目前只打印日志用于调试。
    void PumpEventsBlocking(std::function<void(mpv_event*)> onEvent);

    void Shutdown();

private:
    static void OnMpvRenderUpdate(void* ctx);

    MpvApi api_{}; // 缓存的函数表（来自 MpvLoader）
    mpv_handle* handle_ = nullptr;
    mpv_render_context* renderCtx_ = nullptr;
    RenderReadyCallback renderReadyCb_;
};

} // namespace player::core
