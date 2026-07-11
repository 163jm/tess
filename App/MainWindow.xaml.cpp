#include "pch.h"
#include "MainWindow.xaml.h"
#include <windows.ui.xaml.media.dxinterop.h> // ISwapChainPanelNative
#include <EGL/eglext_angle.h>
#include <filesystem>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Foundation;

namespace winrt::NativePlayer::implementation {

HWND MainWindow::GetWindowHandle() {
    // WinUI3 Window 转 HWND 的标准方式：通过 IWindowNative 接口。
    auto windowNative = this->try_as<::IWindowNative>();
    HWND hwnd{};
    if (windowNative) {
        windowNative->get_WindowHandle(&hwnd);
    }
    return hwnd;
}

MainWindow::MainWindow() {
    InitializeComponent();

    dispatcherQueue_ = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();

    try {
        InitializeAngle();

        // 定位 libmpv-2.dll：假定与可执行文件同目录（见 App/README_VS_PROJECT.md
        // 里生成后事件的 xcopy 配置）。
        wchar_t exePath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::filesystem::path dllPath = std::filesystem::path(exePath).parent_path() / L"libmpv-2.dll";

        player_ = std::make_unique<player::core::MpvPlayer>();
        player_->Initialize(dllPath.wstring());
        player_->InitRenderContext(&MainWindow::GlGetProcAddress, eglDisplay_);

        // mpv 有新帧时会在非 UI 线程回调，这里只标记一个脏标志，
        // 真正的重绘通过下面的定时器在 UI 线程发生（阶段1的简化实现）。
        player_->SetRenderReadyCallback([this]() {
            renderPending_ = true;
        });

        // 简单的重绘定时器：约 60fps 轮询一次是否需要重绘。
        // 阶段2可以换成更精确的基于 DispatcherQueue.TryEnqueue 的回调驱动。
        redrawTimer_ = dispatcherQueue_.CreateTimer();
        redrawTimer_.Interval(std::chrono::milliseconds(16));
        redrawTimer_.Tick([this](auto&&, auto&&) { RenderIfNeeded(); });
        redrawTimer_.Start();

        StatusText().Text(L"播放器已初始化，等待打开文件");
    } catch (const std::exception& ex) {
        StatusText().Text(winrt::to_hstring(std::string("初始化失败: ") + ex.what()));
    }
}

MainWindow::~MainWindow() {
    if (redrawTimer_) {
        redrawTimer_.Stop();
    }
    player_.reset(); // 触发 MpvPlayer::Shutdown()
    DestroyEglSurface();
    if (eglDisplay_ != EGL_NO_DISPLAY) {
        eglTerminate(eglDisplay_);
    }
}

// -----------------------------------------------------------------------
// ANGLE / EGL 初始化：把 EGL 渲染目标绑定到 XAML 的 SwapChainPanel。
// 参考微软官方 Direct3D ANGLE 示例（"ANGLE SwapChainPanel" 相关样例代码）。
// -----------------------------------------------------------------------
void MainWindow::InitializeAngle() {
    // 1. 创建 D3D11 设备（ANGLE 需要一个外部提供的 D3D11 设备来创建 EGL Display）
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
    };
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    winrt::check_hresult(D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags,
        featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
        d3dDevice_.GetAddressOf(), nullptr, d3dContext_.GetAddressOf()));

    // 2. 用这个 D3D11 设备创建 EGL Display（ANGLE 扩展）
    const EGLint displayAttributes[] = {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
        EGL_NONE,
    };

    auto eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
        eglGetProcAddress("eglGetPlatformDisplayEXT"));
    if (!eglGetPlatformDisplayEXT) {
        throw std::runtime_error("找不到 eglGetPlatformDisplayEXT，ANGLE 未正确安装");
    }

    eglDisplay_ = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, d3dDevice_.Get(), displayAttributes);
    if (eglDisplay_ == EGL_NO_DISPLAY) {
        throw std::runtime_error("eglGetPlatformDisplayEXT 失败");
    }

    EGLint major, minor;
    if (!eglInitialize(eglDisplay_, &major, &minor)) {
        throw std::runtime_error("eglInitialize 失败");
    }

    const EGLint configAttributes[] = {
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE,
    };
    EGLint numConfigs = 0;
    if (!eglChooseConfig(eglDisplay_, configAttributes, &eglConfig_, 1, &numConfigs) || numConfigs == 0) {
        throw std::runtime_error("eglChooseConfig 失败");
    }

    const EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    eglContext_ = eglCreateContext(eglDisplay_, eglConfig_, EGL_NO_CONTEXT, contextAttributes);
    if (eglContext_ == EGL_NO_CONTEXT) {
        throw std::runtime_error("eglCreateContext 失败");
    }

    // 3. 把 EGL surface 绑定到 XAML SwapChainPanel（通过 ISwapChainPanelNative）
    auto panelNative = VideoPanel().as<ISwapChainPanelNative>();
    // 注意：真正创建 surface 需要知道 panel 尺寸，这里先延迟到
    // VideoPanel_SizeChanged 首次触发时再创建（见下方 CreateEglSurface）。
    // panelNative 在此处仅用于后续 SetSwapChain 调用，先保留引用逻辑在
    // CreateEglSurface 中完成。
    (void)panelNative;
}

