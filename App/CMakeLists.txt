# -----------------------------------------------------------------------
# App: WinUI3 可执行文件外壳。
#
# 注意：WinUI3 + C++/WinRT 项目通常依赖 Visual Studio 的项目系统（.vcxproj）
# 来处理 XAML 编译（xaml compiler / midlrt / cppwinrt.exe 生成 .g.h/.g.cpp），
# 纯 CMake 驱动这一切较为繁琐。**强烈建议**：
#   1. 用 Visual Studio 2022 新建一个 "Blank App, Packaged (WinUI 3 in Desktop)"
#      模板项目，把下面的 MainWindow.xaml / .cpp / .h、App.xaml 等文件内容
#      拷贝替换进去；
#   2. 把 Core/ 目录作为一个引用的静态库项目或直接把源文件加入解决方案；
#   3. 把 third_party/libmpv 里的 DLL 设置为 "生成后复制到输出目录"。
#
# 本 CMakeLists.txt 保留用于：
#   - 让 Core 静态库可以独立编译验证（不依赖 WinUI3）
#   - 记录清晰的源文件清单，方便你在 VS 项目里对照添加
# 如果你更熟悉纯 MSBuild / VS 项目流程，可以直接跳过 CMake，参考
# App/README_VS_PROJECT.md。
# -----------------------------------------------------------------------

# 占位：实际 WinUI3 exe 目标建议在 Visual Studio 项目模板中创建。
# 这里仅声明源文件清单变量，供参考/未来 CMake 原生支持 WinUI3 后启用。
set(APP_SOURCES
    App.xaml
    App.xaml.h
    App.xaml.cpp
    MainWindow.xaml
    MainWindow.xaml.h
    MainWindow.xaml.cpp
    pch.h
    pch.cpp
)

message(STATUS "App/ 目录源文件已列出，请参考 App/README_VS_PROJECT.md 在 Visual Studio 中创建 WinUI3 项目并引入这些文件。")
