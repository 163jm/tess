// Core 模块的 CI 冒烟测试：不依赖 WinUI3，只验证：
//   1. MpvLoader 能成功加载 libmpv-2.dll 并解析所有函数指针
//   2. mpv_create / mpv_initialize / mpv_terminate_destroy 能正常调用
//   3. MpvPlayer 封装本身编译链接无误
// 不测试实际渲染（渲染依赖 WinUI3 的 SwapChainPanel，留给 build-app job）。

#include "MpvPlayer.h"
#include <iostream>
#include <filesystem>

int main(int argc, char** argv) {
    std::wstring dllPath;
    if (argc > 1) {
        // 允许 CI 显式传入 libmpv-2.dll 路径
        int size = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, nullptr, 0);
        dllPath.resize(size);
        MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, dllPath.data(), size);
    }

    try {
        player::core::MpvPlayer player;
        player.Initialize(dllPath);
        std::wcout << L"[smoke-test] mpv_create + mpv_initialize 成功" << std::endl;

        // 不加载真实视频文件（CI 环境没有视频），只验证属性读取不崩溃。
        std::wcout << L"[smoke-test] IsPaused() = " << player.IsPaused() << std::endl;

        player.Shutdown();
        std::wcout << L"[smoke-test] Shutdown 成功，测试通过" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[smoke-test] 失败: " << ex.what() << std::endl;
        return 1;
    }
}
