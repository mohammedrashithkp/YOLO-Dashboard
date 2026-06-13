# =============================================================================
# Stage 1: Builder
# =============================================================================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
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
        libopencv-core4.5d \
        libopencv-imgproc4.5d \
        libopencv-imgcodecs4.5d \
        libopencv-videoio4.5d \
        libopencv-highgui4.5d \
        libyaml-cpp0.7 \
        libspdlog1 \
        libssh2-1 \
        libboost-system1.74.0 \
        libssl3 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built binary
COPY --from=builder /install/bin/YoloDashboard ./yolo_dashboard

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
