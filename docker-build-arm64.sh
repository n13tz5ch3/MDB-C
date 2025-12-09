#!/bin/bash
# Cross-compile MDB-C for Linux ARM64 using Docker on macOS
# Usage: ./docker-build-arm64.sh [clean|rebuild|shell]

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
IMAGE_NAME="mdb-c-arm64-builder"
CONTAINER_NAME="mdb-c-arm64-build"
BUILD_DIR="build/docker-arm64"
DIST_DIR="dist/linux-arm64"

# Print colored message
print_msg() {
    echo -e "${BLUE}==>${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# Clean build artifacts
clean_build() {
    print_msg "Cleaning build artifacts..."
    rm -rf "$BUILD_DIR"
    print_success "Build directory cleaned"
}

# Build Docker image
build_docker_image() {
    print_msg "Building Docker image: $IMAGE_NAME"

    if docker build -f Dockerfile.arm64 -t "$IMAGE_NAME" .; then
        print_success "Docker image built successfully"
    else
        print_error "Failed to build Docker image"
        exit 1
    fi
}

# Check if Docker image exists
check_docker_image() {
    if ! docker image inspect "$IMAGE_NAME" &> /dev/null; then
        print_warning "Docker image not found. Building..."
        build_docker_image
    else
        print_success "Docker image found: $IMAGE_NAME"
    fi
}

# Run cross-compilation inside Docker
run_docker_build() {
    print_msg "Starting cross-compilation for ARM64..."

    # Create build directory
    mkdir -p "$BUILD_DIR"

    # Run Docker container with build
    docker run --rm \
        --name "$CONTAINER_NAME" \
        -v "$(pwd):/workspace" \
        -w /workspace \
        "$IMAGE_NAME" \
        bash -c "
            set -e
            echo '==> Configuring CMake for ARM64...'
            cmake -B $BUILD_DIR \
                -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-arm64.cmake \
                -DCMAKE_BUILD_TYPE=Release \
                -DMDB_VERSION_43=ON \
                -DBUILD_EXAMPLES=ON

            echo '==> Building for ARM64...'
            cmake --build $BUILD_DIR -j\$(nproc)

            echo '==> Build completed successfully!'
        "

    if [ $? -eq 0 ]; then
        print_success "Cross-compilation completed successfully"

        # Create distribution directory
        print_msg "Creating distribution package..."
        mkdir -p "$DIST_DIR"

        # Copy binaries
        if [ -d "$BUILD_DIR/examples" ]; then
            cp -r "$BUILD_DIR/examples/"mdb* "$DIST_DIR/" 2>/dev/null || true
        fi

        # Copy libraries
        if [ -d "$BUILD_DIR" ]; then
            find "$BUILD_DIR" -name "*.a" -exec cp {} "$DIST_DIR/" \; 2>/dev/null || true
        fi

        print_success "Binaries available in: $DIST_DIR"

        # Verify ARM64 architecture
        print_msg "Verifying ARM64 architecture..."
        docker run --rm \
            -v "$(pwd):/workspace" \
            -w /workspace \
            "$IMAGE_NAME" \
            bash -c "
                for binary in $DIST_DIR/mdb*; do
                    if [ -f \"\$binary\" ] && [ -x \"\$binary\" ]; then
                        echo \"Checking: \$binary\"
                        file \"\$binary\"
                    fi
                done
            "

        print_success "Build verification complete"
    else
        print_error "Cross-compilation failed"
        exit 1
    fi
}

# Create distribution package using build.sh
run_docker_package() {
    print_msg "Creating distribution package for ARM64..."

    # Ensure build exists first
    if [ ! -d "$BUILD_DIR" ]; then
        print_warning "Build not found. Building first..."
        run_docker_build
    fi

    # Run build.sh package inside Docker
    docker run --rm \
        --name "$CONTAINER_NAME-package" \
        -v "$(pwd):/workspace" \
        -w /workspace \
        "$IMAGE_NAME" \
        bash -c "
            set -e
            # Use linux-arm-release preset for ARM64 (closest match)
            # The preset doesn't exist, so we'll create the package manually

            echo '==> Creating distribution package...'

            # Create package structure
            PACKAGE_DIR=\"dist/linux-arm64-release\"
            mkdir -p \"\$PACKAGE_DIR/lib\"
            mkdir -p \"\$PACKAGE_DIR/include/mdb/protocol\"
            mkdir -p \"\$PACKAGE_DIR/include/mdb/hal\"
            mkdir -p \"\$PACKAGE_DIR/include/mdb/devices\"
            mkdir -p \"\$PACKAGE_DIR/bin\"
            mkdir -p \"\$PACKAGE_DIR/docs\"

            # Copy libraries
            echo '  Copying libraries...'
            cp $BUILD_DIR/*.a \"\$PACKAGE_DIR/lib/\" 2>/dev/null || true
            cp $BUILD_DIR/Devices/*.a \"\$PACKAGE_DIR/lib/\" 2>/dev/null || true

            # Copy binaries
            echo '  Copying binaries...'
            cp $BUILD_DIR/examples/mdb_* \"\$PACKAGE_DIR/bin/\" 2>/dev/null || true
            chmod +x \"\$PACKAGE_DIR/bin/\"* 2>/dev/null || true

            # Copy protocol headers
            echo '  Copying protocol headers...'
            cp Protocol_Files/Communication_Format.h \"\$PACKAGE_DIR/include/mdb/protocol/\"
            cp Protocol_Files/Bus_Timing.h \"\$PACKAGE_DIR/include/mdb/protocol/\"
            cp Protocol_Files/PreProcessors.h \"\$PACKAGE_DIR/include/mdb/protocol/\"
            cp Protocol_Files/EVA-DTS.h \"\$PACKAGE_DIR/include/mdb/protocol/\" 2>/dev/null || true
            cp Protocol_Files/FTL.h \"\$PACKAGE_DIR/include/mdb/protocol/\" 2>/dev/null || true

            # Copy HAL headers
            echo '  Copying HAL headers...'
            cp MDB_Linux.h \"\$PACKAGE_DIR/include/mdb/hal/\"
            cp MDB_Callbacks.h \"\$PACKAGE_DIR/include/mdb/hal/\"

            # Copy device headers
            echo '  Copying device headers...'
            find Devices -name '*.h' -exec cp --parents {} \"\$PACKAGE_DIR/include/mdb/\" \\; 2>/dev/null || true

            # Copy documentation
            echo '  Copying documentation...'
            cp README.md \"\$PACKAGE_DIR/docs/\" 2>/dev/null || true
            cp BUILD_PLATFORMS.md \"\$PACKAGE_DIR/docs/\" 2>/dev/null || true
            cp DOCKER_BUILD.md \"\$PACKAGE_DIR/docs/\" 2>/dev/null || true
            cp LICENSE \"\$PACKAGE_DIR/docs/\" 2>/dev/null || true

            # Create tarball
            echo '  Creating tarball...'
            cd dist
            tar -czf mdb-c-linux-arm64-4.3.0.tar.gz linux-arm64-release/
            cd ..

            echo '==> Package created successfully!'
            echo \"Package: dist/mdb-c-linux-arm64-4.3.0.tar.gz\"
            ls -lh dist/mdb-c-linux-arm64-4.3.0.tar.gz
        "

    if [ $? -eq 0 ]; then
        print_success "Distribution package created successfully"
        print_msg "Package contents:"
        tar -tzf dist/mdb-c-linux-arm64-4.3.0.tar.gz | head -20
        echo "..."
        print_success "Package: dist/mdb-c-linux-arm64-4.3.0.tar.gz"
    else
        print_error "Package creation failed"
        exit 1
    fi
}

# Open interactive shell in Docker container
run_docker_shell() {
    print_msg "Opening interactive shell in Docker container..."

    docker run --rm -it \
        --name "$CONTAINER_NAME-shell" \
        -v "$(pwd):/workspace" \
        -w /workspace \
        "$IMAGE_NAME" \
        bash
}

# Print usage
print_usage() {
    cat << EOF
Usage: $0 [command]

Commands:
  build       Cross-compile for Linux ARM64 (default)
  package     Create distribution package (.tar.gz)
  clean       Remove build artifacts
  rebuild     Clean and build
  shell       Open interactive shell in Docker container
  image       Build/rebuild Docker image only
  help        Show this help message

Examples:
  $0                    # Build for ARM64
  $0 package            # Create distribution package
  $0 rebuild            # Clean and rebuild
  $0 shell              # Open shell in container

Output:
  Binaries: $BUILD_DIR/
  Distribution: $DIST_DIR/
  Package: dist/mdb-c-linux-arm64-4.3.0.tar.gz
EOF
}

# Main script logic
main() {
    local command="${1:-build}"

    case "$command" in
        build)
            check_docker_image
            run_docker_build
            ;;
        package)
            check_docker_image
            run_docker_package
            ;;
        clean)
            clean_build
            ;;
        rebuild)
            clean_build
            check_docker_image
            run_docker_build
            ;;
        shell)
            check_docker_image
            run_docker_shell
            ;;
        image)
            build_docker_image
            ;;
        help|--help|-h)
            print_usage
            ;;
        *)
            print_error "Unknown command: $command"
            print_usage
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
