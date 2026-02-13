cmake_minimum_required(VERSION 3.14)
project(backend_notes)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/third_party)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)

find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2 REQUIRED sdl2)
include_directories(${SDL2_INCLUDE_DIRS})

pkg_check_modules(ZMQ REQUIRED libzmq)
include_directories(${ZMQ_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_library(imgui STATIC
    ${THIRD_PARTY_DIR}/imgui/imgui.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_sdl2.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${THIRD_PARTY_DIR}/imgui)
target_compile_options(imgui PRIVATE -fPIC)

add_library(implot STATIC
    ${THIRD_PARTY_DIR}/implot/implot.cpp
    ${THIRD_PARTY_DIR}/implot/implot_demo.cpp
    ${THIRD_PARTY_DIR}/implot/implot_items.cpp
)
target_include_directories(implot PUBLIC ${THIRD_PARTY_DIR}/implot)
target_link_libraries(implot PRIVATE imgui)
target_compile_options(implot PRIVATE -fPIC)

add_executable(main ${SRC_DIR}/main.cpp)

target_link_libraries(main PRIVATE
    imgui
    implot
    ${SDL2_LIBRARIES}
    ${ZMQ_LIBRARIES}
    GLEW::GLEW
    OpenGL::GL
    Threads::Threads
    dl
)
