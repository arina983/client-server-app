cmake_minimum_required(VERSION 3.14)
project(MobileMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(CURL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(ZMQ REQUIRED libzmq)

add_library(imgui STATIC
    ${THIRD_PARTY_DIR}/imgui/imgui.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_sdl2.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC 
    ${THIRD_PARTY_DIR}/imgui
    ${SDL2_INCLUDE_DIRS} 
)
target_link_libraries(imgui PUBLIC ${SDL2_LIBRARIES})
target_compile_options(imgui PRIVATE -fPIC)

add_library(implot STATIC
    ${THIRD_PARTY_DIR}/implot/implot.cpp
    ${THIRD_PARTY_DIR}/implot/implot_demo.cpp
    ${THIRD_PARTY_DIR}/implot/implot_items.cpp
)
target_include_directories(implot PUBLIC ${THIRD_PARTY_DIR}/implot)
target_link_libraries(implot PRIVATE imgui)
target_compile_options(implot PRIVATE -fPIC)

add_executable(MobileMonitor
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/server.cpp
    ${SRC_DIR}/gui.cpp
    ${SRC_DIR}/utils.cpp
    ${SRC_DIR}/map.cpp         
    ${SRC_DIR}/TileManager.cpp  
    ${SRC_DIR}/stb_image.cpp
)

target_include_directories(MobileMonitor PRIVATE
    ${SRC_DIR}
    ${CMAKE_SOURCE_DIR}/include 
    ${THIRD_PARTY_DIR}  
    ${THIRD_PARTY_DIR}/imgui
    ${THIRD_PARTY_DIR}/implot
    ${SDL2_INCLUDE_DIRS}
    ${ZMQ_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
)

target_link_libraries(MobileMonitor PRIVATE
    imgui
    implot
    ${SDL2_LIBRARIES}
    ${ZMQ_LIBRARIES}
    GLEW::GLEW
    OpenGL::GL
    Threads::Threads
    PostgreSQL::PostgreSQL
    CURL::libcurl
    dl
)

message(STATUS "SDL2 include dirs: ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2 libraries:    ${SDL2_LIBRARIES}")
message(STATUS "CURL found:        ${CURL_LIBRARIES}")