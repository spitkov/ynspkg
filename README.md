# YNS Package Manager

Simple package manager for Linux. Uses JSON for package definitions.

## Build

Install dependencies:
```bash
sudo apt-get install build-essential cmake libssl-dev zlib1g-dev
```

We build our own curl to make a static binary that works everywhere without dependencies.

Build steps:
```bash
bash build_curl.sh
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
yns update              # Update package cache
yns install <package>   # Install package
yns remove <package>    # Remove package
yns upgrade <package>   # Upgrade package
yns list               # List packages
yns debug              # Show debug info
yns interactive        # Interactive mode
```

## Package Format

```json
{
  "packages": {
    "package-name": {
      "version": "1.0.0",
      "description": "Package description",
      "install_script": "#!/bin/bash\n# Installation commands"
    }
  }
} 