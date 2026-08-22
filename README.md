# SteamWorkshopDownloader

SteamWorkshopDownloader is a small cross-platform C++17 utility for downloading Steam Workshop items using SteamCMD.

The application accepts Steam Workshop URLs or Workshop item IDs, automatically determines the associated Steam App ID using the Steam Web API, and passes the download commands to SteamCMD.

The application currently supports:

- Windows
- Linux

> **Current version: 0.2.1**

## Features

- Accepts Steam Workshop URLs and plain Workshop item IDs
- Automatically detects the Steam App ID for each Workshop item
- Uses the Steam Web API to retrieve Workshop item details
- Downloads Workshop items through SteamCMD
- Creates a separate log file for each execution
- Archives processed `list.txt` files
- Uses native Windows WinHTTP for HTTPS communication on Windows
- Uses OpenSSL for HTTPS communication on Linux
- Uses platform-specific filesystem and time handling
- Builds using CMake
- Supports both Windows and Linux from the same source tree
- Does not require a graphical user interface

## Requirements

### Windows

For running a pre-built Windows release:

- Windows 10 or newer
- SteamCMD
- The MinGW runtime DLL files included with the release

You do not need:

- CMake
- Visual Studio
- MinGW-w64
- A C++ compiler

SteamCMD is an external dependency and is not included in this repository.

The application expects SteamCMD at:

    Steamcmd/steamcmd.exe

relative to `SWDownloader.exe`.

### Linux

The Linux version requires:

- A Linux distribution
- A C++17-compatible compiler
- CMake 3.20 or newer
- OpenSSL development libraries
- SteamCMD

On Debian/Ubuntu-based systems, install the required packages with:

    sudo apt update
    sudo apt install build-essential cmake libssl-dev

The important package for compiling the Linux version is:

    libssl-dev

It provides the OpenSSL headers and libraries required by the compiler and CMake.

The application itself does not contain OpenSSL. Linux provides the OpenSSL runtime libraries through the system.

## SteamCMD

SteamCMD is required on both supported platforms.

SteamCMD is an external dependency and is not distributed with this repository.

### Windows

The application expects:

    Steamcmd/
    └── steamcmd.exe

### Linux

The application expects:

    Steamcmd/
    └── steamcmd.sh

The Linux implementation starts SteamCMD through Bash.

Make sure the SteamCMD installation is complete before running SteamWorkshopDownloader.

## Download

For normal use, download the latest pre-built release from the GitHub Releases page.

Windows releases contain the executable and the required MinGW runtime DLLs.

Linux users can either:

- build the application from source
- use a pre-built Linux release if one is provided

If you want to build the application yourself, see the "Building from source" section below.

## Installation

### Windows

#### 1. Download the release

Download the latest Windows release ZIP and extract it to a directory of your choice.

A typical installation looks like:

    SteamWorkshopDownloader/
    ├── SWDownloader.exe
    ├── libgcc_s_seh-1.dll
    ├── libstdc++-6.dll
    ├── libwinpthread-1.dll
    │
    └── list.txt

#### 2. Install SteamCMD

Download SteamCMD separately and place `steamcmd.exe` inside:

    Steamcmd/

#### 3. Create `list.txt`

Create a file named:

    list.txt

next to `SWDownloader.exe`.

Add one Workshop URL or Workshop item ID per line.

Example:

    https://steamcommunity.com/sharedfiles/filedetails/?id=123456789
    123456789
    https://steamcommunity.com/sharedfiles/filedetails/?id=555555555

You can also use the included `list.example.txt` as a starting point.

#### 4. Run the application

Run:

    SWDownloader.exe

The application automatically creates the `logs` and `listArchive` directories when required.

### Linux

After building the Linux version, the directory should look similar to:

    SteamWorkshopDownloader/
    ├── SWDownloader
    ├── Steamcmd/
    │   └── steamcmd.sh
    └── list.txt

The application automatically creates:

    logs/

and:

    listArchive/

when required.

Run the application with:

    ./SWDownloader

If the executable does not have execute permission:

    chmod +x SWDownloader

Then run:

    ./SWDownloader

## Input format

Each line of `list.txt` may contain:

- a complete Steam Workshop URL containing an `id` parameter
- a numeric Workshop item ID

Examples:

    https://steamcommunity.com/sharedfiles/filedetails/?id=123456789
    123456789

Lines that do not contain a recognizable Workshop ID are ignored.

## How it works

When started, SteamWorkshopDownloader:

1. Determines its application directory.
2. Initializes the logging system.
3. Reads `list.txt`.
4. Extracts Workshop item IDs.
5. Queries the Steam Web API for each item.
6. Determines the associated Steam App ID.
7. Starts SteamCMD.
8. Requests the Workshop downloads.
9. Archives the processed `list.txt`.
10. Records the execution results in a log file.

