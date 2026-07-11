# NativePlayer — 阶段 1：WinUI3 + libmpv 最小验证

这是 FlutterPlayer 项目 Windows 端原生化重写的**阶段 1** 产物：一个空壳
WinUI3 窗口，内嵌 `SwapChainPanel` 承载视频画面，通过 **libmpv**（运行时
动态加载）播放本地视频文件，验证整条渲染管线可行。

技术栈：**C++ / C++/WinRT / WinUI3 / libmpv（OpenGL + ANGLE 渲染路径）**

## 目录结构

```
NativePlayer/
├── CMakeLists.txt              # 顶层配置（主要用于 Core 独立编译验证）
├── Core/                       # 平台无关的播放器封装（静态库）
│   ├── MpvLoader.h/.cpp         # 运行时动态加载 libmpv-2.dll，规避 MSVC/mingw ABI 问题
│   └── MpvPlayer.h/.cpp         # mpv 播放控制 + OpenGL render context 封装
├── App/                        # WinUI3 应用外壳（需要在 Visual Studio 里搭建，见下）
│   ├── README_VS_PROJECT.md     # ⭐ 详细的 VS 项目搭建步骤，务必先读这个
│   ├── App.xaml / .h / .cpp     # 应用入口
│   ├── MainWindow.xaml          # 窗口布局：SwapChainPanel + 打开文件/播放暂停按钮
│   ├── MainWindow.xaml.h/.cpp   # 核心逻辑：D3D11 + ANGLE/EGL 初始化、mpv 渲染循环、UI事件
│   ├── pch.h/.cpp                # 预编译头
│   └── Package.appxmanifest      # 打包清单
└── third_party/libmpv/
    └── README.md                 # ⭐ libmpv 开发包下载与放置说明，务必先读这个
```

## 快速开始（在你的 Windows 机器上）

1. **准备环境**：按 `App/README_VS_PROJECT.md` 里"前置条件"安装 VS2022 相关组件
2. **下载 libmpv**：按 `third_party/libmpv/README.md` 下载并放置开发包
3. **搭建 VS 项目**：按 `App/README_VS_PROJECT.md` 剩余步骤，用 VS 模板创建
   WinUI3 项目，把本目录下 `App/` 和 `Core/` 的源码内容拷贝/合并进去
4. **安装 ANGLE**：NuGet 安装 `ANGLE.WindowsStore` 包
5. **编译运行**：F5，窗口弹出后点击"打开文件"选一个本地 mp4，应该能正常播放

## 已知需要你在本地确认/微调的点

因为我（Claude）没有 Windows/MSVC 环境实际编译验证，以下几点大概率需要你
在本地根据实际报错做小幅调整：

1. **ANGLE 头文件路径**：`eglext_angle.h`、`PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC`
   等符号名可能因 ANGLE 版本不同略有差异，若编译报"未定义"错误，检查
   NuGet 包里实际提供的头文件名和对应函数签名
2. **DXGI SwapEffect**：`DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL` 在部分 Windows
   版本组合下可能需要换成 `DXGI_SWAP_EFFECT_FLIP_DISCARD`，如果画面闪烁
   或创建 SwapChain 失败可以尝试切换
3. **mpv render API 的 `MPV_RENDER_PARAM_FLIP_Y`**：不同 mpv 版本这个参数
   的默认行为可能不同，如果画面上下颠倒，检查这个标志位
4. **WinUI3 与 Windows App SDK 版本**：`CMakeLists.txt` 里的
   `WINDOWSAPPSDK_VERSION` 是示例值，实际以你 NuGet 还原到的版本为准，
   通常不需要手动改（VS 项目模板会自动管理）

## 下一步（阶段 2 预告）

跑通阶段 1 后，下一步是把 FlutterPlayer 的 `video_player_page.dart` /
`video_player_controller.dart` 里的完整播放控制条（进度条拖拽、音量、
字幕/音轨切换、倍速）和视频库列表页移植过来。届时会引入：

- MVVM 结构（ViewModel 层，对应现有 GetX controller）
- SQLite 存储（对应 Hive）
- 完整的播放状态管理（对应 `PlayerMixin` / `PlayerStateMixin`）

确认阶段 1 在你本地跑通、没有原则性问题后，我们再继续。
