add_library(PlayerCore STATIC
    MpvLoader.h
    MpvLoader.cpp
    MpvPlayer.h
    MpvPlayer.cpp
)

target_include_directories(PlayerCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LIBMPV_INCLUDE_DIR}
)

target_link_libraries(PlayerCore
    PUBLIC
        d3d11.lib
        dxgi.lib
        opengl32.lib
)

# CI 冒烟测试：验证 Core 能编译链接，并且在运行时能成功加载 libmpv-2.dll、
# 完成 mpv_create/initialize/terminate_destroy 的基本调用。
option(NATIVEPLAYER_BUILD_TESTS "编译 Core 的冒烟测试可执行文件" ON)
if(NATIVEPLAYER_BUILD_TESTS)
    add_executable(PlayerCoreSmokeTest tests/smoke_test.cpp)
    target_link_libraries(PlayerCoreSmokeTest PRIVATE PlayerCore)
endif()
