cmake_minimum_required(VERSION 3.15)
project(OverlayApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.89.15
)
FetchContent_MakeAvailable(imgui)

add_executable(overlay src/main.cpp)
target_compile_definitions(overlay PRIVATE UNICODE _UNICODE)
target_include_directories(overlay PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_sources(overlay PRIVATE
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)
target_link_libraries(overlay PRIVATE d3d11 dxgi d3dcompiler)

add_executable(write_coords src/write_coords.cpp)
target_compile_definitions(write_coords PRIVATE UNICODE _UNICODE)

if(MSVC)
  target_compile_options(overlay PRIVATE /W4 /permissive-)
  target_compile_options(write_coords PRIVATE /W4 /permissive-)
endif()
