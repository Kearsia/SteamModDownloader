cmake_minimum_required(VERSION 3.20)

project(ModDownloader
        VERSION 0.2.0
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMMON_SOURCES
        src/main/ModDownloader.cpp
        src/core/Logger.cpp
        src/core/Logger.h
        src/platform/Platform.h
)

if (WIN32)
    message(STATUS "Configuring ModDownloader for Windows")
    add_executable(
            ModDownloader
            ${COMMON_SOURCES}
            src/platform/WindowsPlatform.cpp
    )
    target_link_libraries(
            ModDownloader
            PRIVATE
            winhttp
    )

elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Configuring ModDownloader for Linux")
    find_package(OpenSSL REQUIRED)
    add_executable(
            ModDownloader
            ${COMMON_SOURCES}
            src/platform/LinuxPlatform.cpp
    )
    target_link_libraries(
            ModDownloader
            PRIVATE
            OpenSSL::SSL
            OpenSSL::Crypto
    )
else ()
    message(
            FATAL_ERROR
            "Unsupported operating system: ${CMAKE_SYSTEM_NAME}"
    )

endif ()

target_include_directories(
        ModDownloader
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
)
