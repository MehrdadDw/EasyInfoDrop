cmake_minimum_required(VERSION 3.16)
project(EasyInfoDrop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
if(NOT Qt5_FOUND)
    message(FATAL_ERROR "Qt5 Widgets not found. Install qtbase5-dev.")
endif()

find_package(X11 REQUIRED)
if(NOT X11_FOUND)
    message(FATAL_ERROR "X11 not found. Install libx11-dev.")
endif()

include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

add_executable(EasyInfoDrop src/main.cpp)
target_include_directories(EasyInfoDrop PRIVATE ${CMAKE_SOURCE_DIR}/src ${X11_INCLUDE_DIR})
target_link_libraries(EasyInfoDrop PRIVATE Qt5::Widgets ${X11_LIBRARIES} nlohmann_json::nlohmann_json)
