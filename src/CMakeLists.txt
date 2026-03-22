add_library(sr_platform)

target_sources(sr_platform
    PRIVATE
        platform/Window.cpp)

target_include_directories(sr_platform
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(sr_platform PUBLIC cxx_std_20)

target_compile_definitions(sr_platform
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX)

target_link_libraries(sr_platform
    PRIVATE
        user32
        gdi32)

add_library(sr_engine)

target_sources(sr_engine
    PRIVATE
        app/Application.cpp
        app/layer/CameraLayer.cpp
        app/layer/RenderLayer.cpp
        app/layer/SceneLayer.cpp
        base/Math.cpp
        render/Camera.cpp
        render/Framebuffer.cpp
        resource/loaders/ImageTextureLoader.cpp
        resource/loaders/ObjMeshLoader.cpp)

target_include_directories(sr_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(sr_engine PUBLIC cxx_std_20)

target_compile_definitions(sr_engine
    PRIVATE
        SR_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
        WIN32_LEAN_AND_MEAN
        NOMINMAX)

target_link_libraries(sr_engine
    PUBLIC
        sr_platform
    PRIVATE
        ole32
        windowscodecs)

