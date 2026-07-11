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

## CI 自动编译（GitHub Actions）

`.github/workflows/build-windows.yml` 提供了两个 job，push/PR 到仓库或手动
触发（`workflow_dispatch`）都会跑：

1. **`build-core`**（稳，推荐先看这个）
   - 自动从 `shinchiro/mpv-winbuild-cmake` 的最新 GitHub Release 下载
     libmpv 开发包并放置到 `third_party/libmpv/`
   - 用 CMake + MSVC 只编译 `Core` 静态库和一个冒烟测试可执行文件
     （`Core/tests/smoke_test.cpp`：验证 DLL 能加载、`mpv_create/initialize/
     terminate_destroy` 调用不崩溃）
   - 真正**运行**冒烟测试 exe，日志里能直接看到成功/失败
   - 产物：`PlayerCore.lib`、`PlayerCoreSmokeTest.exe`

2. **`build-app`**（风险较高，`continue-on-error: true`，失败不会让整个
   workflow 标红，但日志会完整保留）
   - 同样先下载 libmpv
   - 用 `nuget restore` + `msbuild` 编译完整的 `NativePlayer.sln`
     （`App` WinUI3 项目 + `Core` 静态库，含 XAML 编译、Windows App SDK /
     C++/WinRT / ANGLE 的 NuGet 还原）
   - 产物：`build/msbuild.log`（详细编译日志，排查配置问题用）和
     编译出的 exe（如果成功）

### 怎么看结果

- 去仓库的 Actions 标签页，看 `Build NativePlayer (Windows)` 这个 workflow
  的运行记录
- `build-core` 是判断"mpv 封装代码本身没问题"的**基准信号**；如果这个都
  失败，说明 `Core/` 里的代码有编译期问题，需要先修
- `build-app` 大概率第一次跑会因为 NuGet 包版本号不对、vcxproj 里某个
  路径/GUID/属性配置有误而失败——**这是预期的**，下载 `msbuild-log`
  artifact，把报错内容发给我，我们照着日志逐条修

### 手动触发并指定 libmpv 版本

如果自动抓的"最新 release"版本有问题，可以在 Actions 页面手动触发
`workflow_dispatch`，在 `libmpv_release_tag` 输入框填一个具体的
release tag（去 <https://github.com/shinchiro/mpv-winbuild-cmake/releases>
看可用的 tag）。



跑通阶段 1 后，下一步是把 FlutterPlayer 的 `video_player_page.dart` /
`video_player_controller.dart` 里的完整播放控制条（进度条拖拽、音量、
字幕/音轨切换、倍速）和视频库列表页移植过来。届时会引入：

- MVVM 结构（ViewModel 层，对应现有 GetX controller）
- SQLite 存储（对应 Hive）
- 完整的播放状态管理（对应 `PlayerMixin` / `PlayerStateMixin`）

确认阶段 1 在你本地跑通、没有原则性问题后，我们再继续。
