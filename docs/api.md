# REST & WebSocket API Reference

The YOLO Dashboard C++ backend exposes several endpoints under the `/api/` prefix.

## Authentication
- `POST /api/auth/login`: Accepts `{"username", "password"}`, returns `{ "token": "..." }`.
- `POST /api/auth/logout`: Invalidates current token.

**Note:** All endpoints below require the `Authorization: Bearer <token>` header.

## Hardware & System
- `GET /api/hardware`: Returns JSON profiling CPU model, RAM usage, and available NPUs (e.g., Hailo TOPS).
- `GET /api/config`: Returns the current configuration as JSON/YAML.
- `POST /api/config`: Updates the system configuration.

## Camera & Inference
- `GET /api/cameras`: Enumerates available V4L2 / DirectShow capture devices.
- `POST /api/cameras/select`: Accepts `{"id": int}` to open a camera stream.
- `GET /api/models/browse`: Browse the `/data/models` directory for `.onnx` and `.hef` files.
- `POST /api/inference/start`: Accepts `{"model_path": "..."}` to initialize ONNX Runtime or HailoRT.
- `POST /api/inference/stop`: Terminates the inference thread.
- `GET /api/inference/status`: Returns current engine state.

## WebSockets
- `ws://<host>/ws/stream`: Pushes MJPEG JPEG buffers annotated with bounding boxes (if inference is active).
- `ws://<host>/ws/metrics`: Pushes live JSON telemetry (FPS, latency, detection count).
