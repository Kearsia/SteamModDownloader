# SteamModDownloader

SteamModDownloader is a small Windows utility written in C++17 that downloads Steam Workshop items using SteamCMD.

The application accepts Steam Workshop URLs or Workshop item IDs, automatically determines the associated Steam App ID using the Steam Web API, and passes the download commands to SteamCMD.

> **Current version: 0.1.0**

## Features

* Accepts Steam Workshop URLs and plain Workshop item IDs
* Automatically detects the Steam App ID for each Workshop item
* Uses the Steam Web API to retrieve Workshop item details
* Downloads Workshop items through SteamCMD
* Creates a separate log file for each execution
* Archives processed `list.txt` files
* Uses the Windows WinHTTP API for HTTPS communication
*  Does not require additional third-party C++ libraries beyond the runtime files included with the release

## Requirements

### Running a pre-built release

The pre-built release is intended for regular Windows users.

You need:

* Windows 10 or newer
* SteamCMD
* The runtime DLL files included with the release package

You do not need:

* CMake
* Visual Studio
* MinGW-w64
* A C++ compiler

SteamCMD is an external dependency and is not included in this repository.

The application expects SteamCMD at:

Steamcmd/steamcmd.exe

relative to `ModDownloader.exe`.

### Building from source

To build the project yourself, you need:

* C++17-compatible compiler
* CMake 3.20 or newer
* Windows C++ toolchain, such as:

  * Visual Studio
  * MinGW-w64
  * LLVM/Clang

## Download

For normal use, download the latest pre-built release from the GitHub Releases page.

The release package contains the executable and the MinGW-w64 runtime DLLs required by the current pre-built build.

If you want to build the application yourself, see [Building from source](#building-from-source).

## Installation

### 1. Download the release

Download the latest release ZIP from the GitHub Releases page and extract it to a directory of your choice.

### 2. Install SteamCMD

Download SteamCMD separately and place `steamcmd.exe` in the `Steamcmd` directoryy.

The resulting directory should look similar to:

SteamModDownloader/
├── ModDownloader.exe
├── libgcc_s_seh-1.dll
├── libstdc++-6.dll
├── libwinpthread-1.dll
├── Steamcmd/
│   └── steamcmd.exe
└── list.txt

### 3. Create `list.txt`

Create a file named `list.txt` next to `ModDownloader.exe`.

Add one Workshop URL or Workshop item ID per line.

Example:

https://steamcommunity.com/sharedfiles/filedetails/?id=123456789
123456789
https://steamcommunity.com/sharedfiles/filedetails/?id=555555555

You can also use the included `list.example.txt` as a starting point.

### 4. Run the application

Launch:

ModDownloader.exe

The application will create the `logs` and `listArchive` directories automatically when required.

## Input format

Each line of `list.txt` may contain:

* a complete Steam Workshop URL containing an `id` parameter, or
* a numeric Workshop item ID

Examples:

https://steamcommunity.com/sharedfiles/filedetails/?id=123456789
123456789

Lines that do not contain a recognizable Workshop ID are ignored.

## How it works

When started, SteamModDownloader:

1. Determines its application directory.
2. Initializes the logging system.
3. Reads `list.txt`.
4. Extracts Workshop item IDs.
5. Queries the Steam Web API for each item.
6. Determines the associated Steam App ID.
7. Starts SteamCMD and requests the Workshop downloads.
8. Archives the processed `list.txt`.
9. Records the execution results in a log file.

If a Workshop item cannot be resolved, it is skipped and the problem is recorded in the log.

## Logs

A separate log file is created for each execution.

Example:

logs/
├── ModDownloader_2026-08-20_11-30-00.log
├── ModDownloader_2026-08-20_12-15-42.log
└── ...

Logs may contain:

* detected Workshop item IDs
* Steam Web API requests
* detected App IDs
* SteamCMD download operations
* SteamCMD output
* errors and warnings
* archive operations

## Archived lists

After a successful download process, the processed `list.txt` is moved to `listArchive`.

Archived files receive a timestamped filename so previous lists are not overwritten.

Example:

listArchive/
├── list_2026-08-20_11-30-00.txt
├── list_2026-08-20_12-15-42.txt
└── ...

This provides a history of previously processed download lists.

## Building from source

Clone the repository and open a terminal in the project directory.

Configure the project:

cmake -S . -B build

Build the Release configuration:

cmake --build build --config Release

The location of the resulting executable depends on the CMake generator.

After building, place the executable and SteamCMD in the following structure:

ModDownloader/
├── ModDownloader.exe
├── Steamcmd/
│   └── steamcmd.exe
└── list.txt

Run the application with:

.\ModDownloader.exe

## Dependencies

### Windows API

The application uses the Windows API and WinHTTP for HTTPS communication.

The WinHTTP library is linked through CMake.

### Steam Web API

SteamModDownloader uses the Steam Web API `ISteamRemoteStorage/GetPublishedFileDetails` endpoint to retrieve information about Workshop items and determine their associated application IDs.

### SteamCMD

SteamCMD performs the actual Workshop downloads.

SteamCMD is an external dependency and is not distributed with this repository.

## Runtime libraries

The current pre-built Windows release is built using MinGW-w64 and includes the following runtime DLLs:

libgcc_s_seh-1.dll
libstdc++-6.dll
libwinpthread-1.dll

These files must remain in the same directory as `ModDownloader.exe`.

They are included in the release package for convenience.

## Limitations

Version `0.1.0` has the following limitations:

* Windows only
* SteamCMD must be installed separately
* SteamCMD is currently used with anonymous login
* Workshop metadata is extracted from the API response using regular expressions
* HTTP status codes are not handled in detail
* Network and API failures are reported through the log
* Invalid or unsupported Workshop items may be skipped
* No graphical user interface is provided
* The application processes the current `list.txt` as a batch

## Error handling

Errors and warnings are recorded in the log file created for the current execution.

For example, if SteamCMD cannot be found, the log records an error similar to:

ERROR: SteamCMD executable not found

If an App ID cannot be determined for a Workshop item, that item is skipped and the problem is recorded in the log.

## Project structure

SteamModDownloader/
├── .gitignore
├── CMakeLists.txt
├── LICENSE
├── README.md
├── list.example.txt
├── ModDownloader.cpp
│
├── Steamcmd/               # Local SteamCMD installation, not committed
├── logs/                   # Generated logs, not committed
├── listArchive/            # Archived lists, not committed
└── build/                  # Local build directory, not committed

The executable and MinGW runtime DLLs are release artifacts and should generally not be committed to the source repository unless you intentionally choose to distribute them from the repository itself.

## Contributing

Contributions are welcome.

For changes:

1. Fork the repository.
2. Create a feature branch.
3. Make your changes.
4. Build and test the project.
5. Commit your changes.
6. Push the branch to your fork.
7. Open a Pull Request.

Please keep changes focused and provide a clear description of the changes.

## Disclaimer

SteamModDownloader is an independent open-source project and is not affiliated with, endorsed by, or sponsored by Valve Corporation.

Steam, SteamCMD, Steam Workshop, and related names and trademarks are property of their respective owners.

Users are responsible for complying with applicable laws, the terms applicable to their Steam account and Steam Workshop content, and the licenses or other usage restrictions associated with downloaded Workshop content.

## License

SteamModDownloader is released under the MIT License.

Copyright © 2026 Kearsia.

## Status

SteamModDownloader is currently released as version `0.1.0`.

This is an early release. The project is functional, but behavior and documentation may evolve in future versions.
