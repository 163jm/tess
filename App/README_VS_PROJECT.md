# 如何在 Visual Studio 中搭建这个 WinUI3 项目

WinUI3 + C++/WinRT 的 XAML 编译（生成 `.g.h` / `.g.cpp`、`midlrt` 处理 `.idl`）
高度依赖 Visual Studio 的 MSBuild 项目系统。纯手写 CMake 驱动这套流程非常繁琐
且容易出问题，因此阶段 1 建议：**用 VS 项目模板 + 手动拷贝源码内容**，而不是
强行用 CMake 编译整个 App。（`Core` 静态库部分仍然可以纯 CMake 编译，或者也
一并放进同一个 VS 解决方案。）

## 前置条件

1. Visual Studio 2022（17.8 及以上），安装时勾选：
   - ".NET 桌面开发" 工作负载（WinUI3 模板依赖）
   - "使用 C++ 的桌面开发" 工作负载
   - 单个组件里勾选 **"C++/WinRT"** 和 **"Windows App SDK C++ 模板"**
2. 安装 **Windows App SDK** 扩展（VS 扩展管理器搜索 "Windows App SDK"，或第一次
   创建 WinUI3 项目时 VS 会提示自动安装）

## 创建项目

1. 新建项目 → 搜索 **"Blank App, Packaged (WinUI 3 in Desktop)"** → C++ 版本
   - 项目名：`NativePlayer.App`
   - 解决方案名：`NativePlayer`
2. VS 会生成默认的 `App.xaml`、`App.xaml.cpp/h`、`MainWindow.xaml`、
   `MainWindow.xaml.cpp/h`、`pch.h` 等文件。**用本目录（`App/`）下同名文件的
   内容替换/合并**（见下方文件说明）。
3. 右键解决方案 → 添加 → 现有项目 → 新建一个 **"静态库 (Static Library)"** 项目，
   命名 `PlayerCore`，把 `Core/` 目录下的 `.h/.cpp` 加入；或者更简单地，直接把
   `Core/*.cpp` 作为"现有项"添加进 `NativePlayer.App` 项目本身（阶段 1 先这样做
   更省事，后续再拆分静态库）。
4. 项目属性 → C/C++ → 常规 → 附加包含目录，加入：
   ```
   $(SolutionDir)Core
   $(SolutionDir)third_party\libmpv\include
   ```
5. **不需要**在"链接器 → 输入"里添加 mpv 相关 lib（我们用运行时动态加载，
   见 `Core/MpvLoader.h` 的说明）。
6. 生成后复制 DLL：项目属性 → 生成事件 → 生成后事件，添加命令行：
   ```
   xcopy /Y "$(SolutionDir)third_party\libmpv\lib\libmpv-2.dll" "$(OutDir)"
   ```
   这样每次编译后 `libmpv-2.dll` 会自动出现在输出目录，程序才能在运行时加载到。

## 本目录文件说明

- `pch.h` / `pch.cpp` — 预编译头，注意已加入 `<mpv/client.h>` 等所需头文件路径
- `App.xaml` / `App.xaml.h` / `App.xaml.cpp` — 应用入口，基本用 VS 默认模板即可，
  不需要特殊修改（阶段 1 逻辑都在 MainWindow 里）
- `MainWindow.xaml` — 声明了一个铺满窗口的 `SwapChainPanel`（承载视频画面）
  和底部一个简单的播放/暂停按钮，用于验证播放器已跑起来
- `MainWindow.xaml.h` / `.cpp` — 核心逻辑：
  - 初始化 D3D11 设备 + ANGLE(EGL) 上下文，绑定到 SwapChainPanel
  - 创建 `player::core::MpvPlayer`，走 `InitRenderContext` + `RenderFrame`
  - "打开文件"按钮触发文件选择对话框，调用 `LoadFile`
  - 一个 `DispatcherQueueTimer` 定期触发重绘（阶段 1 先用简单定时器，
    不做 mpv update callback 到 UI 线程的精细调度，跑通即可）

## 关于 ANGLE

WinUI3 的 `SwapChainPanel` 期望的是 D3D11 swapchain，而 mpv 的 render API
在阶段 1 我们选择走 OpenGL 后端。中间需要 **ANGLE**（Google 的 OpenGL ES →
D3D11 转译层）来桥接。

最简单的引入方式：NuGet 安装 `ANGLE.WindowsStore` 包（在 VS 里
"管理 NuGet 程序包" 搜索），它会提供 `libEGL.dll` / `libGLESv2.dll` 以及对应
头文件。`MainWindow.xaml.cpp` 里的 `InitializeAngle()` 函数演示了标准的
EGL 初始化流程（`eglGetPlatformDisplayEXT` 绑定到 SwapChainPanel 的
`ISwapChainPanelNative` 接口）。

如果 NuGet 包名变化或找不到，搜索 "ANGLE WinUI3 SwapChainPanel EGL" 能找到
微软官方 Direct3D 示例仓库里的等价初始化代码，可以直接参考替换。

## 验证成功的标志

1. 项目能编译通过（Core 部分独立可用 CMake 或 VS 静态库验证）
2. 运行后弹出一个窗口，点击"打开文件"选择一个本地 mp4，画面能正常播放
3. 播放/暂停按钮生效

跑通这一步后，我们再进入阶段 2（视频库列表 + 完整播放控制条）。
