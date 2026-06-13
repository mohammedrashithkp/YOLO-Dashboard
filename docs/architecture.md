# Architecture Overview

YOLO Dashboard is designed as a standalone Edge application with two primary layers: a C++ Application Server (Backend) and a Vanilla SPA (Frontend), proxy-served via Nginx.

## 1. Backend (C++17)
The backend leverages the **Crow** web framework to expose REST APIs, serve static files, and manage WebSocket connections. 

**Core Subsystems:**
- `CameraManager`: Abstraction for video capture. Uses DirectShow on Windows (`camera_enumerator_win.cpp`) and V4L2 on Linux (`camera_enumerator_linux.cpp`).
- `InferenceEngine`: Abstract interface. The primary implementation is `OnnxEngine` which relies on Microsoft's ONNX Runtime. When compiled on supported platforms, a `HailoEngine` is conditionally injected.
- `HardwareDetector`: Probes the system using platform-specific calls (`/proc/cpuinfo` on Linux, native APIs on Windows) to detect CPU capabilities, RAM, and the presence of Hailo-8 M.2/PCIe hats.
- `MetricsCollector` & `LogManager`: Thread-safe singleton-like structures recording moving averages, detections, and log streams, utilizing `spdlog`.
- `RouteHandler` & `WebSocketHandler`: Exposes API and pushes live streaming MJPEG buffers over the network.

## 2. Frontend (HTML/JS/CSS)
- **Frameworkless:** Built with ES6 Modules and Vanilla JS to guarantee performance on low-power devices.
- **Routing:** Hash-based SPA routing (`#camera`, `#hardware`, etc.).
- **Rendering:** Uses the Canvas 2D API for live metrics charting (FPS, Latency) and MJPEG manipulation.

## 3. Deployment
- Handled through `docker-compose`. Nginx acts as a reverse proxy, caching static assets and safely upgrading WebSocket connections. 
