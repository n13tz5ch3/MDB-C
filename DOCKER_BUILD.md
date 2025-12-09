# Docker Cross-Compilation for MDB-C

Quick guide for cross-compiling MDB-C to Linux ARM64 using Docker on macOS.

## Quick Start

```bash
# Build for ARM64 (one command!)
make arm64
```

That's it! Your ARM64 binaries will be in `dist/linux-arm64/`.

## Requirements

- Docker Desktop for Mac (https://www.docker.com/products/docker-desktop)
- No ARM toolchain installation needed!

## Available Commands

### Using Makefile (Recommended)

```bash
make arm64           # Build for ARM64
make arm64-clean     # Clean build artifacts
make arm64-rebuild   # Clean and rebuild
make arm64-shell     # Open interactive shell in container
make docker-image    # Build/rebuild Docker image
make help            # Show all available commands
```

### Using Script Directly

```bash
./docker-build-arm64.sh build      # Build for ARM64
./docker-build-arm64.sh clean      # Clean artifacts
./docker-build-arm64.sh rebuild    # Clean and rebuild
./docker-build-arm64.sh shell      # Interactive shell
./docker-build-arm64.sh image      # Build Docker image only
./docker-build-arm64.sh help       # Show help
```

## What Gets Built

After a successful build, you'll find:

**Binaries** (`dist/linux-arm64/`):
- `mdb_test` - Test application
- `mdb_threaded` - Threaded example
- `mdb43_basket_demo` - MDB 4.3 basket demo
- `mdb_sniffer_app` - Bus sniffer tool

**Libraries** (`build/docker-arm64/`):
- `libmdb_protocol.a` - Core MDB protocol
- `libmdb_hal.a` - Linux HAL
- `libmdb_hal_callback.a` - Callback HAL (thread-safe)

## Verification

The build script automatically verifies ARM64 architecture:

```bash
$ ./docker-build-arm64.sh build
...
==> Verifying ARM64 architecture...
Checking: dist/linux-arm64/mdb_test
dist/linux-arm64/mdb_test: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV)
✓ Build verification complete
```

## Deploying to ARM64 Device

### Option 1: Direct Copy

```bash
# Copy to device
scp -r dist/linux-arm64/* user@raspberry-pi:/opt/mdb-c/

# Test on device
ssh user@raspberry-pi
cd /opt/mdb-c
./mdb_test --help
```

### Option 2: Create Tarball

```bash
# Create distribution archive
cd dist
tar -czf mdb-c-linux-arm64.tar.gz linux-arm64/

# Copy to device
scp mdb-c-linux-arm64.tar.gz user@device:~

# Extract on device
ssh user@device
tar -xzf mdb-c-linux-arm64.tar.gz
cd linux-arm64
./mdb_test --help
```

## Directory Structure

```
MDB-C/
├── Dockerfile.arm64           # Docker build definition
├── docker-build-arm64.sh      # Automated build script
├── .dockerignore              # Optimizes Docker context
├── Makefile                   # Convenient shortcuts
├── cmake/
│   └── toolchains/
│       └── linux-arm64.cmake  # ARM64 toolchain config
├── build/
│   └── docker-arm64/          # Docker build output
│       ├── libmdb_protocol.a
│       ├── libmdb_hal.a
│       └── libmdb_hal_callback.a
└── dist/
    └── linux-arm64/           # Distribution binaries
        ├── mdb_test
        ├── mdb_threaded
        ├── mdb43_basket_demo
        └── mdb_sniffer_app
```

## Troubleshooting

### Docker Not Running

```bash
$ make arm64
✗ Failed to connect to Docker daemon

Solution: Start Docker Desktop
```

### Permission Denied

```bash
$ make arm64
bash: ./docker-build-arm64.sh: Permission denied

Solution:
chmod +x docker-build-arm64.sh
```

### Out of Disk Space

```bash
# Clean Docker cache
docker system prune -a

# Remove old containers
docker container prune

# Remove unused images
docker image prune -a
```

### Build Fails

```bash
# Rebuild Docker image
make docker-image

# Clean and rebuild
make arm64-rebuild
```

### Wrong Architecture

If binaries show `x86_64` instead of `aarch64`:

```bash
# Rebuild Docker image from scratch
docker rmi mdb-c-arm64-builder
make docker-image

# Rebuild project
make arm64-rebuild

# Verify
file dist/linux-arm64/mdb_test
# Should show: ARM aarch64
```

## Advanced Usage

### Custom Build Configuration

Edit `docker-build-arm64.sh` and modify the CMake configuration:

```bash
# Find this section:
cmake -B $BUILD_DIR \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-arm64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DMDB_VERSION_43=ON \
    -DBUILD_EXAMPLES=ON

# Add your options:
    -DMDB_VERBOSE_LOGGING=ON \
    -DMDB_TTY_DEVICE=/dev/ttyACM0
```

### Interactive Development

```bash
# Open shell in build container
make arm64-shell

# Inside container, you can:
root@container:/workspace# cmake --version
root@container:/workspace# aarch64-linux-gnu-gcc --version
root@container:/workspace# ls -la
root@container:/workspace# cmake -B build/test ...
```

### Manual Docker Commands

```bash
# Build image
docker build -f Dockerfile.arm64 -t mdb-c-arm64-builder .

# Run build manually
docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  mdb-c-arm64-builder \
  bash -c "
    cmake -B build/docker-arm64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-arm64.cmake \
      -DCMAKE_BUILD_TYPE=Release
    cmake --build build/docker-arm64
  "

# Check binary architecture
docker run --rm \
  -v "$(pwd):/workspace" \
  mdb-c-arm64-builder \
  file build/docker-arm64/examples/mdb_test
```

## Why Docker?

**Benefits:**
- No toolchain installation on macOS
- Consistent build environment
- Same process works on any platform
- Reproducible builds
- Easy CI/CD integration

**vs Native Cross-Compilation:**
- Native: Requires ARM toolchain installation
- Native: Platform-specific setup
- Docker: Works everywhere Docker runs
- Docker: Isolated from host system

## CI/CD Integration

See [BUILD_PLATFORMS.md](BUILD_PLATFORMS.md#cicd-integration) for GitHub Actions examples.

## More Information

- Full build documentation: [BUILD_PLATFORMS.md](BUILD_PLATFORMS.md)
- MDB 4.3 features: [CLAUDE.md](CLAUDE.md)
- Project README: [README.md](README.md)

## Support

Target platforms for ARM64 build:
- Raspberry Pi 3/4/5 (64-bit OS)
- NVIDIA Jetson (Nano, TX2, Xavier, Orin)
- AWS Graviton instances
- Apple M1/M2 running Linux (via VM)
- Generic ARM64/AArch64 Linux systems

Tested on:
- macOS 13+ (Apple Silicon and Intel)
- Linux x86_64 with Docker

## License

Same as main project - see [LICENSE](LICENSE)
