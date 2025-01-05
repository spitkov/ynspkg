# YNS Package Manager

A lightweight package manager for Linux that uses a single repository source.

## Dependencies

- CMake (>= 3.10)
- C++17 compiler
- libcurl
- nlohmann-json

On Ubuntu/Debian, you can install the dependencies with:
```bash
sudo apt-get install build-essential cmake libcurl4-openssl-dev nlohmann-json3-dev
```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Installation

After building, you can install the package manager with:
```bash
sudo make install
```

This will:
- Install the `yns` binary to `/usr/local/bin`
- Create necessary directories:
  - `/var/cache/yns/` - For package cache
  - `/var/lib/yns/` - For installed package database

## Usage

### Update package cache
```bash
yns update
```

### Install a package
```bash
yns install <package_name>
```

### Remove a package
```bash
yns remove <package_name>
```

### Upgrade a package
```bash
yns upgrade <package_name>
```

### List all packages
```bash
yns list
```

### Interactive Mode
```bash
yns interactive
```

Interactive mode provides a shell-like interface where you can execute multiple commands without the `yns` prefix. Available commands in interactive mode:

- `help` - Show available commands
- `update` - Update package cache
- `install <package>` - Install a package
- `remove <package>` - Remove a package
- `upgrade <package>` - Upgrade a package
- `list` - List all packages
- `clear` - Clear the screen
- `exit` - Exit interactive mode

Example interactive session:
```
$ yns interactive
YNS Package Manager Interactive Mode
Type 'help' for available commands or 'exit' to quit
yns> update
[100%] Package cache updated successfully
yns> list
Available packages:
==================
calc [installed 1.1, update available 2.0]
notepad [available 2.0]
yns> install notepad
[50%] Installing notepad@2.0
notepad@2.0 installed successfully
yns> exit
Goodbye!
```

## Color Coding

The package manager uses color-coded output for better visibility:

🔵 Blue:
- Progress indicators (e.g., "[50%] Installing package...")
- Available but not installed packages in `yns list`
- Interactive mode prompt

🟢 Green:
- Success messages
- Installed packages that are up to date in `yns list`
- Interactive mode welcome message

🟡 Yellow:
- Installed packages with updates available in `yns list`

🔴 Red:
- Error messages

Example `yns list` output:
```
Available packages:
==================
calc [installed 1.1, update available 2.0]    # Yellow
notepad [available 2.0]                       # Blue
vim [installed 3.0]                           # Green
```

## Repository Structure

The package manager uses a single repository source at:
`https://raw.githubusercontent.com/spitkov/ynsrepo/refs/heads/main/repo.json`

The repository contains package information in JSON format, including:
- Package name
- Version
- Installation script URL
- Update script URL
- Remove script URL

## Features

- Colored terminal output
- Progress indicators
- Automatic package cache management
- Version tracking for installed packages
- Secure script execution
- Error handling and recovery
- Interactive shell mode 