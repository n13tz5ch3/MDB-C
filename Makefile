# Makefile for MDB-C Cross-Compilation
# Provides convenient shortcuts for Docker-based ARM64 builds

.PHONY: help arm64 arm64-package arm64-clean arm64-rebuild arm64-shell docker-image clean all

# Default target
help:
	@echo "MDB-C Cross-Compilation Targets:"
	@echo ""
	@echo "  make arm64           - Cross-compile for Linux ARM64 using Docker"
	@echo "  make arm64-package   - Create ARM64 distribution package (.tar.gz)"
	@echo "  make arm64-clean     - Clean ARM64 build artifacts"
	@echo "  make arm64-rebuild   - Clean and rebuild for ARM64"
	@echo "  make arm64-shell     - Open shell in ARM64 build container"
	@echo "  make docker-image    - Build/rebuild Docker image"
	@echo ""
	@echo "  make native          - Native build for current platform"
	@echo "  make clean           - Clean all build artifacts"
	@echo "  make all             - Build for all platforms"
	@echo ""
	@echo "Requirements:"
	@echo "  - Docker Desktop installed and running"
	@echo "  - macOS or Linux host system"
	@echo ""

# ARM64 cross-compilation targets
arm64:
	@echo "Building for Linux ARM64..."
	./docker-build-arm64.sh build

arm64-package:
	@echo "Creating ARM64 distribution package..."
	./docker-build-arm64.sh package

arm64-clean:
	@echo "Cleaning ARM64 build..."
	./docker-build-arm64.sh clean

arm64-rebuild:
	@echo "Rebuilding for Linux ARM64..."
	./docker-build-arm64.sh rebuild

arm64-shell:
	@echo "Opening ARM64 build shell..."
	./docker-build-arm64.sh shell

docker-image:
	@echo "Building Docker image..."
	./docker-build-arm64.sh image

# Native build
native:
	@echo "Building for native platform..."
	mkdir -p build/native
	cd build/native && cmake ../.. -DCMAKE_BUILD_TYPE=Release && cmake --build .

# Clean all
clean:
	@echo "Cleaning all build artifacts..."
	rm -rf build/docker-arm64
	rm -rf build/native
	rm -rf dist/linux-arm64

# Build for all platforms
all: native arm64

# Show build info
info:
	@echo "MDB-C Build Configuration:"
	@echo "  Version: 4.3.0"
	@echo "  Platforms:"
	@echo "    - Native (current platform)"
	@echo "    - Linux ARM64 (via Docker)"
	@echo ""
	@echo "Docker Images:"
	@docker images | grep mdb-c-arm64-builder || echo "  No ARM64 build image found"
	@echo ""
	@echo "Build Artifacts:"
	@ls -lh build/ 2>/dev/null || echo "  No build directory"
	@echo ""
	@ls -lh dist/ 2>/dev/null || echo "  No distribution directory"
