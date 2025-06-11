cmake_minimum_required(VERSION 3.10)
project(EasyFormDrop)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic moc for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

cmake_policy(SET CMP0135 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)

# Find Qt5
find_package(Qt5 COMPONENTS Widgets REQUIRED)
if(NOT Qt5_FOUND)
    message(FATAL_ERROR "Qt5 Widgets not found. Please install qtbase5-dev (Ubuntu) or equivalent.")
endif()

# Find X11
find_package(X11 REQUIRED)
if(NOT X11_FOUND)
    message(FATAL_ERROR "X11 not found. Please install libx11-dev (Ubuntu).")
endif()

# Fetch nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

# Add executable
add_executable(EasyFormDrop src/main.cpp)

# Link libraries
target_link_libraries(EasyFormDrop PRIVATE Qt5::Widgets ${X11_LIBRARIES} nlohmann_json::nlohmann_json)
target_include_directories(EasyFormDrop PRIVATE ${X11_INCLUDE_DIR} ${Qt5Widgets_INCLUDE_DIRS})

# Copy config file
configure_file(config/config.json config/config.json COPYONLY)
