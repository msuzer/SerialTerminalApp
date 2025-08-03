# SerialTerminalApp

A clean, customizable serial terminal desktop application built with Qt 6.9.1 using CMake.

## ✨ Features

- Serial port connect/disconnect
- Configurable baud rate, parity, stop bits, and format (e.g., 8N1)
- Send/receive window with EOL settings
- Timestamped logging with auto-scroll
- Command history with persistent storage
- Light/Dark theme toggle
- Auto font scaling (Ctrl + Mouse Wheel)
- Deploy script (`deploy.ps1`) to build portable distribution

## 🔧 Requirements

- Qt 6.9.1 (with Qt SerialPort module)
- CMake or Qt Creator
- MinGW toolchain (or adjust for MSVC if needed)

## 🚀 Build Instructions

```bash
git clone https://github.com/msuzer/SerialTerminalApp.git
cd SerialTerminalApp
mkdir build && cd build
cmake ..
cmake --build .

Or open the folder in Qt Creator, select your kit, and build.

📦 Deployment
Use the deploy.ps1 script to generate a self-contained folder with all required Qt DLLs:

.\deploy.ps1 -Zip
Output: SerialTerminal/ + SerialTerminal_vX.Y.Z_YYYYMMDD_HHMMSS.zip

📁 Not in Repo
❗ This repository does not include:

Compiled .exe or .zip files

Qt DLLs (you must have Qt installed)

📜 License
MIT License — free to use and modify.