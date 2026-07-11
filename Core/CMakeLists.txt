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
