# YOLO Dashboard 🎯

A high-performance, cross-platform AI Inference Management Platform designed to run YOLO ONNX models at the edge. Built with a C++ backend, a sleek premium dark-mode web UI, and support for hardware acceleration via the Hailo-8 AI NPU.

## Features ✨
- **Ultra-Fast Inference Backend:** Written entirely in C++ (C++17) using OpenCV, ONNX Runtime, and Crow.
- **Hardware Accelerated:** Native support for CPU inference, with conditional compilation for Hailo-8 NPU (e.g. Raspberry Pi 5 AI Hat+).
- **Premium Web UI:** A beautiful, responsive glassmorphism dashboard built with Vanilla HTML/CSS/JS.
- **Real-Time Streaming:** Live MJPEG camera feeds directly to the browser.
- **Live Metrics:** Real-time FPS, latency, and detection charting via WebSockets.
- **Dockerized & Cross-Architecture:** Simple multi-stage Dockerfiles that auto-detect `amd64` and `arm64` platforms.
- **Remote Server Management:** Fully control the application from the UI, including disconnecting active cameras and gracefully shutting down the Docker containers.
- **Persistent Configuration:** All application settings and preferences survive container restarts.

## System Architecture 🏗️
The project uses a monolithic C++ backend serving a single-page application (SPA) frontend.
Please read the [Architecture Documentation](docs/architecture.md) for deeper details.

## Quick Start (Docker) 🐳

### Prerequisites
- Docker & Docker Compose
- (Optional) `usbipd-win` if passing cameras from Windows host to WSL2/Docker.

### Deployment
1. Clone the repository:
   ```bash
   git clone https://github.com/mohammedrashithkp/YOLO-Dashboard.git
   cd YOLO-Dashboard
   ```
2. Build and start the container:
   ```bash
   docker-compose up -d --build
   ```
3. Access the dashboard:
   - Open your browser to `http://localhost`
   - Default credentials: `admin` / `admin`

## Documentation 📚
- [Architecture & Design](docs/architecture.md)
- [REST & WebSocket API](docs/api.md)
- [Hardware & NPU Support](docs/hardware.md)

## Directories 📂
- `src/` & `include/` - C++ Backend Source Code
- `web/` - Frontend HTML, CSS, and JS
- `docs/` - Technical Documentation
- `config/` - YAML Configuration Files
- `data/` - Runtime data (models, logs, metrics, recordings)