void MainWindow::CreateEglSurface(int width, int height) {
    if (width <= 0 || height <= 0) return;
    DestroyEglSurface();

    const EGLint surfaceAttributes[] = {
        EGL_WIDTH, width, EGL_HEIGHT, height,
        EGL_FIXED_SIZE_ANGLE, EGL_TRUE,
        EGL_NONE,
    };

    // ANGLE 支持直接从 IDXGISwapChain 创建 EGL surface：
    // 先创建一个 DXGI SwapChain for Composition，再交给 ANGLE 包装成 EGLSurface，
    // 最后通过 ISwapChainPanelNative::SetSwapChain 绑定到 XAML 面板。
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    winrt::check_hresult(d3dDevice_.As(&dxgiDevice));
    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    winrt::check_hresult(dxgiDevice->GetAdapter(&dxgiAdapter));
    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
    winrt::check_hresult(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    winrt::check_hresult(dxgiFactory->CreateSwapChainForComposition(
        d3dDevice_.Get(), &swapChainDesc, nullptr, &swapChain));

    auto panelNative = VideoPanel().as<ISwapChainPanelNative>();
    winrt::check_hresult(panelNative->SetSwapChain(swapChain.Get()));

    eglSurface_ = eglCreatePlatformWindowSurfaceEXT
        ? reinterpret_cast<PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC>(
              eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT"))(
              eglDisplay_, eglConfig_, swapChain.Get(), surfaceAttributes)
        : eglCreateWindowSurface(eglDisplay_, eglConfig_,
              reinterpret_cast<EGLNativeWindowType>(swapChain.Get()), surfaceAttributes);

    if (eglSurface_ == EGL_NO_SURFACE) {
        throw std::runtime_error("创建 EGL surface 失败");
    }

    eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_);
}

void MainWindow::DestroyEglSurface() {
    if (eglSurface_ != EGL_NO_SURFACE) {
        eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(eglDisplay_, eglSurface_);
        eglSurface_ = EGL_NO_SURFACE;
    }
}

void* MainWindow::GlGetProcAddress(void* ctx, const char* name) {
    // ctx 在这里传入的是 eglDisplay_，mpv 只需要函数指针本身，ctx 不使用。
    (void)ctx;
    return reinterpret_cast<void*>(eglGetProcAddress(name));
}

void MainWindow::RenderIfNeeded() {
    if (!renderPending_ || !player_ || eglSurface_ == EGL_NO_SURFACE) return;
    renderPending_ = false;

    eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_);

    EGLint width = 0, height = 0;
    eglQuerySurface(eglDisplay_, eglSurface_, EGL_WIDTH, &width);
    eglQuerySurface(eglDisplay_, eglSurface_, EGL_HEIGHT, &height);

    // FBO 0 = 默认帧缓冲（即当前绑定的 EGL surface）
    player_->RenderFrame(/*fbo=*/0, width, height);

    eglSwapBuffers(eglDisplay_, eglSurface_);
}

// -----------------------------------------------------------------------
// UI 事件
// -----------------------------------------------------------------------

void MainWindow::OpenFileButton_Click(IInspectable const&, RoutedEventArgs const&) {
    // 注意：文件选择器在 WinUI3 桌面应用里需要绑定窗口句柄（见下方 InitializeWithWindow）。
    auto picker = winrt::Windows::Storage::Pickers::FileOpenPicker();
    picker.SuggestedStartLocation(winrt::Windows::Storage::Pickers::PickerLocationId::VideosLibrary);
    picker.FileTypeFilter().Append(L".mp4");
    picker.FileTypeFilter().Append(L".mkv");
    picker.FileTypeFilter().Append(L".avi");

    auto initializeWithWindow = picker.as<::IInitializeWithWindow>();
    initializeWithWindow->Initialize(GetWindowHandle());

    auto self = this;
    picker.PickSingleFileAsync().Completed(
        [self](auto&& asyncOp, auto&& status) {
            if (status != winrt::Windows::Foundation::AsyncStatus::Completed) return;
            auto file = asyncOp.GetResults();
            if (!file) return;

            self->dispatcherQueue_.TryEnqueue([self, path = file.Path()]() {
                self->player_->LoadFile(path.c_str());
                self->player_->Play();
                self->StatusText().Text(L"正在播放: " + path);
            });
        });
}

void MainWindow::PlayPauseButton_Click(IInspectable const&, RoutedEventArgs const&) {
    if (!player_) return;
    player_->TogglePause();
    StatusText().Text(player_->IsPaused() ? L"已暂停" : L"播放中");
}

void MainWindow::VideoPanel_SizeChanged(IInspectable const&, SizeChangedEventArgs const& args) {
    auto size = args.NewSize();
    CreateEglSurface(static_cast<int>(size.Width), static_cast<int>(size.Height));
}

} // namespace winrt::NativePlayer::implementation