If a Workshop item cannot be resolved, it is skipped and the problem is recorded in the log.

## Platform architecture

The project is divided into platform-independent and platform-specific code.

The source tree is:

    src/
    ├── core/
    │   ├── Logger.cpp
    │   └── Logger.h
    │
    ├── main/
    │   └── main.cpp
    │
    └── platform/
        ├── Platform.h
        ├── WindowsPlatform.cpp
        └── LinuxPlatform.cpp

### Core

The `core` directory contains functionality shared between platforms.

Currently this includes the logging system:

    src/core/
    ├── Logger.cpp
    └── Logger.h

### Main

The main application logic is located in:

    src/main/main.cpp

This code does not contain Windows-specific or Linux-specific networking implementation.

### Platform

Platform-specific functionality is located in:

    src/platform/

Windows:

    WindowsPlatform.cpp

Linux:

    LinuxPlatform.cpp

The common interface is declared in:

    Platform.h

CMake selects the appropriate implementation automatically.

## HTTPS implementation

SteamWorkshopDownloader communicates with the Steam Web API over HTTPS.

The implementation differs depending on the operating system.

### Windows

Windows uses the native WinHTTP API.

The Windows implementation links against:

    winhttp

through CMake.

No OpenSSL installation is required on Windows.

### Linux

Linux uses OpenSSL for HTTPS communication.

The Linux build therefore requires the OpenSSL development package.

On Debian/Ubuntu:

    sudo apt install libssl-dev

CMake locates OpenSSL automatically using `find_package(OpenSSL REQUIRED)`.

The Linux executable is linked against:

    OpenSSL::SSL
    OpenSSL::Crypto

## Building from source

The project uses CMake and supports separate builds for Windows and Linux.

It is recommended to use a separate build directory for each platform.

### Building on Linux

Install the required packages:

    sudo apt update
    sudo apt install build-essential cmake libssl-dev

Verify CMake:

    cmake --version

From the project root, configure the Linux build:

    cmake -S . -B build_linux

Build the application:

    cmake --build build_linux

The resulting executable will normally be:

    build_linux/SWDownloader

Run it with:

    ./build_linux/SWDownloader

Alternatively:

    cd build_linux
    ./SWDownloader

### Building on Windows

You can use a Windows C++ toolchain such as:

- Visual Studio
- MinGW-w64
- LLVM/Clang

Configure the project:

    cmake -S . -B build

Build the Release configuration:

    cmake --build build --config Release

The exact location of the resulting executable depends on the CMake generator.

For a typical Visual Studio build:

    build/
    └── Release/
        └── SWDownloader.exe

For a typical MinGW build:

    build/
    └── SWDownloader.exe

## Building with CLion

The project can be opened directly in CLion.

CLion uses the project's:

    CMakeLists.txt

to configure the build.

CMake automatically selects the correct platform implementation.

On Windows:

    WindowsPlatform.cpp

is compiled.

On Linux:

    LinuxPlatform.cpp

is compiled.

This means that the same source tree can be used on both operating systems.

## CMake configuration

The project requires:

    CMake 3.20+

and:

    C++17

The CMake configuration selects dependencies according to the target platform.

On Windows:

    WinHTTP

is linked.

On Linux:

    OpenSSL

is found and linked automatically.

The project intentionally does not compile both platform implementations at the same time.

## Logs

A separate log file is created for each execution.

Example:

    logs/
    ├── SWDownloader_2026-08-20_11-30-00.log
    ├── SWDownloader_2026-08-20_12-15-42.log
    └── SWDownloader_2026-08-20_13-05-21.log

Logs may contain:

- detected Workshop item IDs
- Steam Web API operations
- detected App IDs
- SteamCMD download operations
- errors and warnings
- archive operations
- application startup and shutdown information

The logger is shared between the Windows and Linux implementations.

## Archived lists

After a successful download process, the processed:

    list.txt

is moved to:

    listArchive/

Archived files receive a timestamped filename.

Example:

    listArchive/
    ├── list_2026-08-20_11-30-00.txt
    ├── list_2026-08-20_12-15-42.txt
    └── list_2026-08-20_13-05-21.txt

This provides a history of previously processed download lists.

## Dependencies

### C++ standard

The application requires:

    C++17

### CMake

Minimum supported version:

    CMake 3.20

### Windows

The Windows version uses:

    Windows API
    WinHTTP

WinHTTP is provided by Windows and is linked through CMake.

No OpenSSL installation is required for the Windows version.

### Linux

The Linux version uses:

    OpenSSL

On Debian/Ubuntu, install the development package with:

    sudo apt install libssl-dev

