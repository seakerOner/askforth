# How to build AskForth

This document will guide you on how to build AskForth for different platforms

## Platform Status

| Platform | Status |
|----------|--------|
| Linux    | Supported |
| Windows  | Supported |
| ESP32    | Planned |
| skdojo   | Planned |

## Build on Linux

To build AskForth on Linux you will need:
    - GCC
    - make

Enter the project directory and run

```bash
make x86-64-linux
```

The executable will be generated at: 

```text
build/x86-64-linux/AskForth
```

## Build on Windows

AskForth can be built on Windows using [MSYS2](https://www.msys2.org/) with the UCRT64 GCC toolchain.

### 1. Install MSYS2

Install MSYS2 and open the **MSYS2 UCRT64** terminal.

Update the MSYS2 package database and system:

```bash
pacman -Syu
```

If MSYS2 asks you to close the terminal after the update, close it, open **MSYS2 UCRT64** again, and run:

```bash
pacman -Syu
```

### 2. Install the toolchain

Install GCC and GNU Make for the UCRT64 environment:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc make
```

Verify the installation:

```bash
gcc --version
make --version
```

### 3. Build AskForth

Enter the AskForth project directory:

```bash
cd /path/to/AskForth
```

Build the Windows target:

```bash
make x86-64-windows
```

The executable will be generated as:

```text
build/askforth.exe
```

### 4. Run AskForth

From the MSYS2 UCRT64 terminal:

```bash
./build/askforth.exe
```

The generated executable is a native Windows executable and can also be run from Windows PowerShell or Command Prompt:

```powershell
.\build\x86-64-windows\askforth.exe
```

### MSYS2 Environment

AskForth uses the **UCRT64** environment for its Windows build.

The required packages are:

```text
mingw-w64-ucrt-x86_64-gcc
make
```

The MSYS2 environment is only used as the build environment. AskForth itself is compiled as a native Windows executable.

### Windows Security Warning

The Windows executable is currently **unsigned**.

Depending on the Windows security configuration, Windows Smart App Control or SmartScreen may prevent the executable from running or display a security warning.

This is expected for development builds.

The project is open-source, and code signing for distributed Windows releases may be added in the future.
