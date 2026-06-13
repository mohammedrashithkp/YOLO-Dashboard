cmake_minimum_required(VERSION 3.20)
project(YoloDashboard VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(ENABLE_HAILO "Enable HailoRT NPU inference support" OFF)

# ---------------------------------------------------------------------------
# Dependencies — required
# ---------------------------------------------------------------------------
find_package(OpenCV REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(spdlog REQUIRED)

# ---------------------------------------------------------------------------
# Dependencies — optional
# ---------------------------------------------------------------------------
find_package(libssh2 QUIET)
if(NOT libssh2_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBSSH2 IMPORTED_TARGET libssh2)
    endif()
endif()

find_package(onnxruntime QUIET)
if(NOT onnxruntime_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ONNXRUNTIME IMPORTED_TARGET libonnxruntime)
    endif()
endif()

if(NOT onnxruntime_FOUND AND NOT ONNXRUNTIME_FOUND)
    # Manual fallback for prebuilt binaries without CMake config
    find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h PATHS /usr/local/include /usr/include)
    find_library(ONNXRUNTIME_LIBRARY NAMES onnxruntime PATHS /usr/local/lib /usr/lib)
    if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIBRARY)
        set(onnxruntime_FOUND TRUE)
        add_library(onnxruntime::onnxruntime UNKNOWN IMPORTED)
        set_target_properties(onnxruntime::onnxruntime PROPERTIES
            IMPORTED_LOCATION "${ONNXRUNTIME_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_INCLUDE_DIR}"
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# FetchContent — Crow (header-only) and nlohmann/json
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
    GIT_SHALLOW    TRUE
)

# Crow options — disable examples/tests to speed up configuration
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(nlohmann_json Crow)

# Point Crow's static-file serving at the web/ directory during development
set(CROW_STATIC_DIRECTORY "${CMAKE_SOURCE_DIR}/web" CACHE STRING
    "Directory served as static files by Crow")
add_compile_definitions(CROW_STATIC_DIR="${CROW_STATIC_DIRECTORY}")

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/camera/camera_manager.cpp
    src/camera/video_recorder.cpp
    src/hardware/hardware_detector.cpp
    src/inference/onnx_engine.cpp
    src/config/config_manager.cpp
    src/auth/auth_manager.cpp
    src/metrics/metrics_collector.cpp
    src/metrics/log_manager.cpp
    src/web/routes.cpp
    src/web/websocket_handler.cpp
)

# Platform-specific camera enumerator
if(WIN32)
    list(APPEND SOURCES src/camera/camera_enumerator_win.cpp)
else()
    list(APPEND SOURCES src/camera/camera_enumerator_linux.cpp)
endif()

# ---------------------------------------------------------------------------
# Hailo NPU support (optional)
# ---------------------------------------------------------------------------
if(ENABLE_HAILO)
    find_package(HailoRT REQUIRED)
    list(APPEND SOURCES src/inference/hailo_engine.cpp)
    message(STATUS "HailoRT NPU support ENABLED")
endif()

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
    yaml-cpp
    spdlog::spdlog
    Crow::Crow
    nlohmann_json::nlohmann_json
)

# libssh2
if(libssh2_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE libssh2::libssh2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LIBSSH2_ENABLED)
elseif(LIBSSH2_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::LIBSSH2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LIBSSH2_ENABLED)
endif()

# ONNX Runtime
if(onnxruntime_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE onnxruntime::onnxruntime)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ONNXRUNTIME_ENABLED)
elseif(ONNXRUNTIME_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::ONNXRUNTIME)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ONNXRUNTIME_ENABLED)
endif()

# HailoRT
if(ENABLE_HAILO)
    target_link_libraries(${PROJECT_NAME} PRIVATE HailoRT::libhailort)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAILO_ENABLED)
endif()

# Platform-specific system libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 mswsock)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY web/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/web
)

install(DIRECTORY config/
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
)

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "=== YoloDashboard Build Configuration ===")
message(STATUS "  C++ Standard  : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "  OpenCV        : ${OpenCV_VERSION}")
message(STATUS "  spdlog        : found")
message(STATUS "  yaml-cpp      : found")
message(STATUS "  libssh2       : ${libssh2_FOUND}${LIBSSH2_FOUND}")
message(STATUS "  ONNX Runtime  : ${onnxruntime_FOUND}${ONNXRUNTIME_FOUND}")
message(STATUS "  Hailo NPU     : ${ENABLE_HAILO}")
message(STATUS "  Static Dir    : ${CROW_STATIC_DIRECTORY}")
message(STATUS "==========================================")
message(STATUS "")
