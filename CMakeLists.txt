cmake_minimum_required(VERSION 3.20)

project(ModDownloader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Unicode support for Windows API
add_definitions(-DUNICODE -D_UNICODE)


add_executable(
        ModDownloader
        ModDownloader.cpp
)


# Windows libraries
target_link_libraries(
        ModDownloader
        winhttp
)


# Output executable name
set_target_properties(
        ModDownloader
        PROPERTIES
        OUTPUT_NAME "ModDownloader"
)