CMake detects the installed OpenSSL automatically.

### Steam Web API

SteamWorkshopDownloader uses the Steam Web API endpoint:

    ISteamRemoteStorage/GetPublishedFileDetails

to retrieve information about Workshop items and determine their associated application IDs.

An API key is not required for this endpoint.

### SteamCMD

SteamCMD performs the actual Workshop downloads.

SteamCMD is an external dependency and is not distributed with this repository.

## Windows runtime libraries

The pre-built Windows release is currently built using MinGW-w64.

The release package contains the required MinGW runtime DLLs:

    libgcc_s_seh-1.dll
    libstdc++-6.dll
    libwinpthread-1.dll

These files should remain in the same directory as:

    SWDownloader.exe

They are included in the Windows release package so that normal users do not need to install MinGW-w64 themselves.

## Linux runtime libraries

The Linux version uses the OpenSSL libraries provided by the Linux system.

The user normally does not need to manually copy OpenSSL `.so` files next to the application.

On Debian/Ubuntu-based systems, the required runtime libraries are normally provided by the system's OpenSSL packages.

For building from source, the development package is required:

    sudo apt install libssl-dev

The development package provides the headers and libraries required by CMake and the compiler.

## Typical project directory

The source repository should have a structure similar to:

    SteamWorkshopDownloader/
    ├── .gitignore
    ├── CMakeLists.txt
    ├── LICENSE
    ├── README.md
    ├── list.example.txt
    │
    ├── src/
    │   ├── core/
    │   │   ├── Logger.cpp
    │   │   └── Logger.h
    │   │
    │   ├── main/
    │   │   └── main.cpp
    │   │
    │   └── platform/
    │       ├── Platform.h
    │       ├── WindowsPlatform.cpp
    │       └── LinuxPlatform.cpp
    │
    ├── Steamcmd/
    │   ├── steamcmd.exe
    │   └── steamcmd.sh
    │
    ├── logs/
    ├── listArchive/
    ├── build/
    ├── build_linux/
    └── cmake-build-debug/

The `logs`, `listArchive` and build directories are generated/local directories and generally should not be committed to the repository.

SteamCMD is also an external dependency and should normally not be committed to the repository.

## Limitations

Version `0.2.1` currently has the following limitations:

- SteamCMD must be installed separately
- SteamCMD is currently used with anonymous login
- Workshop metadata is extracted from the API response using regular expressions
- HTTP handling is intentionally lightweight
- Network and API failures are reported through the log
- Invalid or unsupported Workshop items may be skipped
- No graphical user interface is provided
- The application processes the current `list.txt` as a batch
- Linux requires OpenSSL to be available on the system
- The application currently supports Windows and Linux

## Error handling

Errors and warnings are recorded in the log file created for the current execution.

For example, if SteamCMD cannot be found, the log records an error similar to:

    ERROR: SteamCMD executable not found

If an App ID cannot be determined for a Workshop item, that item is skipped and the problem is recorded in the log.

If OpenSSL is missing while configuring the Linux build, CMake reports an error similar to:

    Could NOT find OpenSSL

On Debian/Ubuntu, this can normally be fixed by installing:

    sudo apt install libssl-dev

After installing OpenSSL, reconfigure the project:

    cmake -S . -B build_linux

and build it again:

    cmake --build build_linux

## Cross-platform design

The application is designed so that the main application logic does not need to know which operating system is being used.

Platform-specific operations are hidden behind:

    Platform.h

Windows provides its implementation through:

    WindowsPlatform.cpp

Linux provides its implementation through:

    LinuxPlatform.cpp

CMake automaticly chooses the correct implementation:

This allows the same project to be compiled on both platforms without maintaining separate applications.

## Contributing

Contributions are welcome.

For changes:

1. Fork the repository.
2. Create a feature branch.
3. Make your changes.
4. Build and test the project.
5. Test the changes on the relevant platform.
6. Commit your changes.
7. Push the branch to your fork.
8. Open a Pull Request.

Please keep changes focused and provide a clear description of the changes.

## Disclaimer

SteamWorkshopDownloader is an independent open-source project and is not affiliated with, endorsed by, or sponsored by Valve Corporation.

Steam, SteamCMD, Steam Workshop, and related names and trademarks are property of their respective owners.

Users are responsible for complying with applicable laws, the terms applicable to their Steam account and Steam Workshop content, and the licenses or other usage restrictions associated with downloaded Workshop content.

## License

SteamWorkshopDownloader is released under the MIT License.

Copyright © 2026 Kearsia.

## Status

SteamWorkshopDownloader is currently released as version:

    0.2.1

The project currently supports:

    Windows
    Linux

The application is functional, but behavior and documentation may evolve in future versions.
