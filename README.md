# YNS Package Manager

Simple package manager for Linux. Uses JSON for package definitions.

## Build

Install dependencies:
```bash
sudo apt-get install build-essential cmake libssl-dev zlib1g-dev nlohmann-json3-dev
```

We build our own curl because the system's libcurl is usually only available as a dynamic library. Building our own lets us:
- Create a static binary that works everywhere
- Include only the features we need (HTTP support)
- Avoid dependency problems during static linking

Build steps:
```bash
# Get the code
git clone https://github.com/spitkov/ynspkg.git
cd ynspkg

# Build static curl
chmod +x build_curl.sh
./build_curl.sh

# Build yns
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
yns version            # Show YNS version
yns updateyns          # Update YNS to latest version
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