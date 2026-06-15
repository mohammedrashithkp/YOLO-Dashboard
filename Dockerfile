# =============================================================================
# Stage 1: Builder
# =============================================================================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        wget \
        pkg-config \
        libopencv-dev \
        libyaml-cpp-dev \
        libspdlog-dev \
        libssh2-1-dev \
        libboost-all-dev \
        libasio-dev \
        libssl-dev \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Install ONNX Runtime (architecture-aware for RPi/ARM64 support)
RUN ARCH=$(uname -m) && \
    ONNX_VERSION="1.17.1" && \
    if [ "$ARCH" = "x86_64" ]; then \
        ONNX_ARCH="linux-x64"; \
    elif [ "$ARCH" = "aarch64" ]; then \
        ONNX_ARCH="linux-aarch64"; \
    else \
        echo "Unsupported architecture: $ARCH" && exit 1; \
    fi && \
    wget -qO- "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-${ONNX_ARCH}-${ONNX_VERSION}.tgz" | tar xvz -C /usr/local/ --strip-components=1

# Copy project source
COPY CMakeLists.txt ./
COPY vcpkg.json     ./
COPY include/       include/
COPY src/           src/
COPY web/           web/
COPY config/        config/

# Configure and build
RUN cmake -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCROW_STATIC_DIRECTORY=/app/web \
    && cmake --build build --parallel "$(nproc)" \
    && cmake --install build --prefix /install

# =============================================================================
# Stage 2: Runtime
# =============================================================================
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        libopencv-core-dev \
        libopencv-imgproc-dev \
        libopencv-imgcodecs-dev \
        libopencv-videoio-dev \
        libopencv-highgui-dev \
        libyaml-cpp-dev \
        libspdlog-dev \
        libssh2-1 \
        libboost-system-dev \
        libssl3 \
        ca-certificates \
        v4l-utils \
        curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built binary
COPY --from=builder /install/bin/YoloDashboard ./yolo_dashboard

# Copy ONNX Runtime libraries
COPY --from=builder /usr/local/lib/libonnxruntime.so* /usr/local/lib/
RUN ldconfig

# Copy web assets and default configuration
COPY --from=builder /build/web/   ./web/
COPY --from=builder /build/config/ ./config/

# Create persistent data directories
RUN mkdir -p /app/data/models \
             /app/data/recordings \
             /app/data/logs \
             /app/data/metrics

EXPOSE 8080

ENTRYPOINT ["./yolo_dashboard"]
