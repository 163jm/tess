#pragma once

#include "MainWindow.xaml.g.h"
#include "MpvPlayer.h"
#include <winrt/Microsoft.UI.Dispatching.h>
#include <wrl/client.h>

namespace winrt::NativePlayer::implementation {

struct MainWindow : MainWindowT<MainWindow> {
    MainWindow();
    ~MainWindow();

    void OpenFileButton_Click(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void PlayPauseButton_Click(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void VideoPanel_SizeChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& args);

private:
    HWND GetWindowHandle();

    // --- ANGLE / EGL 初始化 ---
    void InitializeAngle();
    void CreateEglSurface(int width, int height);
    void DestroyEglSurface();
    void RenderIfNeeded();

    EGLDisplay eglDisplay_ = EGL_NO_DISPLAY;
    EGLContext eglContext_ = EGL_NO_CONTEXT;
    EGLConfig eglConfig_ = nullptr;
    EGLSurface eglSurface_ = EGL_NO_SURFACE;

    Microsoft::WRL::ComPtr<ID3D11Device> d3dDevice_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3dContext_;

    // --- mpv 播放器 ---
    std::unique_ptr<player::core::MpvPlayer> player_;
    bool renderPending_ = false;

    // --- UI 线程调度 ---
    winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcherQueue_{ nullptr };
    winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer redrawTimer_{ nullptr };

    static void* GlGetProcAddress(void* ctx, const char* name);
};

} // namespace winrt::NativePlayer::implementation

namespace winrt::NativePlayer::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}
